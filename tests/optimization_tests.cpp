#include <algorithms.h>

#include <iostream>
#include <random>
#include <stdexcept>

namespace {
    void require(bool condition, const char *message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    algo::Matrix makeGraph(int vertexCount, std::mt19937 &random) {
        algo::Matrix adjacency(vertexCount, std::vector<int>(vertexCount));

        for (int from = 0; from < vertexCount; ++from) {
            for (int to = 0; to < vertexCount; ++to) {
                if (random() % 13 == 0) {
                    adjacency[from][to] = 1;
                }
            }
        }

        return adjacency;
    }

    void advanceReference(algo::WarshallStep &state, int intermediate) {
        const int vertexCount = static_cast<int>(state.reach.size());
        state.changed = algo::Matrix(vertexCount, std::vector<int>(vertexCount));
        state.k = intermediate;

        for (int from = 0; from < vertexCount; ++from) {
            for (int to = 0; to < vertexCount; ++to) {
                const bool throughIntermediate = state.reach[from][intermediate] && state.reach[intermediate][to];

                if (!state.reach[from][to] && throughIntermediate) {
                    state.reach[from][to] = 1;
                    state.changed[from][to] = 1;
                }
            }
        }
    }

    void checkWarshallSnapshots(const algo::Matrix &adjacency, bool reflexive) {
        const auto steps = algo::warshall(adjacency, reflexive);
        const int vertexCount = static_cast<int>(adjacency.size());

        algo::WarshallStep reference;
        reference.reach = adjacency;
        reference.changed = algo::Matrix(vertexCount, std::vector<int>(vertexCount));

        if (reflexive) {
            for (int vertex = 0; vertex < vertexCount; ++vertex) {
                reference.reach[vertex][vertex] = 1;
            }
        }

        require(steps.front().reach == reference.reach, "Initial reachability differs");
        require(steps.front().changed == reference.changed, "Initial change markers differ");

        for (int intermediate = 0; intermediate < vertexCount; ++intermediate) {
            advanceReference(reference, intermediate);
            const auto &snapshot = steps[intermediate + 1];

            require(snapshot.reach == reference.reach, "Intermediate Warshall reachability differs");
            require(snapshot.changed == reference.changed, "Intermediate change markers differ");
            require(snapshot.k == reference.k, "Intermediate vertex differs");
        }

        require(algo::transitiveClosure(adjacency, reflexive) == reference.reach, "Fast closure differs");
    }

    void checkResultOnlyDijkstra(const algo::Matrix &binary) {
        algo::Matrix weighted = binary;

        for (int from = 0; from < static_cast<int>(weighted.size()); ++from) {
            for (int to = 0; to < static_cast<int>(weighted.size()); ++to) {
                weighted[from][to] *= 1 + (from * 7 + to * 11) % 97;
            }
        }

        for (int source = 0; source < static_cast<int>(weighted.size()); ++source) {
            const auto trace = algo::dijkstra(weighted, source);
            const auto fast = algo::dijkstra(weighted, source, algo::TraceMode::ResultOnly);

            require(fast.steps.empty(), "Result-only mode recorded steps");
            require(fast.distance == trace.distance, "Fast distances differ");
            require(fast.parent == trace.parent, "Fast parent choices differ");
        }
    }

    void checkLargeClosure() {
        constexpr int vertexCount = 1000;
        algo::Matrix chain(vertexCount, std::vector<int>(vertexCount));

        for (int vertex = 0; vertex + 1 < vertexCount; ++vertex) {
            chain[vertex][vertex + 1] = 1;
        }

        for (bool reflexive: {false, true}) {
            const auto closure = algo::transitiveClosure(chain, reflexive);

            for (int from = 0; from < vertexCount; ++from) {
                for (int to = 0; to < vertexCount; ++to) {
                    const bool expected = from < to || (reflexive && from == to);

                    require(closure[from][to] == expected, "Large chain closure differs");
                }
            }
        }
    }
}

int main() {
    try {
        std::mt19937 random(18026);

        for (int vertexCount: {1, 2, 10, 63, 64, 65, 127, 128, 129}) {
            const auto adjacency = makeGraph(vertexCount, random);
            checkWarshallSnapshots(adjacency, false);
            checkWarshallSnapshots(adjacency, true);

            if (vertexCount <= 65) {
                checkResultOnlyDijkstra(adjacency);
            }
        }

        checkWarshallSnapshots({{0, 1, 0}, {0, 0, 1}, {0, 0, 0}}, false);
        checkWarshallSnapshots({{0, 1, 0}, {0, 0, 1}, {1, 0, 0}}, false);
        checkLargeClosure();

        std::cout << "PASS: exact Warshall snapshots, 64-bit boundaries, fast solvers, 1000-vertex chain.\n";

        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';

        return 1;
    }
}
