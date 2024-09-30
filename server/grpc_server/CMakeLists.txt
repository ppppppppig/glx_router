cmake_minimum_required(VERSION 3.5)

include_directories(/usr/local/include)

# 手动添加 gRPC 和 Protobuf 的库路径
link_directories(/usr/lib/aarch64-linux-gnu/)
find_package(Protobuf REQUIRED)

set(CMAKE_CXX_STANDARD 11)


set(PROTO_SRCS
    /root/glx/glx_router/proto_generate/hello.pb.cc
    /root/glx/glx_router/proto_generate/hello.grpc.pb.cc
)

add_library(grpc_server ${CMAKE_CURRENT_SOURCE_DIR}/grpc_server.cpp ${CMAKE_CURRENT_SOURCE_DIR}/grpc_server_instance.cpp ${PROTO_SRCS})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(grpc_server grpc++ grpc++_reflection grpc /lib/aarch64-linux-gnu/libgpr.so  tbb protobuf::libprotobuf ${Boost_LIBRARIES})
