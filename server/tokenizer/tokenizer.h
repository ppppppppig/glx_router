#pragma once

#include <pybind11/embed.h>  // 必须包含 Pybind11 的嵌入支持
#include <iostream>
#include <thread>

#include "../abstract.h"
#include "../queue_manager/queue_manager.h"

namespace py = pybind11;     // 简化命名空间
namespace GlxRouter {
namespace NodeSpace {

class TokenizerNodeOutputData: public Queue::QueueData {

public:
    TokenizerNodeOutputData(const std::string& stream_id, const std::vector<int>& input_ids)
        : Queue::QueueData(stream_id), input_ids(input_ids) {}

    void* GetData() override {
        return static_cast<void*>(&input_ids);
    }

private:
    std::vector<int> input_ids;

};

class TokenizerNode: public Node {
public:
    TokenizerNode(const std::string& input_que, const std::string& output_que, const std::string& model_type);
    std::shared_ptr<Queue::QueueData> Process(std::shared_ptr<Queue::QueueData> input_data) override;

private:
    py::scoped_interpreter guard_;
    py::object tokenizer_instance_;
};


class TokenizerNodeRuntime: public NodeRuntime {
public:
    TokenizerNodeRuntime(const std::string& input_que, const std::string& output_que, const std::string& model_type) {
        std::cout << "dddd" << std::endl;
        TokenizerNode* temp_ptr = new TokenizerNode(input_que, output_que, model_type);
        std::cout << "ee" << std::endl;
        node_ = std::unique_ptr<TokenizerNode>(temp_ptr);
        std::cout << "ff" << std::endl;
    }
    bool GetInputData(std::shared_ptr<Queue::QueueData>&) override;
    void PutOutputData(std::shared_ptr<Queue::QueueData>) override;
    
    void Process() override;
};

inline bool TokenizerNodeRuntime::GetInputData(std::shared_ptr<Queue::QueueData>& input_data_ptr) {
    bool is_get_data = Singleton<Queue::QueueManager>::Get().GetQueue(node_->InputName()).TryPop(input_data_ptr);
    if (!is_get_data) {
        return false;
    }
    return true;
}

inline void TokenizerNodeRuntime::PutOutputData(std::shared_ptr<Queue::QueueData> output_data_ptr) {
    Singleton<Queue::QueueManager>::Get().GetQueue(node_->OutputName()).Push(output_data_ptr);
}


class TokenizerExecThread: public ExecThread {
public:
    TokenizerExecThread(const std::string& input_que, const std::string& output_que, const std::string& model_type);
    void Control() override; // 停止，启动
    void Process() override;
    void InitThread() override;
private:
    void ThreadFun();
private:
    // aclrtStream *stream_;
    std::thread thread_;
    std::shared_ptr<NodeRuntime> node_runtime_;
    bool running_;
};

inline TokenizerExecThread::TokenizerExecThread(const std::string& input_que, const std::string& output_que, const std::string& model_type)
: node_runtime_(std::make_shared<TokenizerNodeRuntime>(input_que, output_que, model_type)) {
        // aclrtCreateStream(stream_);
    }


} // namespace Node
} // namespace GlxRouter
