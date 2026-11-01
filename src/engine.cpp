#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUrl>
#include <engine.h>
#include <graphitem.h>
#include <stdexcept>

namespace {
    algo::Matrix parseAdjacency(const QJsonArray &rows) {
        algo::Matrix adjacency;
        for (const auto &rowValue: rows) {
            std::vector<int> row;
            for (const auto &value: rowValue.toArray()) {
                row.push_back(value.toInt());
            }
            adjacency.push_back(row);
        }

        return adjacency;
    }

    QString formatDistance(int n) { return n >= algo::Inf ? QString::fromUtf8("∞") : QString::number(n); }

    QString formatRoute(const std::vector<int> &p) {
        QStringList r;
        for (int v: p) {
            r << QString("v%1").arg(v + 1);
        }

        return r.join(" → ");
    }

}

Engine::Engine(QObject *p) : QObject(p) {
    QFile f(":/data/graphs.json");
    if (!f.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Missing dataset");
    }

    m_data = QJsonDocument::fromJson(f.readAll()).object();
    loadVariant();
    rebuild();
    finish();
}

int Engine::variant() const { return m_variant; }

int Engine::mode() const { return m_mode; }

int Engine::source() const { return m_source; }

int Engine::target() const { return m_target; }

int Engine::step() const { return m_step; }

int Engine::stepCount() const {
    return static_cast<int>(m_mode ? warshallSteps().size() : dijkstraResult().steps.size());
}

int Engine::nodeCount() const { return static_cast<int>(matrix().size()); }

int Engine::edgeCount() const {
    int n = 0;
    for (auto &r: matrix()) {
        for (int x: r) {
            n += x > 0;
        }
    }

    return n;
}

bool Engine::reflexive() const { return m_reflexive; }

void Engine::setVariant(int variant) {
    if (variant < 1 || variant > 20 || variant == m_variant) {
        return;
    }

    m_variant = variant;
    loadVariant();
    rebuild();
}

void Engine::setMode(int mode) {
    if (mode < 0 || mode > 1 || mode == m_mode) {
        return;
    }

    m_mode = mode;
    m_target = qMin(m_target, nodeCount() - 1);
    m_step = 0;
    emit rowsChanged();
    emit cellsChanged();
    emit changed();
}

void Engine::setSource(int source) {
    if (source < 0 || source >= 15 || source == m_source) {
        return;
    }

    m_source = source;
    rebuild();
}

void Engine::setTarget(int target) {
    if (target < 0 || target >= nodeCount() || target == m_target) {
        return;
    }

    m_target = target;
    emit changed();
}

void Engine::setStep(int step) {
    step = qBound(0, step, stepCount() - 1);
    if (step == m_step) {
        return;
    }

    m_step = step;

    if (m_mode == 0) {
        emit rowsChanged();
    } else {
        emit cellsChanged();
    }

    emit changed();
}

void Engine::setReflexive(bool reflexive) {
    if (reflexive == m_reflexive) {
        return;
    }

    m_reflexive = reflexive;
    rebuild();
}

void Engine::next() { setStep(m_step + 1); }

void Engine::previous() { setStep(m_step - 1); }

void Engine::finish() { setStep(stepCount() - 1); }

void Engine::reset() { setStep(0); }

QVariantList Engine::rows() const {
    QVariantList list;
    const auto &s = dStep();

    for (int i = 0; i < 15; ++i) {
        list << QVariantMap{
                {"vertex", i + 1},
                {"distance", formatDistance(s.distance[i])},
                {"fixed", s.fixed[i]},
                {"active", i == s.current},
                {"path", s.distance[i] == algo::Inf ? "Нет пути" : formatRoute(algo::route(s.parent, m_source, i))}};
    }

    return list;
}

QVariantList Engine::cells() const {
    QVariantList list;
    const auto &s = wStep();

    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            list << QVariantMap{{"row", i},
                                {"col", j},
                                {"value", s.reach[i][j]},
                                {"fresh", s.changed[i][j] > 0},
                                {"original", m_warshallGraph[i][j] > 0}};
        }
    }

    return list;
}

