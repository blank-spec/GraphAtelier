cmake_minimum_required(VERSION 3.21)
project(GraphAtelier VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Svg)
qt_standard_project_setup(REQUIRES 6.5)

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(GraphAtelier WIN32
    src/main.cpp
    src/algorithms.cpp
    src/algorithms.h
    src/engine.cpp
    src/engine.h
    src/graphitem.cpp
    src/graphitem.h
)

target_include_directories(GraphAtelier PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(GraphAtelier
    URI GraphAtelier
    VERSION 1.0
    QML_FILES
        qml/Theme.qml
        qml/Main.qml
        qml/LabButton.qml
        qml/LineIcon.qml
        qml/Choice.qml
        qml/Mark.qml
)

qt_add_resources(GraphAtelier data
    PREFIX "/"
    FILES
        data/graphs.json
        data/provenance.json
        data/Manrope.ttf
)

target_link_libraries(GraphAtelier PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
)

if(MSVC)
    target_compile_options(GraphAtelier PRIVATE /utf-8 /W4)
else()
    target_compile_options(GraphAtelier PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
option(ATELIER_UI_CAPTURE "Enable development-only UI capture commands" OFF)

if(ATELIER_UI_CAPTURE)
    target_sources(GraphAtelier PRIVATE
        tests/ui_capture.cpp
        tests/ui_capture.h
        tests/ui_benchmark.cpp
        tests/ui_benchmark.h
    )
    target_include_directories(GraphAtelier PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_compile_definitions(GraphAtelier PRIVATE ATELIER_UI_CAPTURE)
endif()

if(BUILD_TESTING)
    qt_add_executable(algorithm_tests
        tests/algorithm_tests.cpp
        src/algorithms.cpp
    )

    qt_add_resources(algorithm_tests test_data
        PREFIX "/"
        FILES data/graphs.json
    )

    target_link_libraries(algorithm_tests PRIVATE Qt6::Core)
    target_include_directories(algorithm_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_test(NAME algorithms COMMAND algorithm_tests)

    add_executable(optimization_tests tests/optimization_tests.cpp src/algorithms.cpp)
    target_include_directories(optimization_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_test(NAME optimizations COMMAND optimization_tests)

    qt_add_executable(engine_tests
        tests/engine_tests.cpp
        src/engine.cpp src/engine.h
        src/graphitem.cpp src/graphitem.h
        src/algorithms.cpp
    )
    qt_add_resources(engine_tests engine_test_data PREFIX "/" FILES data/graphs.json)
    target_link_libraries(engine_tests PRIVATE Qt6::Quick Qt6::Svg)
    target_include_directories(engine_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    if(MSVC)
        target_compile_options(engine_tests PRIVATE /utf-8)
    endif()

    add_test(NAME engine COMMAND engine_tests)

    qt_add_executable(algorithm_benchmark
        tests/algorithm_benchmark.cpp
        src/algorithms.cpp
    )
    qt_add_resources(algorithm_benchmark benchmark_data
        PREFIX "/"
        FILES data/graphs.json
    )
    target_link_libraries(algorithm_benchmark PRIVATE Qt6::Core)
    target_include_directories(algorithm_benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()
