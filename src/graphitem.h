#pragma once

#include <QPainter>
#include <QPointer>
#include <QQuickPaintedItem>

#include <engine.h>

class GraphItem : public QQuickPaintedItem {
    Q_OBJECT

    Q_PROPERTY(Engine *engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(bool showAll READ showAll WRITE setShowAll NOTIFY viewChanged)
    Q_PROPERTY(bool showWeights READ showWeights WRITE setShowWeights NOTIFY viewChanged)
    Q_PROPERTY(qreal phase READ phase WRITE setPhase NOTIFY phaseChanged)

public:
    explicit GraphItem(QQuickItem *parent = nullptr);
    Engine *engine() const;
    void setEngine(Engine *engine);
    bool showAll() const;
    void setShowAll(bool showAll);

    bool showWeights() const;
    void setShowWeights(bool showWeights);

    qreal phase() const;
    void setPhase(qreal phase);
    void paint(QPainter *painter) override;
    Q_INVOKABLE void resetLayout();

    static bool saveGraph(const Engine &engine, const QString &path, bool png);

signals:
    void engineChanged();
    void viewChanged();
    void phaseChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    static void draw(QPainter &painter, const QRectF &bounds, const Engine &engine, const QVector<QPointF> &positions,
                     bool showAll, bool showWeights, qreal phase, bool exporting = false);

    QVector<QPointF> positions() const;

    QPointer<Engine> m_engine;
    QVector<QPointF> m_custom;
    bool m_all = false;
    bool m_weights = true;
    int m_drag = -1;
    int m_layoutN = 0;
    qreal m_phase = 0;
};
