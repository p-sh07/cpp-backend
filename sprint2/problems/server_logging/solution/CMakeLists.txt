cmake_minimum_required(VERSION 3.11)

#add_compile_options(Wno-enum-constexpr-conversion)
add_compile_definitions(BOOST_NO_CXX98_FUNCTION_BASE)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

#suppress warnings
add_definitions(-w)

file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo.cmake"  ${PROJECT_SOURCE_DIR})

include(${CONNAN_FILE_PATH})
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
		src/boost_log.cpp
		src/boost_log.h
)
#target_compile_options(game_server PRIVATE -Wno-enum-constexpr-conversion)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
