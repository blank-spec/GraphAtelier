#include <QImage>
#include <QMouseEvent>
#include <QPainterPath>
#include <QSvgGenerator>
#include <QtMath>
#include <algorithm>
#include <graphitem.h>


namespace {
    void drawLabel(QPainter &p, const QRectF &r, const QString &t, QColor c, int size, bool bold = false) {
        QFont f("Manrope", size);
        f.setBold(bold);
        p.setFont(f);
        p.setPen(c);
        p.drawText(r, Qt::AlignCenter, t);
    }
}

GraphItem::GraphItem(QQuickItem *p) : QQuickPaintedItem(p) {
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setOpaquePainting(false);
}

Engine *GraphItem::engine() const { return m_engine; }

void GraphItem::setEngine(Engine *e) {
    if (m_engine) {
        disconnect(m_engine, nullptr, this, nullptr);
    }

    m_engine = e;
    m_custom.clear();
    m_layoutN = e ? e->nodeCount() : 0;

    if (e) {
        connect(e, &Engine::changed, this, [this] {
            if (m_layoutN != m_engine->nodeCount()) {
                m_custom.clear();
                m_layoutN = m_engine->nodeCount();
                m_drag = -1;
            }
            update();
        });
    }
    emit engineChanged();
    update();
}

bool GraphItem::showAll() const { return m_all; }

void GraphItem::setShowAll(bool showAll) {
    if (showAll == m_all) {
        return;
    }

    m_all = showAll;
    update();
    emit viewChanged();
}

bool GraphItem::showWeights() const { return m_weights; }

void GraphItem::setShowWeights(bool showWeights) {
    if (showWeights == m_weights) {
        return;
    }

    m_weights = showWeights;
    update();
    emit viewChanged();
}

qreal GraphItem::phase() const { return m_phase; }

void GraphItem::setPhase(qreal phase) {
    m_phase = phase;
    update();
    emit phaseChanged();
}

void GraphItem::paint(QPainter *p) {
    if (m_engine) {
        draw(*p, boundingRect(), *m_engine, positions(), m_all, m_weights, m_phase);
    }
}

void GraphItem::resetLayout() {
    m_custom.clear();
    update();
}

bool GraphItem::saveGraph(const Engine &e, const QString &path, bool png) {
    QVector<QPointF> pos;
    for (int i = 0; i < e.nodeCount(); ++i) {
        qreal a = -M_PI_2 + 2 * M_PI * i / e.nodeCount();
        pos << QPointF(.5 + .4 * qCos(a), .5 + .4 * qSin(a));
    }

    QSize size(1800, 1100);
    if (png) {
        QImage img(size, QImage::Format_ARGB32_Premultiplied);
        QPainter p(&img);
        draw(p, QRectF(QPointF(), size), e, pos, e.mode() != 0, true, 0, true);
        p.end();
        return img.save(path);
    }

    QSvgGenerator svg;
    svg.setFileName(path);
    svg.setSize(size);
    svg.setViewBox(QRect(QPoint(), size));
    svg.setTitle("Graph Atelier — variant " + QString::number(e.variant()));
    QPainter p;

    if (!p.begin(&svg)) {
        return false;
    }
    draw(p, QRectF(QPointF(), size), e, pos, e.mode() != 0, true, 0, true);

    return p.end();
}

void GraphItem::mousePressEvent(QMouseEvent *ev) {
    auto points = positions();
    for (int i = 0; i < points.size(); ++i) {
        QPointF pt(32 + points[i].x() * (width() - 64), 30 + points[i].y() * (height() - 60));
        if (QLineF(pt, ev->position()).length() < 27) {
            m_drag = i;
            m_custom = points;
            if (m_engine) {
                m_engine->setTarget(i);
            }
            ev->accept();
            return;
        }
    }
    ev->ignore();
}

