cmake_minimum_required(VERSION 3.11)
set(QT_CREATOR_SKIP_CONAN_SETUP ON)

project(http_server CXX)
set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

#Serch project folder recursively for the conanbuildinfo file
file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo.cmake"  ${PROJECT_SOURCE_DIR})
#message(STATUS ${CONNAN_FILE_PATH})

include(${CONNAN_FILE_PATH})
conan_basic_setup()

# Добавляем четыре проекта
add_executable(HTTPServer src/main.cpp)

# Просим компоновщик подключить библиотеку для поддержки потоков
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(HTTPServer PRIVATE Threads::Threads)
