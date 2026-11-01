#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <algorithms.h>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>

using algo::Matrix;

void require(bool condition, const char *description) {
    if (!condition) {
        throw std::runtime_error(description);
    }
}

Matrix parseMatrix(const QJsonArray &rows) {
    Matrix result;
    for (const auto &rowValue: rows) {
        std::vector<int> row;
        for (const auto &value: rowValue.toArray()) {
            row.push_back(value.toInt());
        }
        result.push_back(row);
    }
    return result;
}

void checkDijkstra(const Matrix &adjacency) {
    const int count = static_cast<int>(adjacency.size());
    Matrix reference(count, std::vector<int>(count, algo::Inf));

    for (int from = 0; from < count; ++from) {
        for (int to = 0; to < count; ++to) {
            if (from == to) {
                reference[from][to] = 0;
            } else if (adjacency[from][to] > 0) {
                reference[from][to] = adjacency[from][to];
            }
        }
    }
    for (int intermediate = 0; intermediate < count; ++intermediate) {
        for (int from = 0; from < count; ++from) {
            for (int to = 0; to < count; ++to) {
                reference[from][to] =
                        std::min(reference[from][to], reference[from][intermediate] + reference[intermediate][to]);
            }
        }
    }

    for (int source = 0; source < count; ++source) {
        const auto result = algo::dijkstra(adjacency, source);
        require(result.distance == reference[source], "Dijkstra differs from Floyd oracle");

        for (int target = 0; target < count; ++target) {
            const auto path = algo::route(result.parent, source, target);
            if (reference[source][target] == algo::Inf) {
                require(path.empty(), "Unreachable target has a route");
                continue;
            }

            require(!path.empty() && path.front() == source && path.back() == target, "Invalid route endpoints");
            int cost = 0;
            for (size_t index = 1; index < path.size(); ++index) {
                const int weight = adjacency[path[index - 1]][path[index]];
                require(weight > 0, "Route uses a missing edge");
                cost += weight;
            }
            require(cost == result.distance[target], "Route weight differs from distance");
        }

        int previousFixedCount = 0;
        for (const auto &step: result.steps) {
            const int fixedCount = static_cast<int>(std::count(step.fixed.begin(), step.fixed.end(), true));
            require(fixedCount >= previousFixedCount, "Fixed vertices reverted");
            previousFixedCount = fixedCount;
            for (int vertex = 0; vertex < count; ++vertex) {
                if (step.fixed[vertex]) {
                    require(step.distance[vertex] == reference[source][vertex], "Fixed distance is not optimal");
                }
            }
        }
    }
}

void checkWarshall(const Matrix &adjacency) {
    const int count = static_cast<int>(adjacency.size());
    for (bool reflexive: {false, true}) {
        Matrix reference(count, std::vector<int>(count));
        for (int source = 0; source < count; ++source) {
            std::queue<int> pending;
            for (int target = 0; target < count; ++target) {
                if (adjacency[source][target]) {
                    reference[source][target] = 1;
                    pending.push(target);
                }
            }
            while (!pending.empty()) {
                const int current = pending.front();
                pending.pop();
                for (int target = 0; target < count; ++target) {
                    if (adjacency[current][target] && !reference[source][target]) {
                        reference[source][target] = 1;
                        pending.push(target);
                    }
                }
            }
            if (reflexive) {
                reference[source][source] = 1;
            }
        }

        const auto steps = algo::warshall(adjacency, reflexive);
        require(steps.back().reach == reference, "Warshall differs from BFS oracle");
        require(steps.size() == adjacency.size() + 1, "Incorrect number of Warshall snapshots");
        for (size_t step = 1; step < steps.size(); ++step) {
            for (int from = 0; from < count; ++from) {
                for (int to = 0; to < count; ++to) {
                    const bool added = !steps[step - 1].reach[from][to] && steps[step].reach[from][to];
                    require(steps[step].changed[from][to] == added, "Wrong new-cell marker");
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        QFile file(":/data/graphs.json");
        require(file.open(QIODevice::ReadOnly), "Dataset is missing");
        const auto document = QJsonDocument::fromJson(file.readAll()).object();

        for (const auto &variant: document["dijkstra"].toArray()) {
            checkDijkstra(parseMatrix(variant.toArray()));
        }
        for (const auto &variant: document["warshall"].toArray()) {
            checkWarshall(parseMatrix(variant.toArray()));
        }

        std::mt19937 random(18026);
        for (int test = 0; test < 100; ++test) {
            const int count = 1 + static_cast<int>(random() % 12);
            Matrix weighted(count, std::vector<int>(count));
            Matrix binary = weighted;
            for (int from = 0; from < count; ++from) {
                for (int to = 0; to < count; ++to) {
                    if (from != to && random() % 4 == 0) {
                        weighted[from][to] = 1 + static_cast<int>(random() % 20);
                        binary[from][to] = 1;
                    }
                }
            }
            checkDijkstra(weighted);
            checkWarshall(binary);
        }

        checkDijkstra({{0}});
        checkWarshall({{0}});
        checkDijkstra({{0, 4, 1, 0}, {0, 0, 0, 2}, {0, 1, 0, 8}, {0, 0, 0, 0}});
        checkWarshall({{0, 1, 0}, {0, 0, 1}, {0, 0, 0}});
        require(algo::route({1, 0, -1}, 2, 0).empty(), "Cyclic parent array was accepted");

        bool rejected = false;
        try {
            algo::dijkstra({{0, -1}, {0, 0}}, 0);
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require(rejected, "Negative edge was accepted");

        std::cout << "PASS: 40 Excel variants, 100 random graphs, route costs, snapshots, "
                     "disconnected graphs, A+/A*, invalid weights.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
