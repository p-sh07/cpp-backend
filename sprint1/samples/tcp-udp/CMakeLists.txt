cmake_minimum_required(VERSION 3.11)
set(QT_CREATOR_SKIP_CONAN_SETUP ON)

project(TcpUdpServerClient CXX)
set(CMAKE_CXX_STANDARD 20)

#Serch project folder recursively for the conanbuildinfo file
file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo.cmake"  ${PROJECT_SOURCE_DIR})
#message(STATUS ${CONNAN_FILE_PATH})

include(${CONNAN_FILE_PATH})
conan_basic_setup()

# Добавляем четыре проекта
add_executable(TCPServer src/tcp_server.cpp)
add_executable(TCPClient src/tcp_client.cpp)
add_executable(UDPServer src/udp_server.cpp)
add_executable(UDPClient src/udp_client.cpp)

# Просим компоновщик подключить библиотеку для поддержки потоков
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(TCPServer PRIVATE Threads::Threads)
target_link_libraries(TCPClient PRIVATE Threads::Threads)
target_link_libraries(UDPServer PRIVATE Threads::Threads)
target_link_libraries(UDPClient PRIVATE Threads::Threads)

#target_link_libraries(TCPServer PRIVATE Threads::Threads dl)
#target_link_libraries(TCPClient PRIVATE Threads::Threads dl)
#target_link_libraries(UDPServer PRIVATE Threads::Threads dl)
#target_link_libraries(UDPClient PRIVATE Threads::Threads dl)
