#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantList>

#include <algorithms.h>

#include <array>
#include <optional>

class Engine : public QObject {
    Q_OBJECT

    Q_PROPERTY(int variant READ variant WRITE setVariant NOTIFY changed)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(int source READ source WRITE setSource NOTIFY changed)
    Q_PROPERTY(int target READ target WRITE setTarget NOTIFY changed)
    Q_PROPERTY(int step READ step WRITE setStep NOTIFY changed)
    Q_PROPERTY(int stepCount READ stepCount NOTIFY changed)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY changed)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY changed)
    Q_PROPERTY(bool reflexive READ reflexive WRITE setReflexive NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList cells READ cells NOTIFY cellsChanged)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString pathText READ pathText NOTIFY changed)
    Q_PROPERTY(QString distanceText READ distanceText NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit Engine(QObject *parent = nullptr);
    int variant() const;
    int mode() const;
    int source() const;
    int target() const;
    int step() const;
    int stepCount() const;
    int nodeCount() const;
    int edgeCount() const;
    bool reflexive() const;
    void setVariant(int variant);
    void setMode(int mode);
    void setSource(int source);
    void setTarget(int target);
    void setStep(int step);
    void setReflexive(bool reflexive);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void finish();
    Q_INVOKABLE void reset();
    QVariantList rows() const;
    QVariantList cells() const;
    QString message() const;
    QString pathText() const;
    QString distanceText() const;
    QString status() const;
    const algo::Matrix &matrix() const;
    const algo::DijkstraStep &dStep() const;
    const algo::WarshallStep &wStep() const;
    std::vector<int> currentPath() const;
    Q_INVOKABLE bool exportResults(const QString &folder);

    bool exportTo(const QString &folder);

signals:
    void changed();
    void statusChanged();
    void rowsChanged();
    void cellsChanged();

private:
    void rebuild();
    void loadVariant();
    const algo::DijkstraResult &dijkstraResult() const;
    const std::vector<algo::WarshallStep> &warshallSteps() const;

    QJsonObject m_data;
    algo::Matrix m_dijkstraGraph;
    algo::Matrix m_warshallGraph;
    std::array<std::optional<algo::DijkstraResult>, 15> m_dijkstraCache;
    std::array<std::optional<std::vector<algo::WarshallStep>>, 2> m_warshallCache;

    int m_variant = 18;
    int m_mode = 0;
    int m_source = 0;
    int m_target = 13;
    int m_step = 0;
    bool m_reflexive = false;
    QString m_status;
};
