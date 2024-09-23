#include <pybind11/embed.h>  // 必须包含 Pybind11 的嵌入支持
#include <iostream>

#include "../abstract.h"
#include "../queue_manager/queue_manager.h"

namespace py = pybind11;     // 简化命名空间
namespace GlxRouter {
namespace Node {

class TokenizerNodeOutputData: public QueueData {

public:
    void* GetData() override {
        return static_cast<void*>(&input_ids);
    }

private:
    std::vector<int> input_ids;

};

class TokenizerNode: public Node {
public:
    TokenizerNode(const std::string& input_que, const std::string& output_que, const std::string model_type="Llama");
    std::shared_ptr<Queue::QueueData> Process(std::shared_ptr<Queue::QueueData> input_data) override;

private:
    py::scoped_interpreter guard_;
    py::object tokenizer_instance_;
};


class TokenizerNodeRuntime: public NodeRuntime {
public:
    void Init(const std::string& input_que, const std::string& output_que, const std::string& model_type) override;
    void GetInputData(std::shared_ptr<Queue::QueueData>&) override;
    void PutOutputData(std::shared_ptr<Queue::QueueData>&) override;
    
    void Process() override;
};

inline void TokenizerNodeRuntime::Init(const std::string& input_que, const std::string& output_que, const std::string& model_type) {
    node_ = make_shared<TokenizerNode>(input_que, output_que, model_type);
}

inline std::string TokenizerNodeRuntime::GetInputData(std::shared_ptr<Queue::QueueData>& input_data_ptr) {
    Singleton<QueueManger>->GetQueue(node->InputNames()).pop(input_data_ptr);
}

inline void TokenizerNodeRuntime::PutOutputData(std::shared_ptr<Queue::QueueData>& output_data_ptr) {
    Singleton<QueueManger>->GetQueue(node->OutputNames()).push(output_data_ptr);
}

} // namespace Node
} // namespace GlxRouter
