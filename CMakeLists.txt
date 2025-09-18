cmake_minimum_required(VERSION 3.29)
project(clion_test)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Data_Structure_Component)

add_executable(clion_test main.cpp Data_Structure_Component/CircularBuffer.h Data_Structure_Component/HashTable.h Data_Structure_Component/SkipList.h Data_Structure_Component/BloomFilter.h)
