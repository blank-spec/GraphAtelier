#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStyleHints>
#include <engine.h>
#include <graphitem.h>

#ifdef ATELIER_UI_CAPTURE
#include <ui_capture.h>
#endif

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("Graph Atelier");
    app.setOrganizationName("GraphAtelier");

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

    QFontDatabase::addApplicationFont(":/data/Manrope.ttf");
    app.setFont(QFont("Manrope", 10));
    QQuickStyle::setStyle("Basic");

    Engine engine;
    QStringList args = app.arguments();

    if (args.contains("--export")) {
        int i = args.indexOf("--export");
        return i + 1 < args.size() && engine.exportTo(args[i + 1]) ? 0 : 2;
    }

    qmlRegisterType<GraphItem>("Atelier", 1, 0, "GraphView");
    QQmlApplicationEngine qml;
    qml.rootContext()->setContextProperty("lab", &engine);

    QObject::connect(&qml, &QQmlApplicationEngine::warnings, &app, [](const QList<QQmlError> &warnings) {
        QFile log(QCoreApplication::applicationDirPath() + "/GraphAtelier.log");

        if (!log.open(QIODevice::WriteOnly | QIODevice::Append)) {
            return;
        }

        for (const auto &warning: warnings) {
            log.write(warning.toString().toUtf8() + '\n');
        }
    });

    QObject::connect(
            &qml, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
            Qt::QueuedConnection);

    qml.loadFromModule("GraphAtelier", "Main");

#ifdef ATELIER_UI_CAPTURE
    configureUiCapture(qml, engine);
#endif

    return app.exec();
}
