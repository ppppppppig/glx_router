#include <iostream>
#include <thread>
#include <vector>
#include <torch_npu/csrc/core/npu/NPUStream.h>
#include <acl/acl.h>
#include <torch/torch.h>
#include <c10/util/Exception.h>
#include <torch_npu/csrc/framework/utils/OpPreparation.h>
#include <torch_npu/csrc/aten/NPUNativeFunctions.h>

void threadFunc(int threadId) {
    int32_t devId = 0;
    // aclrtGetDevice(&devId);
    // auto stream = c10_npu::getCurrentNPUStream(devId).stream();
    // std::cout << "Thread " << threadId << ": Current NPU Stream: " << stream << std::endl;
}

int main() {
    const int numThreads = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(threadFunc, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}
