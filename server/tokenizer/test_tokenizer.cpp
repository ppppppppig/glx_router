#include "tokenizer.h"

int main() {
    TokenizerNodeRuntime node;
    std::string input_que_name = "input0";
    std::string output_que_name = "output0"

    Singleton<QueueManager>::Get().AddQueue(input_que_name);
    Singleton<QueueManager>::Get().AddQueue(output_que_name);

    std::string mock_stream_id = "123";
    std::shared_ptr<QueueData> input_data = std::make_shared<ServerOutputQueueData>(mock_stream_id, "hello, world, I Like China!")
    Singleton<QueueManager>::Get().GetQueue(input_que_name).Push(input_data);
    node->Init(input_que_name, output_que_name);
    node->Process();
    std::shared_ptr<QueueData> output_data;
    Singleton<QueueManager>::Get().GetQueue(output_que_name).Pop(output_data);
    
    std::vector<int>* input_ids = static_cast<std::vector<int>*>(output_data->GetData());
    std::cout << "input_ids: ";
    for (const auto& val: *input_ids) {
        std::cout << val << std::endl;
    }
    return 0;
}