QString Engine::message() const {
    if (m_mode) {
        if (!m_step) {
            return "Исходная матрица. Выберите шаг, чтобы открыть новые связи.";
        }

        int count = 0;
        for (auto &r: wStep().changed) {
            for (int x: r) {
                count += x;
            }
        }

        return QString("Промежуточная вершина v%1 · новых пар: %2").arg(wStep().k + 1).arg(count);
    }

    const auto &s = dStep();
    if (!m_step) {
        return QString("Старт в v%1. Дистанция 0, остальные — ∞.").arg(m_source + 1);
    }

    if (s.note == "fix") {
        return QString("Фиксируем v%1: минимальная дистанция %2.").arg(s.current + 1).arg(s.distance[s.current]);
    }

    return QString("Релаксация v%1 → v%2: новая дистанция %3.").arg(s.from + 1).arg(s.to + 1).arg(s.distance[s.to]);
}

QString Engine::pathText() const {
    if (m_mode) {
        return "R[i,j] = R[i,j] OR (R[i,k] AND R[k,j])";
    }

    auto p = currentPath();
    return p.empty() ? "Маршрут ещё не найден" : formatRoute(p);
}

QString Engine::distanceText() const {
    if (!m_mode) {
        return formatDistance(dStep().distance[m_target]);
    }

    int reachablePairs = 0;

    for (const auto &row: wStep().reach) {
        for (int reachable: row) {
            reachablePairs += reachable;
        }
    }

    return QString::number(reachablePairs);
}

QString Engine::status() const { return m_status; }

const algo::Matrix &Engine::matrix() const { return m_mode ? m_warshallGraph : m_dijkstraGraph; }

const algo::DijkstraStep &Engine::dStep() const {
    const int index = m_mode ? static_cast<int>(dijkstraResult().steps.size()) - 1 : m_step;

    return dijkstraResult().steps.at(index);
}

const algo::WarshallStep &Engine::wStep() const {
    const int index = m_mode ? m_step : static_cast<int>(warshallSteps().size()) - 1;

    return warshallSteps().at(index);
}

std::vector<int> Engine::currentPath() const {
    if (m_mode) {
        return {};
    }

    const auto &s = dStep();
    if (s.distance[m_target] == algo::Inf) {
        return {};
    }

    return algo::route(s.parent, m_source, m_target);
}

bool Engine::exportResults(const QString &folder) {
    QString p = QUrl(folder).isLocalFile() ? QUrl(folder).toLocalFile() : folder;
    return exportTo(p);
}

