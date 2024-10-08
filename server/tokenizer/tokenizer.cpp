#include <cassert>
#include <pthread.h>

#include "tokenizer.h"
namespace GlxRouter {
namespace NodeSpace {

TokenizerNode::TokenizerNode(const std::string& input_que, const std::string& output_que, const std::string& model_type):
    Node(input_que, output_que, model_type), guard_()  {
        try {
            py::module sys = py::module::import("sys");
            sys.attr("path").attr("append")("/root/glx/glx_router/server/tokenizer");  // 添加上一级目录
            py::module tokenizer_module = py::module::import("router_llama");
            py::object tokenizer_class = tokenizer_module.attr("Internlm2Tokenizer");
            tokenizer_instance_ = tokenizer_class();
        } catch (const py::error_already_set& e) {
            std::cerr << "Error importing module: " << e.what() << std::endl;
            assert(1==0);
        }
}

std::shared_ptr<Queue::QueueData> TokenizerNode::Process(std::shared_ptr<Queue::QueueData> input_data) {
    std::string* prompts = static_cast<std::string*>(input_data->GetData());
    py::object result = tokenizer_instance_.attr("Process")(*prompts);
    std::vector<int> input_ids;
    input_ids.reserve(1000);
    try {
        if (py::isinstance<py::list>(result)) {
            py::list result_list = result.cast<py::list>();
            for (size_t i = 0; i < result_list.size(); ++i) {
                input_ids.push_back(result_list[i].cast<int>());
            }
        }
        // 使用 input_ids
    } catch (const py::cast_error& e) {
        std::cerr << "Error casting result to vector<int>: " << e.what() << std::endl;
        assert(1==0);
    }
    std::shared_ptr<Queue::QueueData> output_data = std::make_shared<TokenizerNodeOutputData>(input_data->GetStreamId(), input_ids);
    return output_data;
}

void TokenizerNodeRuntime::Process() {
    std::shared_ptr<Queue::QueueData> input_data_ptr;
    bool is_get_data = GetInputData(input_data_ptr);
    if (!is_get_data) {
        return;
    }
    auto output_data_ptr = node_->Process(input_data_ptr);
    PutOutputData(output_data_ptr);
}

void TokenizerExecThread::Control() {
    // 只能停止
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TokenizerExecThread::Process() {
    if (!running_) {
        return;
    }
    node_runtime_->Process();
}

void TokenizerExecThread::ThreadFun() {
    pid_t lwpId = syscall(SYS_gettid); // 获取线程 ID
    std::cout << "Thread LWP ID: " << lwpId << std::endl;
    while (running_) {
        Process();
    }
    running_ = false;
}

void TokenizerExecThread::InitThread() {
    running_ = true;
    thread_ = std::thread(&TokenizerExecThread::ThreadFun, this);

}

} // namespace NodeSpace
} // namespace GlxRouter
