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

add_library(tokenizer ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer.cpp  ${PROTO_SRCS})

include_directories(/root/miniconda3/include/python3.10/)


target_link_libraries(tokenizer PUBLIC pthread /root/miniconda3/lib/libpython3.10.so)
