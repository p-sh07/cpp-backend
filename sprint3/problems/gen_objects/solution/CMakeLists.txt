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

add_executable(game_server
        src/app_util.h
        src/application.h
        src/application.cpp
        src/boost_json.cpp
        src/game_data.cpp
        src/game_data.h
        src/http_server.h
        src/http_server.cpp
        src/json_loader.h
        src/json_loader.cpp
        src/loot_generator.cpp
        src/loot_generator.h
        src/main.cpp
        src/model.h
        src/model.cpp
        src/request_handling.h
        src/request_handling.cpp
        src/sdk.h
        src/server_logger.h
        src/server_logger.cpp
)

IF (APPLE)
    add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE JSON_PRETTY_PRINT)
    target_compile_options(game_server PRIVATE -Wno-enum-constexpr-conversion)
ENDIF ()

#target_compile_definitions(game_server PRIVATE BOOST_BEAST_USE_STD_STRING_VIEW)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)

add_executable(game_server_tests
        src/app_util.h
        src/loot_generator.h
        src/loot_generator.cpp
        src/model.h
        src/model.cpp
        tests/loot_generator_tests.cpp
        tests/model_tests.cpp
)
target_include_directories(game_server_tests PRIVATE  CONAN_PKG::boost)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads)
