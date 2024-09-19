cmake_minimum_required(VERSION 3.12)
project(event2dot)

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 20)

include_directories(v0)

add_executable(event2dot_v0_o3
        v0/config.cpp
        v0/config.h
        v0/dotgen.cpp
        v0/dotgen.h
        v0/graph.cpp
        v0/graph.h
        v0/log_duration.h
        v0/main.cpp
        v0/readfile.cpp
        v0/readfile.h)

add_executable(event2dot_v1_o3
        v1/binarytree.cpp
        v1/binarytree.h
        v1/config.cpp
        v1/config.h
        v1/dotgen.cpp
        v1/dotgen.h
        v1/graph.cpp
        v1/graph.h
        v1/log_duration.h
        v1/main.cpp
        v1/readfile.cpp
        v1/readfile.h)

add_executable(event2dot_v2_o3
        v2/binarytree.cpp
        v2/binarytree.h
        v2/config.cpp
        v2/config.h
        v2/dotgen.cpp
        v2/dotgen.h
        v2/graph.cpp
        v2/graph.h
        v2/log_duration.h
        v2/main.cpp
        v2/readfile.cpp
        v2/readfile.h)