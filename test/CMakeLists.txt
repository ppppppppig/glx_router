cmake_minimum_required(VERSION 3.10)
project(NPUStreamDemo)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

include_directories(
    $ENV{PYTHON_INCLUDE_PATH}
    $ENV{PYTORCH_INSTALL_PATH}/include
    $ENV{PYTORCH_INSTALL_PATH}/include/torch/csrc/api/include
    $ENV{PYTORCH_NPU_INSTALL_PATH}/include
    "/usr/local/Ascend/ascend-toolkit/8.0.RC2/runtime/include/"
#    "/root/miniconda3/lib/python3.10/site-packages/torch_npu/include/third_party/acl/inc/"
)
message("/root/miniconda3/include/python3.10")

link_directories(
    $ENV{PYTHON_LIB_PATH}
    $ENV{PYTORCH_INSTALL_PATH}/lib
    $ENV{PYTORCH_NPU_INSTALL_PATH}/lib
    "/root/miniconda3/lib/python3.10/site-packages/torch/lib/"
)

message("    /root/miniconda3/lib/python3.10/")
message("/usr/local/Ascend/ascend-toolkit/latest/lib64")
set(PYTHON_LIBRARIES /root/miniconda3/lib/libpython3.10.so)  # 根据你的实际版本调整

# 添加可执行文件
add_executable(npu_stream_test npu_stream_test.cc)

# 链接库
target_compile_options(npu_stream_test PRIVATE "-Wno-unused-function" "-Wno-unused-variable" "-Wno-unused-parameter" "-Wno-attributes")
target_link_libraries(npu_stream_test  torch pthread  shm torch torch_global_deps torch_cpu torch_python torch_npu /root/miniconda3/lib/python3.10/site-packages/torch.libs/libgfortran-b6d57c85.so.5.0.0 "/root/miniconda3/lib/python3.10/site-packages/torch/lib/libc10.so")

# target_link_libraries(npu_stream_test torch c10 torch_cpu torch_python torch_npu pthread ${PYTHON_LIBRARIES})
