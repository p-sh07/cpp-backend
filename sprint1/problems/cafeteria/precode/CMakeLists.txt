cmake_minimum_required(VERSION 3.11)

project(cafeteria CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(cafeteria
	src/main.cpp
	src/cafeteria.h
	src/result.h
	src/hotdog.h
	src/gascooker.h
	src/ingredients.h
	src/clock.h
)
target_link_libraries(cafeteria PRIVATE Threads::Threads)
