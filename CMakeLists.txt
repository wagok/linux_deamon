cmake_minimum_required(VERSION 3.2)
project(linux_deamon)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
find_package (Threads)
set(SOURCE_FILES main.cpp)
add_executable(linux_deamon ${SOURCE_FILES} logger.h logger.cpp config.h config.cpp work.h work.cpp options.h options.cpp monitor.h monitor.cpp)
target_link_libraries (linux_deamon ${CMAKE_THREAD_LIBS_INIT})