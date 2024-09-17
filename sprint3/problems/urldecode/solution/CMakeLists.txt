cmake_minimum_required(VERSION 3.11)

project(urldecode CXX)
set(CMAKE_CXX_STANDARD 20)

#conanbuildinfo_multi.cmake
set(CONAN_DISABLE_CHECK_COMPILER True)

if (APPLE)
    add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE)
    #target_compile_options(game_server PRIVATE -Wno-enum-constexpr-conversion)
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(urldecode
    src/main.cpp
    src/urldecode.h
    src/urldecode.cpp
)

add_executable(tests
    tests/tests.cpp
    src/urldecode.h
    src/urldecode.cpp
)
target_link_libraries(tests PRIVATE CONAN_PKG::boost)
