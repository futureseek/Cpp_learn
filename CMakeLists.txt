cmake_minimum_required(VERSION 3.29)
project(clion_test)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)




# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Concurrent_Control_Component)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Network_Communication_Component)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Data_Structure_Component)

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Infrastructure_Components)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Database_Component)

add_executable(clion_test main.cpp)

# 链接 mysqlcppconn
target_link_libraries(clion_test PRIVATE mysqlcppconn)


