cmake_minimum_required(VERSION 3.11)
project(book_manager CXX)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-w)
set(CONAN_DISABLE_CHECK_COMPILER True)

file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo_multi.cmake" ${PROJECT_SOURCE_DIR})

include(${CONNAN_FILE_PATH})
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(book_manager
        boost_json.cpp
        main.cpp)

IF (APPLE)
    add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE JSON_PRETTY_PRINT)
    target_compile_options(book_manager PUBLIC -Wno-enum-constexpr-conversion)
ENDIF ()

target_link_libraries(book_manager PRIVATE CONAN_PKG::libpqxx CONAN_PKG::boost)