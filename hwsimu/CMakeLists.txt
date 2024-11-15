cmake_minimum_required(VERSION 3.0.0)
project(e2e_sim VERSION 0.1.0 LANGUAGES C CXX)

# include(CTest)
# enable_testing()

add_executable(edm src/EDM.cpp)
add_executable(numa src/NUMA.cpp)
add_executable(rdma src/RDMA.cpp)
add_executable(flash src/FLASH.cpp)
add_executable(goodput src/utilization.cpp)
include_directories(include)

add_compile_definitions(DEBUG)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
