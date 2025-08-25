cmake_minimum_required(VERSION 3.29)
project(clion_test)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)

add_executable(clion_test main.cpp Singleton.h Observer.h Decorator.h)
