cmake_minimum_required(VERSION 3.5)

include_directories(/usr/local/include)

# 手动添加 gRPC 和 Protobuf 的库路径
link_directories(/usr/lib/aarch64-linux-gnu/)
find_package(Protobuf REQUIRED)

set(CMAKE_CXX_STANDARD 11)

set(PROTO_SRCS
    ${CMAKE_SOURCE_DIR}/proto_generate/hello.pb.cc
    ${CMAKE_SOURCE_DIR}/proto_generate/hello.grpc.pb.cc
)

add_executable(server ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp ${PROTO_SRCS})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(server grpc++ grpc++_reflection grpc /lib/aarch64-linux-gnu/libgpr.so  tbb protobuf::libprotobuf ${Boost_LIBRARIES})
