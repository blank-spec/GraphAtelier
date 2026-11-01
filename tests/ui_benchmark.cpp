#include <ui_benchmark.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QTimer>

#include <engine.h>

UiBenchmark::UiBenchmark(QQuickWindow *window, Engine &engine, const QString &directory) :
    QObject(window), m_window(window), m_engine(engine), m_directory(directory) {
    connect(window, &QQuickWindow::frameSwapped, this, &UiBenchmark::completeSwitch, Qt::QueuedConnection);
}

void UiBenchmark::start() {
    QTimer::singleShot(500, this, &UiBenchmark::beginSwitch);

    QTimer::singleShot(15000, this, [this] {
        saveReport(false);
        QCoreApplication::exit(2);
    });
}

void UiBenchmark::beginSwitch() {
    m_pending = true;
    m_timer.start();

    const int mode = m_measurements.size() % 2 == 0 ? 1 : 0;
    m_engine.setMode(mode);
    m_callMilliseconds = m_timer.nsecsElapsed() / 1'000'000.0;

    m_window->update();
}

void UiBenchmark::completeSwitch() {
    if (!m_pending) {
        return;
    }

    m_pending = false;
    m_measurements.append(QJsonObject{{"mode", m_engine.mode()},
                                      {"set_mode_ms", m_callMilliseconds},
                                      {"first_frame_ms", m_timer.nsecsElapsed() / 1'000'000.0}});

    if (m_measurements.size() == 5) {
        saveReport(true);
        QCoreApplication::quit();
        return;
    }

    QTimer::singleShot(300, this, &UiBenchmark::beginSwitch);
}

void UiBenchmark::saveReport(bool completed) {
    QDir().mkpath(m_directory);
    QFile file(m_directory + "/switch-benchmark.json");

    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    const QJsonObject report{
            {"completed", completed}, {"qt_version", QT_VERSION_STR}, {"measurements", m_measurements}};

    file.write(QJsonDocument(report).toJson());
}
