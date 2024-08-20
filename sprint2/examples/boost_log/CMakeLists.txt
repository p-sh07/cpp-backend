cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

#suppress warnings
add_definitions(-w)
file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo.cmake"  ${PROJECT_SOURCE_DIR})
include(${CONNAN_FILE_PATH})

#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello_log main.cpp)

target_include_directories(hello_log PRIVATE CONAN_PKG::boost)
target_link_libraries(hello_log PRIVATE Threads::Threads CONAN_PKG::boost)

# Boost.Beast будет использовать std::string_view вместо boost::string_view
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)
