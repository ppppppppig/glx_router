#include "tokenizer.h"

namespace GlxRouter {
namespace Node{

TokenizerNode::TokenizerNode(const std::string& input_que, const std::string& output_que, const std::string model_type="Llama"):
    Node(input_que, output_que, model_type), guard_()  {
        py::module tokenizer_module = py::module::import("router_llama");
        py::object tokenizer_class = my_python_module.attr("Internlm2Tokenizer");
        tokenizer_instance_ = tokenizer_class();
}

void TokenizerNode::Process(std::shared_ptr<Queue::QueueData> input_data) {
    std::string* prompts = static_cast<std::string*>(input_data->GetData());
    py::object result = tokenizer_instance_.attr("porcess")(*prompts);
    std::vector<int> input_ids = result.cast<std::vector<int>>;
    std::shared_ptr<Queue::QueueData> output_data = std::make_shared<TokenizerNodeOutputData>(input_data->GetStreamId(), input_ids);
    return output_data;
}

void TokenizerNodeRuntime::Process() {
    auto item = GetInputData();
    auto output_data_ptr = node->Process(item->prompt);
    PutOutputData(output_data_ptr);
}

} // namespace Node
} // namespace GlxRouter
