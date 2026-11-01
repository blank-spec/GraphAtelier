#pragma once

#include <QElapsedTimer>
#include <QJsonArray>
#include <QObject>
#include <QString>

class Engine;
class QQuickWindow;

class UiBenchmark final : public QObject {
public:
    UiBenchmark(QQuickWindow *window, Engine &engine, const QString &directory);

    void start();

private:
    void beginSwitch();
    void completeSwitch();
    void saveReport(bool completed);

    QQuickWindow *m_window;
    Engine &m_engine;
    QString m_directory;
    QElapsedTimer m_timer;
    QJsonArray m_measurements;
    double m_callMilliseconds = 0;
    bool m_pending = false;
};
