cmake_minimum_required(VERSION 3.0)
project(pybind11_example)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/include)

# 手动添加 gRPC 和 Protobuf 的库路径
# link_directories(/usr/lib/aarch64-linux-gnu/)

# 找到 Python 包
# find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
# find_package(pybind11 REQUIRED)
# include_directories(${pybind11_INCLUDE_DIRS})
# find_package(Boost REQUIRED)

# set(Acl_INCLUDE_DIRS "/usr/local/Ascend/ascend-toolkit/8.0.RC2/runtime/include/")
# set(Python_INCLUDE_DIRS "/usr/include/python3.8/")
# include_directories(${Boost_INCLUDE_DIRS} ${Acl_INCLUDE_DIRS} ${Python_INCLUDE_DIRS})

# 添加grpc_server
add_subdirectory(../server/grpc_server "${CMAKE_BINARY_DIR}/grpc_server_build")
add_executable(test_grpc test_grpc.cpp)
target_link_libraries(test_grpc PRIVATE grpc_server)

# add_library(tokenizer tokenizer.cpp)
# target_link_libraries(tokenizer PRIVATE pybind11::embed Python3::Python)
# target_link_libraries(tokenizer PRIVATE tbb ${Boost_LIBRARIES})

# add_executable(pybind11_example test_tokenizer.cpp)
# target_link_libraries(pybind11_example PRIVATE tokenizer pthread)