#include <algorithms.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace {
    int nearestUnfixedVertex(const algo::DijkstraStep &state) {
        int nearest = -1;

        for (int vertex = 0; vertex < static_cast<int>(state.distance.size()); ++vertex) {
            if (state.fixed[vertex]) {
                continue;
            }

            if (nearest < 0 || state.distance[vertex] < state.distance[nearest]) {
                nearest = vertex;
            }
        }

        return nearest;
    }

    void relaxOutgoingEdges(const algo::Matrix &adjacency, algo::DijkstraStep &state, algo::DijkstraResult &result,
                            bool recordSteps) {
        const int source = state.current;
        const auto &outgoing = adjacency[source];

        for (int target = 0; target < static_cast<int>(outgoing.size()); ++target) {
            const int weight = outgoing[target];

            if (weight == 0 || state.fixed[target]) {
                continue;
            }

            const int candidateDistance = state.distance[source] + weight;

            if (candidateDistance >= state.distance[target]) {
                continue;
            }

            state.distance[target] = candidateDistance;
            state.parent[target] = source;

            if (recordSteps) {
                state.from = source;
                state.to = target;
                state.note = "relax";
                result.steps.push_back(state);
            }
        }
    }

    class PackedReachability {
    public:
        explicit PackedReachability(const algo::Matrix &adjacency, bool reflexive) :
            m_vertexCount(adjacency.size()),
            m_rows(m_vertexCount, std::vector<std::uint64_t>((m_vertexCount + 63) / 64)) {
            for (std::size_t from = 0; from < m_vertexCount; ++from) {
                for (std::size_t to = 0; to < m_vertexCount; ++to) {
                    if (adjacency[from][to] || (reflexive && from == to)) {
                        m_rows[from][to / 64] |= std::uint64_t{1} << (to % 64);
                    }
                }
            }
        }

        void advance(std::size_t intermediate, algo::WarshallStep *snapshot = nullptr) {
            const std::size_t wordIndex = intermediate / 64;
            const std::uint64_t intermediateBit = std::uint64_t{1} << (intermediate % 64);
            const auto &intermediateRow = m_rows[intermediate];

            for (std::size_t from = 0; from < m_vertexCount; ++from) {
                auto &sourceRow = m_rows[from];

                if ((sourceRow[wordIndex] & intermediateBit) == 0) {
                    continue;
                }

                for (std::size_t word = 0; word < sourceRow.size(); ++word) {
                    std::uint64_t added = intermediateRow[word] & ~sourceRow[word];
                    sourceRow[word] |= intermediateRow[word];

                    if (snapshot) {
                        markAddedCells(*snapshot, from, word, added);
                    }
                }
            }
        }

        algo::Matrix matrix() const {
            algo::Matrix result(m_vertexCount, std::vector<int>(m_vertexCount));

            for (std::size_t from = 0; from < m_vertexCount; ++from) {
                for (std::size_t to = 0; to < m_vertexCount; ++to) {
                    result[from][to] = static_cast<int>((m_rows[from][to / 64] >> (to % 64)) & 1);
                }
            }

            return result;
        }

    private:
        static void markAddedCells(algo::WarshallStep &snapshot, std::size_t from, std::size_t word,
                                   std::uint64_t added) {
            // Enumerate only newly discovered pairs; unused tail bits always stay zero.
            while (added != 0) {
                const std::size_t to = word * 64 + std::countr_zero(added);
                snapshot.reach[from][to] = 1;
                snapshot.changed[from][to] = 1;
                added &= added - 1;
            }
        }

        std::size_t m_vertexCount;
        std::vector<std::vector<std::uint64_t>> m_rows;
    };
}

namespace algo {
    void validate(const Matrix &adjacency, bool binary) {
        if (adjacency.empty() || adjacency.size() > 1000) {
            throw std::invalid_argument("Invalid graph size");
        }

        for (const auto &row: adjacency) {
            if (row.size() != adjacency.size()) {
                throw std::invalid_argument("Matrix must be square");
            }

            for (int weight: row) {
                if (weight < 0 || weight >= Inf / static_cast<int>(adjacency.size()) || (binary && weight > 1)) {
                    throw std::invalid_argument("Invalid edge weight");
                }
            }
        }
    }

    DijkstraResult dijkstra(const Matrix &adjacency, int source, TraceMode traceMode) {
        validate(adjacency);
        const int vertexCount = static_cast<int>(adjacency.size());

        if (source < 0 || source >= vertexCount) {
            throw std::out_of_range("Invalid source");
        }

        const bool recordSteps = traceMode == TraceMode::Steps;
        DijkstraStep state;
        state.distance.assign(vertexCount, Inf);
        state.parent.assign(vertexCount, -1);
        state.fixed.assign(vertexCount, false);
        state.distance[source] = 0;

        DijkstraResult result;

        if (recordSteps) {
            result.steps.reserve(1 + 3 * vertexCount);
            state.note = "initial";
            result.steps.push_back(state);
        }

        for (int iteration = 0; iteration < vertexCount; ++iteration) {
            const int nearest = nearestUnfixedVertex(state);

            if (nearest < 0 || state.distance[nearest] == Inf) {
                break;
            }

            state.fixed[nearest] = true;
            state.current = nearest;

            if (recordSteps) {
                state.from = -1;
                state.to = -1;
                state.note = "fix";
                result.steps.push_back(state);
            }

            relaxOutgoingEdges(adjacency, state, result, recordSteps);
        }

        result.distance = std::move(state.distance);
        result.parent = std::move(state.parent);

        return result;
    }

    std::vector<WarshallStep> warshall(const Matrix &adjacency, bool reflexive) {
        validate(adjacency, true);
        const std::size_t vertexCount = adjacency.size();
        PackedReachability reachability(adjacency, reflexive);

        WarshallStep initial;
        initial.reach = reachability.matrix();
        initial.changed = Matrix(vertexCount, std::vector<int>(vertexCount));

        std::vector<WarshallStep> steps;
        steps.reserve(vertexCount + 1);
        steps.push_back(std::move(initial));

        for (std::size_t intermediate = 0; intermediate < vertexCount; ++intermediate) {
            WarshallStep snapshot;
            snapshot.reach = steps.back().reach;
            snapshot.changed = Matrix(vertexCount, std::vector<int>(vertexCount));
            snapshot.k = static_cast<int>(intermediate);

            reachability.advance(intermediate, &snapshot);
            steps.push_back(std::move(snapshot));
        }

        return steps;
    }

    Matrix transitiveClosure(const Matrix &adjacency, bool reflexive) {
        validate(adjacency, true);
        PackedReachability reachability(adjacency, reflexive);

        for (std::size_t intermediate = 0; intermediate < adjacency.size(); ++intermediate) {
            reachability.advance(intermediate);
        }

        return reachability.matrix();
    }

    std::vector<int> route(const std::vector<int> &parent, int source, int target) {
        const int vertexCount = static_cast<int>(parent.size());

        if (source < 0 || target < 0 || source >= vertexCount || target >= vertexCount) {
            return {};
        }

        std::vector<int> path;
        path.reserve(parent.size());
        int current = target;

        while (current >= 0 && current < vertexCount && path.size() < parent.size()) {
            path.push_back(current);

            if (current == source) {
                std::reverse(path.begin(), path.end());
                return path;
            }

            current = parent[current];
        }

        return {};
    }
}
