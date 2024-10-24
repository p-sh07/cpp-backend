enable_testing()

set(CONAN_DISABLE_CHECK_COMPILER True)
cmake_minimum_required(VERSION 3.11)

add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE)

project(samples CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(application
    src/main.cpp
    src/leap_year.h
    src/leap_year.cpp
)

# Создаём новую цель сборки - исполняемый файл с тестами
add_executable(boost_test_example
    tests/boost_test_main.cpp
    src/leap_year.h
    src/leap_year.cpp
)
# Линкуем тестирующее приложение с Boost, так как Boost.Test требует компоновки
target_link_libraries(boost_test_example PRIVATE CONAN_PKG::boost) 