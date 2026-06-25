cmake_minimum_required(VERSION 3.12)
project(fuel_planner CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Source files
set(SOURCES
    src/graph.cpp
    src/dijkstra.cpp
    src/tradeoff_analyzer.cpp
    src/json_exporter.cpp
    src/tests.cpp
    src/cli.cpp
    src/main.cpp
)

# Headers
set(HEADERS
    src/state.h
    src/graph.h
    src/fuel_manager.h
    src/input_validator.h
    src/pareto.h
    src/dijkstra.h
    src/tradeoff_analyzer.h
    src/json_exporter.h
    src/tests.h
    src/cli.h
)

add_executable(fuel_planner ${SOURCES} ${HEADERS})

target_include_directories(fuel_planner PRIVATE src)