void GraphItem::mouseMoveEvent(QMouseEvent *ev) {
    if (m_drag >= 0 && width() > 100 && height() > 100) {
        m_custom[m_drag] = QPointF(qBound(.02, (ev->position().x() - 32) / (width() - 64), .98),
                                   qBound(.02, (ev->position().y() - 30) / (height() - 60), .98));
        update();
    }
}

void GraphItem::mouseReleaseEvent(QMouseEvent *ev) {
    m_drag = -1;
    ev->accept();
}

QVector<QPointF> GraphItem::positions() const {
    if (!m_engine) {
        return {};
    }
    if (!m_custom.empty()) {
        return m_custom;
    }

    QVector<QPointF> p;
    int n = m_engine->nodeCount();

    for (int i = 0; i < n; ++i) {
        qreal a = -M_PI_2 + 2 * M_PI * i / n;
        p.append(QPointF(.5 + .40 * qCos(a), .5 + .37 * qSin(a)));
    }

    return p;
}

void GraphItem::draw(QPainter &p, const QRectF &rect, const Engine &e, const QVector<QPointF> &points, bool all,
                     bool weights, qreal phase, bool exporting) {
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect, QColor("#141923"));

    const int n = e.nodeCount();
    QRectF area = rect.adjusted(32, 30, -32, -30);

    if (exporting) {
        area = rect.adjusted(50, 120, e.mode() ? -550 : -50, -85);
    }

    QVector<QPointF> pos;
    for (auto a: points) {
        pos << QPointF(area.left() + a.x() * area.width(), area.top() + a.y() * area.height());
    }

    const qreal radius = exporting ? 27 : 21;
    const auto path = e.currentPath();
    auto pathEdge = [&](int u, int v) {
        for (size_t k = 1; k < path.size(); ++k) {
            if (path[k - 1] == u && path[k] == v) {
                return true;
            }
        }
        return false;
    };
    for (qreal x = area.left(); x < area.right(); x += 28) {
        for (qreal y = area.top(); y < area.bottom(); y += 28) {
            p.setPen(QColor("#293246"));
            p.drawPoint(QPointF(x, y));
        }
    }

    const auto &a = e.matrix();
    for (int pass = 0; pass < 2; ++pass) {
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                if (!a[u][v] || u == v) {
                    continue;
                }
                const bool route = pathEdge(u, v), tree = !e.mode() && e.dStep().parent[v] == u;
                const bool active =
                        e.mode() ? (u == e.wStep().k || v == e.wStep().k) : (e.dStep().from == u && e.dStep().to == v);
                const bool strong = route || tree || active;
                if ((pass == 0 && strong) || (pass == 1 && !strong) || (!all && !strong && !e.mode())) {
                    continue;
                }
                QColor color = route    ? QColor("#89b4ff")
                               : active ? QColor("#c1adff")
                               : tree   ? QColor("#8392b2")
                                        : QColor("#344159");
                QPointF delta = pos[v] - pos[u];
                qreal len = qSqrt(QPointF::dotProduct(delta, delta));
                if (len < 1) {
                    continue;
                }
                QPointF normal(-delta.y() / len, delta.x() / len);
                QPointF ctrl = (pos[u] + pos[v]) / 2 + normal * qMin<qreal>(all ? 22 : 12, len * .15);
                QPointF start = pos[u] + (ctrl - pos[u]) / QLineF(pos[u], ctrl).length() * (radius + 2);
                QPointF end = pos[v] + (ctrl - pos[v]) / QLineF(pos[v], ctrl).length() * (radius + 4);
                QPainterPath curve(start);
                curve.quadTo(ctrl, end);
                p.setPen(QPen(color, route ? 3.4 : active ? 2.2 : tree ? 1.7 : 1));
                p.setBrush(Qt::NoBrush);
                p.drawPath(curve);
                QPointF tangent = end - ctrl;
                tangent /= QLineF(QPointF(), tangent).length();
                QPointF side(-tangent.y(), tangent.x());
                p.setPen(Qt::NoPen);
                p.setBrush(color);
                p.drawPolygon(QPolygonF{end, end - tangent * 8 + side * 3.7, end - tangent * 8 - side * 3.7});
                if (weights && !e.mode() && (strong || exporting)) {
                    QPointF mid = curve.pointAtPercent(.5);
                    QRectF box(mid.x() - 13, mid.y() - 10, 26, 20);
                    p.setBrush(QColor("#141923"));
                    p.drawRoundedRect(box, 4, 4);
                    drawLabel(p, box, QString::number(a[u][v]), strong ? QColor("#dce4f4") : QColor("#9baac7"),
                              exporting ? 10 : 9);
                }
                if (route && phase > 0) {
                    p.setBrush(QColor("#e2eaff"));
                    p.drawEllipse(curve.pointAtPercent(phase), 3.5, 3.5);
                }
            }
        }
    }

    for (int i = 0; i < n; ++i) {
        bool selected = i == e.target(), source = !e.mode() && i == e.source(),
             active = e.mode() ? i == e.wStep().k : i == e.dStep().current;
        bool onPath = std::find(path.begin(), path.end(), i) != path.end();
        QColor fill = source ? QColor("#89b4ff") : active ? QColor("#c1adff") : QColor("#232e44");
        p.setBrush(fill);
        p.setPen(QPen(selected             ? QColor("#ed799d")
                      : source               ? QColor("#89b4ff")
                      : onPath || selected ? QColor("#89b4ff")
                                           : QColor("#6e80a2"),
                      selected ? 2.5 : 1.3));
        p.drawEllipse(pos[i], radius, radius);
        if (selected) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor("#a34e73"), 1));
            p.drawEllipse(pos[i], radius + 5, radius + 5);
        }
        drawLabel(p, QRectF(pos[i].x() - radius, pos[i].y() - radius, 2 * radius, 2 * radius), QString::number(i + 1),
                  source || active ? QColor("#151c30") : QColor("#edf1fa"), exporting ? 14 : 11, true);
        if (!e.mode()) {
            int d = e.dStep().distance[i];
            drawLabel(p, QRectF(pos[i].x() - 25, pos[i].y() + radius + 5, 50, 18),
                      d == algo::Inf ? QString::fromUtf8("∞") : QString::number(d), QColor("#b4bfd3"),
                      exporting ? 11 : 9);
        }
    }

    if (exporting) {
        drawLabel(p, QRectF(45, 20, rect.width() - 90, 55),
                  QString("Graph Atelier / %1 / вариант %2").arg(e.mode() ? "Уоршелл" : "Дейкстра").arg(e.variant()),
                  QColor("#edf1fa"), 22, true);
        drawLabel(p, QRectF(45, rect.height() - 62, rect.width() - 90, 32),
                  e.mode() ? "Исходные дуги · справа — итоговая достижимость (строка → столбец)"
                           : QString("Источник v%1 · числа у вершин — дистанции · синий — маршрут к v%2")
                                     .arg(e.source() + 1)
                                     .arg(e.target() + 1),
                  QColor("#b4bfd3"), 12);
        if (e.mode()) {
            qreal left = rect.right() - 500, top = 220, cell = 38;
            for (int i = 0; i < 10; ++i) {
                drawLabel(p, QRectF(left + (i + 1) * cell, top, cell, cell), QString::number(i + 1), QColor("#dce4f4"),
                          11);
                drawLabel(p, QRectF(left, top + (i + 1) * cell, cell, cell), QString::number(i + 1), QColor("#dce4f4"),
                          11);
                for (int j = 0; j < 10; ++j) {
                    QRectF r(left + (j + 1) * cell + 2, top + (i + 1) * cell + 2, cell - 4, cell - 4);
                    p.fillRect(r, e.wStep().reach[i][j] ? QColor("#c1adff") : QColor("#283349"));
                    drawLabel(p, r, QString::number(e.wStep().reach[i][j]), QColor("#151c30"), 11);
                }
            }
        }
    }

    p.restore();
}
