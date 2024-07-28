cmake_minimum_required(VERSION 3.11)

project(Radio CXX)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE CONNAN_FILE_PATH "conanbuildinfo.cmake"  ${PROJECT_SOURCE_DIR})

include(${CONNAN_FILE_PATH})
conan_basic_setup()

# Ищем Boost версии 1.78
find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Под Windows нужно определить макрос NOMINMAX для корректной работы при включении
# библиотеки minisound
if(WIN32)
  add_definitions(-DNOMINMAX)
endif()

add_executable(radio src/main.cpp src/audio.h)
target_link_libraries(radio PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

#For MacOS AudioUnit, CoreAudio [and possibly AudioToolbox, compiles without]
#if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(APPLE)
    target_link_libraries(radio PRIVATE "-framework CoreFoundation" "-framework AudioUnit" "-framework CoreAudio")
endif()

#Needed or not? - nope
#set_target_properties(radio PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")

#example of another way
#find_library(AUDIO_UNIT AudioUnit)
#if (NOT AUDIO_UNIT)
#    message(FATAL_ERROR "AudioUnit not found")
#endif()
