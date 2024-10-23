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

#For CTest
include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#Game app source
add_library(game_lib STATIC
        src/app_util.h
        src/application.h
        src/application.cpp
        src/collision_detector.h
        src/collision_detector.cpp
        src/boost_json.cpp
        src/game_data.h
        src/game_data.cpp
        src/loot_generator.h
        src/loot_generator.cpp
        src/model.h
        src/model.cpp
)

#Server code
add_executable(game_server
        src/http_server.h
        src/http_server.cpp
        src/json_loader.h
        src/json_loader.cpp
        src/main.cpp
        src/request_handling.h
        src/request_handling.cpp
        src/sdk.h
        src/server_logger.h
        src/server_logger.cpp
)

#Unit tests
add_executable(game_server_tests tests/game_server_tests.cpp)
add_executable(collision_detection_tests
        tests/collision-detector-tests.cpp
)

IF (APPLE)
    add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE JSON_PRETTY_PRINT)
    target_compile_options(game_lib PUBLIC -Wno-enum-constexpr-conversion)
ENDIF ()

#Lib
#target_compile_definitions(game_server PRIVATE BOOST_BEAST_USE_STD_STRING_VIEW)
target_include_directories(game_lib PUBLIC CONAN_PKG::boost)
target_link_libraries(game_lib PUBLIC Threads::Threads CONAN_PKG::boost)

#Server
target_link_libraries(game_server game_lib)

#Tests: Catch2 Ctest
catch_discover_tests(game_server_tests)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 game_lib)
target_link_libraries(collision_detection_tests PRIVATE CONAN_PKG::catch2 game_lib)
