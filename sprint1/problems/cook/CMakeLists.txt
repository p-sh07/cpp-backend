cmake_minimum_required(VERSION 3.11)
set(QT_CREATOR_SKIP_CONAN_SETUP ON)

project(CookBreakfast CXX)
set(CMAKE_CXX_STANDARD 20)

#find_file(CONAN_BUILDINFO conanbuildinfo.cmake)

#if(${CMAKE_BINARY_DIR} MATCHES "Qt")
#    set(CONNAN_FILE_DIR ${CMAKE_BINARY_DIR}/..)
#else if(${CMAKE_BINARY_DIR} MATCHES "cmake-build-debug")
#
#else()
#    set(CONNAN_FILE_DIR ${CMAKE_BINARY_DIR})
#endif()

set(CONNAN_FILE_DIR "${PROJECT_SOURCE_DIR}/build")

message(STATUS "CONAN_FILE_DIR is: ${CONNAN_FILE_DIR}")

include(${CONNAN_FILE_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(cook_breakfast src/main.cpp)
target_link_libraries(cook_breakfast PRIVATE Threads::Threads)
