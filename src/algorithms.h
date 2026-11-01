#pragma once
#include <limits>
#include <string>
#include <vector>

namespace algo {
    using Matrix = std::vector<std::vector<int>>;
    constexpr int Inf = std::numeric_limits<int>::max() / 4;

    enum class TraceMode { Steps, ResultOnly };

    struct DijkstraStep {
        std::vector<int> distance;
        std::vector<int> parent;
        std::vector<bool> fixed;

        int current = -1;
        int from = -1;
        int to = -1;
        std::string note;
    };

    struct DijkstraResult {
        std::vector<DijkstraStep> steps;
        std::vector<int> distance;
        std::vector<int> parent;
    };

    struct WarshallStep {
        Matrix reach;
        Matrix changed;
        int k = -1;
    };

    void validate(const Matrix &a, bool binary = false);
    DijkstraResult dijkstra(const Matrix &a, int source, TraceMode traceMode = TraceMode::Steps);
    std::vector<WarshallStep> warshall(const Matrix &a, bool reflexive = false);
    Matrix transitiveClosure(const Matrix &adjacency, bool reflexive = false);
    std::vector<int> route(const std::vector<int> &parent, int source, int target);
}
