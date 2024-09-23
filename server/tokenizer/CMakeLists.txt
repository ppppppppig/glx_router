cmake_minimum_required(VERSION 3.0)
project(pybind11_example)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/include)

# 手动添加 gRPC 和 Protobuf 的库路径
link_directories(/usr/lib/aarch64-linux-gnu/)

# 找到 Python 包
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
add_library(tokenizer tokenizer.cpp)
target_link_libraries(tokenizer PRIVATE pybind11::embed Python3::Python)
target_link_libraries(tokenizer  tbb  ${Boost_LIBRARIES})

add_executable(pybind11_example test_tokenizer.cpp)
target_link_libraries(pybind11_example PRIVATE tokenizer)