bool Engine::exportTo(const QString &folder) {
    try {
        if (folder.isEmpty()) {
            throw std::runtime_error("Folder is empty");
        }

        QDir dir(folder);
        if (!dir.mkpath(".")) {
            throw std::runtime_error("Cannot create folder");
        }

        auto write = [&](const QString &name, const QByteArray &bytes) {
            QSaveFile f(dir.filePath(name));
            if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
                throw std::runtime_error("Cannot save file");
            }
        };

        auto csv = [](const algo::Matrix &a) {
            QString t = "from/to";
            for (int i = 0; i < static_cast<int>(a.size()); ++i) {
                t += QString(",v%1").arg(i + 1);
            }
            t += '\n';
            for (int i = 0; i < static_cast<int>(a.size()); ++i) {
                t += QString("v%1").arg(i + 1);
                for (int x: a[i]) {
                    t += "," + formatDistance(x);
                }
                t += '\n';
            }
            return t.toUtf8();
        };

        write("warshall_reachability.csv", csv(warshallSteps().back().reach));
        write("warshall_input.csv", csv(m_warshallGraph));
        write("dijkstra_input.csv", csv(m_dijkstraGraph));

        algo::Matrix all;
        QString routes = "source,target,distance,route\n";
        QString sourceRoutes = "target,distance,route\n";
        QJsonArray snapshots;

        for (int s = 0; s < 15; ++s) {
            auto r = algo::dijkstra(m_dijkstraGraph, s, algo::TraceMode::ResultOnly);
            all.push_back(r.distance);
            for (int t = 0; t < 15; ++t) {
                QString line = QString("v%1,%2,%3\n")
                                       .arg(t + 1)
                                       .arg(formatDistance(r.distance[t]), formatRoute(algo::route(r.parent, s, t)));
                routes += QString("v%1,").arg(s + 1) + line;
                if (s == m_source) {
                    sourceRoutes += line;
                }
            }
        }

        write("dijkstra_all_distances.csv", csv(all));
        write("dijkstra_all_routes.csv", routes.toUtf8());
        write("dijkstra_selected_source.csv", sourceRoutes.toUtf8());

        for (auto &s: warshallSteps()) {
            QJsonArray a;
            for (auto &r: s.reach) {
                QJsonArray row;
                for (int v: r) {
                    row.append(v);
                }
                a.append(row);
            }
            snapshots.append(QJsonObject{{"k", s.k + 1}, {"reach", a}});
        }

        write("warshall_steps.json", QJsonDocument(snapshots).toJson());

        const int oldMode = m_mode, oldStep = m_step;
        m_mode = 0;
        m_step = static_cast<int>(dijkstraResult().steps.size()) - 1;
        const bool dSaved = GraphItem::saveGraph(*this, dir.filePath("dijkstra_graph.svg"), false) &&
                            GraphItem::saveGraph(*this, dir.filePath("dijkstra_graph.png"), true);

        m_mode = 1;
        m_step = static_cast<int>(warshallSteps().size()) - 1;
        const bool wSaved = GraphItem::saveGraph(*this, dir.filePath("warshall_graph.svg"), false) &&
                            GraphItem::saveGraph(*this, dir.filePath("warshall_graph.png"), true);

        m_mode = oldMode;
        m_step = oldStep;

        if (!dSaved || !wSaved) {
            throw std::runtime_error("Cannot save visualization");
        }

        QString report = QString("# Вариант %1\n\nДейкстра: 15 вершин, положительные веса, 0 = отсутствие "
                                 "дуги.\nСтрока — начало дуги; столбец — конец.\n\nВыбранный источник: v%2. "
                                 "Кратчайшие пути для всех 15 источников находятся в "
                                 "dijkstra_all_routes.csv.\n\nУоршелл: 10 вершин. Режим: %3.\nВ режиме A+ "
                                 "единица на диагонали появляется только при наличии цикла.\n\nPNG и SVG: граф "
                                 "Дейкстры с деревом кратчайших путей и выбранным маршрутом; исходный граф "
                                 "Уоршелла с итоговой матрицей достижимости.\nФайлы содержат окончательные "
                                 "результаты независимо от положения шкалы шагов.\n\n## Дистанции от v%2\n\n| "
                                 "Вершина | Длина | Маршрут |\n|---|---:|---|\n")
                                 .arg(m_variant)
                                 .arg(m_source + 1)
                                 .arg(m_reflexive ? "A* (включает путь длины 0)" : "A+ (пути длины ≥ 1)");

        for (int t = 0; t < 15; ++t) {
            report += QString("| v%1 | %2 | %3 |\n")
                              .arg(t + 1)
                              .arg(formatDistance(dijkstraResult().distance[t]),
                                   formatRoute(algo::route(dijkstraResult().parent, m_source, t)));
        }

        write("RESULTS.md", report.toUtf8());
        m_status = "Результаты сохранены: " + QDir::toNativeSeparators(dir.absolutePath());
        emit statusChanged();

        return true;
    } catch (const std::exception &) {
        m_status = "Не удалось сохранить файлы. Выберите доступную папку и повторите.";
        emit statusChanged();

        return false;
    }
}

void Engine::rebuild() {
    auto &shortestPaths = m_dijkstraCache[m_source];

    if (!shortestPaths) {
        shortestPaths = algo::dijkstra(m_dijkstraGraph, m_source);
    }

    auto &closure = m_warshallCache[m_reflexive ? 1 : 0];

    if (!closure) {
        closure = algo::warshall(m_warshallGraph, m_reflexive);
    }

    m_step = 0;
    emit rowsChanged();
    emit cellsChanged();
    emit changed();
}

const algo::DijkstraResult &Engine::dijkstraResult() const {
    return m_dijkstraCache[m_source].value();
}

const std::vector<algo::WarshallStep> &Engine::warshallSteps() const {
    return m_warshallCache[m_reflexive ? 1 : 0].value();
}

void Engine::loadVariant() {
    m_dijkstraGraph = parseAdjacency(m_data["dijkstra"].toArray().at(m_variant - 1).toArray());
    m_warshallGraph = parseAdjacency(m_data["warshall"].toArray().at(m_variant - 1).toArray());

    for (auto &shortestPaths: m_dijkstraCache) {
        shortestPaths.reset();
    }

    for (auto &closure: m_warshallCache) {
        closure.reset();
    }
}
