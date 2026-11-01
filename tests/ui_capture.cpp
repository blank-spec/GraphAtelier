#include <ui_benchmark.h>
#include <ui_capture.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

#include <engine.h>
#include <graphitem.h>

namespace {
    QQuickItem *findVisualItem(QQuickItem *item, const QString &name) {
        if (item->objectName() == name) {
            return item;
        }

        for (auto *child: item->childItems()) {
            if (auto *match = findVisualItem(child, name)) {
                return match;
            }
        }

        return nullptr;
    }

    void setPopupVisible(QObject *root, const char *name, bool visible) {
        auto *choice = root->findChild<QObject *>(name);

        if (!choice) {
            return;
        }

        auto *popup = choice->property("popup").value<QObject *>();

        if (popup) {
            QMetaObject::invokeMethod(popup, visible ? "open" : "close");
        }
    }

    void saveWindow(QQuickWindow *window, const QString &path) {
        if (window) {
            window->grabWindow().save(path);
        }
    }

    void savePresentationFrame(QQuickWindow *window, const QString &directory, const QString &name) {
        const QImage screenshot = window->grabWindow();
        screenshot.save(directory + "/" + name + ".png");
        auto *graph = window->findChild<GraphItem *>();

        if (graph) {
            const QRectF graphBounds = graph->mapRectToScene(graph->boundingRect());
            const qreal scale = screenshot.devicePixelRatio();
            const QRect pixelBounds(qRound(graphBounds.x() * scale), qRound(graphBounds.y() * scale),
                                    qRound(graphBounds.width() * scale), qRound(graphBounds.height() * scale));
            screenshot.copy(pixelBounds).save(directory + "/" + name + "-graph.png");
        }
    }

    void capturePresentation(QQuickWindow *window, Engine &engine, const QString &directory) {
        auto *timer = new QTimer(window);
        timer->setInterval(450);
        engine.reset();

        QObject::connect(timer, &QTimer::timeout, window, [window, &engine, directory, timer, stage = 0]() mutable {
            switch (stage++) {
                case 0:
                    savePresentationFrame(window, directory, "dijkstra-start");
                    engine.setStep(engine.stepCount() / 2);
                    break;
                case 1:
                    savePresentationFrame(window, directory, "dijkstra-progress");
                    engine.finish();
                    break;
                case 2:
                    savePresentationFrame(window, directory, "dijkstra-final");
                    engine.setMode(1);
                    break;
                case 3:
                    savePresentationFrame(window, directory, "warshall-start");
                    engine.setStep(2);
                    break;
                case 4:
                    savePresentationFrame(window, directory, "warshall-progress");
                    engine.finish();
                    break;
                case 5:
                    savePresentationFrame(window, directory, "warshall-final");
                    engine.setMode(0);
                    engine.finish();
                    QMetaObject::invokeMethod(window, "toggleGraphFullscreen");
                    break;
                default:
                    savePresentationFrame(window, directory, "fullscreen");
                    timer->stop();
                    QCoreApplication::quit();
                    break;
            }
        });

        timer->start();
    }
}

void configureUiCapture(QQmlApplicationEngine &qml, Engine &engine) {
    const QStringList arguments = QCoreApplication::arguments();
    const int captureIndex = arguments.indexOf("--capture");
    auto *window = qobject_cast<QQuickWindow *>(qml.rootObjects().value(0));

    if (!window || captureIndex < 0) {
        return;
    }

    const QString directory = arguments.value(captureIndex + 1, ".");
    QDir().mkpath(directory);

    if (arguments.contains("--presentation")) {
        capturePresentation(window, engine, directory);
        return;
    }

    if (arguments.contains("--benchmark-switch")) {
        auto *benchmark = new UiBenchmark(window, engine, directory);
        benchmark->start();
        return;
    }

    if (arguments.contains("--compact")) {
        window->resize(1120, 760);
    }

    if (arguments.contains("--focus-row")) {
        window->requestActivate();

        QTimer::singleShot(300, window, [window] {
            auto *row = findVisualItem(window->contentItem(), "distanceRow8");

            if (row) {
                row->forceActiveFocus(Qt::TabFocusReason);
            }
        });

        QTimer::singleShot(650, window, [window, directory] {
            auto *row = findVisualItem(window->contentItem(), "distanceRow8");

            if (!row) {
                QCoreApplication::exit(3);
                return;
            }

            auto *background = row->property("background").value<QQuickItem *>();
            auto *content = row->property("contentItem").value<QQuickItem *>();
            const QJsonObject geometry{{"row_height", row->height()},
                                       {"row_width", row->width()},
                                       {"visual_focus", row->property("visualFocus").toBool()},
                                       {"background_y", background ? background->y() : -1},
                                       {"background_height", background ? background->height() : -1},
                                       {"content_y", content ? content->y() : -1},
                                       {"content_height", content ? content->height() : -1}};

            QFile report(directory + "/row-geometry.json");

            if (report.open(QIODevice::WriteOnly)) {
                report.write(QJsonDocument(geometry).toJson());
            }

            saveWindow(window, directory + "/row-focus.png");
            QCoreApplication::quit();
        });

        return;
    }

    if (arguments.contains("--menus")) {
        QTimer::singleShot(300, window, [window] { setPopupVisible(window, "variantChoice", true); });

        QTimer::singleShot(650, window, [window, directory] {
            saveWindow(window, directory + "/variant-menu.png");
            setPopupVisible(window, "variantChoice", false);
            setPopupVisible(window, "sourceChoice", true);
        });

        QTimer::singleShot(1000, window, [window, directory] {
            saveWindow(window, directory + "/source-menu.png");
            QCoreApplication::quit();
        });

        return;
    }

    QTimer::singleShot(400, window, [&engine] { engine.finish(); });

    QTimer::singleShot(800, window, [window, &engine, directory] {
        saveWindow(window, directory + "/dijkstra.png");
        engine.setMode(1);
        engine.setStep(2);
    });

    QTimer::singleShot(1300, window, [window, directory] {
        saveWindow(window, directory + "/warshall.png");
        QCoreApplication::quit();
    });
}
