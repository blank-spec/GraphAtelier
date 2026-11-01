#include <engine.h>

#include <QCoreApplication>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool condition, const char *message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void checkCachedSolutions(Engine &engine) {
        engine.setMode(0);
        engine.setSource(0);
        engine.finish();
        const auto *shortestPathSnapshot = &engine.dStep();
        const auto *closureSnapshot = &engine.wStep();

        engine.setSource(1);
        engine.setSource(0);
        engine.finish();

        require(&engine.dStep() == shortestPathSnapshot, "Dijkstra cache was not reused");
        require(&engine.wStep() == closureSnapshot, "Changing source recomputed Warshall");

        engine.setReflexive(true);
        engine.setReflexive(false);
        engine.finish();

        require(&engine.wStep() == closureSnapshot, "Warshall cache was not reused");
        require(&engine.dStep() == shortestPathSnapshot, "Changing closure recomputed Dijkstra");
    }

    void checkModelNotifications(Engine &engine) {
        int rowNotifications = 0;
        int cellNotifications = 0;

        const auto rowsConnection = QObject::connect(&engine, &Engine::rowsChanged, [&rowNotifications] {
            ++rowNotifications;
        });

        const auto cellsConnection = QObject::connect(&engine, &Engine::cellsChanged, [&cellNotifications] {
            ++cellNotifications;
        });

        engine.setTarget((engine.target() + 1) % engine.nodeCount());

        require(rowNotifications == 0, "Target selection rebuilt the distance model");
        require(cellNotifications == 0, "Target selection rebuilt the matrix model");

        engine.reset();

        require(rowNotifications == 1, "Dijkstra step did not update distances");
        require(cellNotifications == 0, "Dijkstra step updated the hidden matrix");

        QObject::disconnect(rowsConnection);
        QObject::disconnect(cellsConnection);
    }

    void checkVariantResults(Engine &engine) {
        for (int variant = 1; variant <= 20; ++variant) {
            engine.setVariant(variant);
            engine.setMode(0);

            for (int source: {0, 14, 0}) {
                engine.setSource(source);
                engine.finish();
                const auto reference = algo::dijkstra(engine.matrix(), source);

                require(engine.dStep().distance == reference.distance, "Stale distances after variant/source change");
                require(engine.dStep().parent == reference.parent, "Stale parents after variant/source change");
            }

            engine.setMode(1);

            for (bool reflexive: {false, true, false}) {
                engine.setReflexive(reflexive);
                engine.finish();
                const auto reference = algo::transitiveClosure(engine.matrix(), reflexive);

                require(engine.wStep().reach == reference, "Stale closure after variant/mode change");
                require(engine.rows().size() == 15, "Incorrect distance model size");
                require(engine.cells().size() == 100, "Incorrect reachability model size");
            }
        }
    }
}

int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);

    try {
        Engine engine;
        checkCachedSolutions(engine);
        checkModelNotifications(engine);
        checkVariantResults(engine);

        std::cout << "PASS: solution caches, model notifications, all variants and parameter switches.\n";

        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';

        return 1;
    }
}
