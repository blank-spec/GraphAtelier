#include <algorithms.h>

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    using Clock = std::chrono::steady_clock;
    volatile long long measurementSink = 0;

    algo::Matrix parseMatrix(const QJsonArray &rows) {
        algo::Matrix adjacency;
        adjacency.reserve(rows.size());

        for (const auto &rowValue: rows) {
            std::vector<int> row;

            for (const auto &weight: rowValue.toArray()) {
                row.push_back(weight.toInt());
            }

            adjacency.push_back(std::move(row));
        }

        return adjacency;
    }

    template<class Operation>
    double measureBatch(Operation &operation, int repetitions) {
        const auto start = Clock::now();

        for (int iteration = 0; iteration < repetitions; ++iteration) {
            measurementSink = operation();
        }

        const auto elapsed = Clock::now() - start;
        const double microseconds = std::chrono::duration<double, std::micro>(elapsed).count();

        return microseconds / repetitions;
    }

    template<class Operation>
    void measure(const char *name, Operation operation) {
        for (int iteration = 0; iteration < 200; ++iteration) {
            measurementSink = operation();
        }

        int repetitions = 1;

        while (measureBatch(operation, repetitions) * repetitions < 30'000 && repetitions < 1'048'576) {
            repetitions *= 2;
        }

        std::vector<double> samples;
        samples.reserve(31);

        for (int batch = 0; batch < 31; ++batch) {
            samples.push_back(measureBatch(operation, repetitions));
        }

        std::sort(samples.begin(), samples.end());

        std::cout << name << ',' << repetitions << ",31," << samples[15] << ',' << samples[29] << '\n';
    }

    void measureShortestPaths(const algo::Matrix &adjacency) {
        measure("dijkstra_one_source_steps", [&adjacency] {
            const auto result = algo::dijkstra(adjacency, 0);

            return static_cast<long long>(result.distance[13]) + result.steps.size();
        });

        measure("dijkstra_all_sources_steps", [&adjacency] {
            long long checksum = 0;

            for (int source = 0; source < 15; ++source) {
                const auto result = algo::dijkstra(adjacency, source);
                checksum += result.distance[source] + result.steps.size();
            }

            return checksum;
        });

        measure("dijkstra_all_sources_result_only", [&adjacency] {
            long long checksum = 0;

            for (int source = 0; source < 15; ++source) {
                const auto result = algo::dijkstra(adjacency, source, algo::TraceMode::ResultOnly);
                checksum += result.distance[(source + 1) % 15];
            }

            return checksum;
        });
    }

    void measureReachability(const algo::Matrix &adjacency) {
        measure("warshall_steps", [&adjacency] {
            const auto steps = algo::warshall(adjacency);

            return static_cast<long long>(steps.back().reach[0][9]) + steps.size();
        });

        measure("warshall_result_only", [&adjacency] {
            const auto closure = algo::transitiveClosure(adjacency);

            return closure[0][9];
        });
    }
}

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);

    try {
        QFile input(":/data/graphs.json");

        if (!input.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Cannot read benchmark dataset");
        }

        const auto datasets = QJsonDocument::fromJson(input.readAll()).object();
        const auto weighted = parseMatrix(datasets["dijkstra"].toArray().at(17).toArray());
        const auto binary = parseMatrix(datasets["warshall"].toArray().at(17).toArray());

        if (algo::dijkstra(weighted, 0).distance[13] != 13) {
            throw std::runtime_error("Unexpected Dijkstra checksum");
        }

        const auto closure = algo::transitiveClosure(binary);
        int reachablePairs = 0;

        for (const auto &row: closure) {
            for (int reachable: row) {
                reachablePairs += reachable;
            }
        }

        if (reachablePairs != 100) {
            throw std::runtime_error("Unexpected Warshall checksum");
        }

        std::cout << "workload,repetitions_per_batch,batches,median_us,p95_batch_mean_us\n";
        std::cout << std::fixed << std::setprecision(4);

        measureShortestPaths(weighted);
        measureReachability(binary);

        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
