cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)
set(CONAN_DISABLE_CHECK_COMPILER True)

#suppress warnings
add_definitions(-w)

file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo.cmake" ${PROJECT_SOURCE_DIR})

include(${CONNAN_FILE_PATH})
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#Client code
add_executable(game_server_client
        main.cpp
)

IF (APPLE)
    add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE JSON_PRETTY_PRINT)
    target_compile_options(game_server_client PUBLIC -Wno-enum-constexpr-conversion)
ENDIF ()

#game_server_client
target_include_directories(game_server_client PUBLIC CONAN_PKG::boost)
target_link_libraries(game_server_client PUBLIC Threads::Threads CONAN_PKG::boost)
