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
        src/main.cpp
        src/http_server.cpp
        src/http_server.h
        src/sdk.h
        src/model.h
        src/model.cpp
        src/app_util.h
        src/boost_json.cpp
        src/json_loader.h
        src/json_loader.cpp
        src/request_handling.cpp
        src/request_handling.h
        src/server_logger.cpp
        src/server_logger.h
        src/application.cpp
        src/application.h
)

IF (APPLE)
    add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE)
    target_compile_options(game_server PRIVATE -Wno-enum-constexpr-conversion)
ENDIF ()

target_compile_definitions(game_server PRIVATE BOOST_BEAST_USE_STD_STRING_VIEW)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
