#pragma once

#include <pybind11/embed.h>  // 必须包含 Pybind11 的嵌入支持
#include <iostream>

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
        TokenizerNode* temp_ptr = new TokenizerNode(input_que, output_que, model_type);
        node_ = std::unique_ptr<TokenizerNode>(temp_ptr);
    }
    void GetInputData(std::shared_ptr<Queue::QueueData>&) override;
    void PutOutputData(std::shared_ptr<Queue::QueueData>) override;
    
    void Process() override;
};

inline void TokenizerNodeRuntime::GetInputData(std::shared_ptr<Queue::QueueData>& input_data_ptr) {
    Singleton<Queue::QueueManager>::Get().GetQueue(node_->InputName()).Pop(input_data_ptr);
}

inline void TokenizerNodeRuntime::PutOutputData(std::shared_ptr<Queue::QueueData> output_data_ptr) {
    Singleton<Queue::QueueManager>::Get().GetQueue(node_->OutputName()).Push(output_data_ptr);
}

} // namespace Node
} // namespace GlxRouter
