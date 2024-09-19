cmake_minimum_required(VERSION 3.11)

project(urlencode CXX)
set(CMAKE_CXX_STANDARD 20)

set(CONAN_DISABLE_CHECK_COMPILER True)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

if(APPLE)

endif()

add_executable(urlencode
    src/main.cpp
    src/urlencode.h
    src/urlencode.cpp
)

add_executable(tests
    tests/tests.cpp
    src/urlencode.h
    src/urlencode.cpp
)
target_link_libraries(tests PRIVATE CONAN_PKG::gtest)
