#include "tokenizer.h"

using GlxRouter::Queue::QueueManager;
using GlxRouter::Queue::QueueData;
using GlxRouter::Queue::ServerOutputQueueData;
using GlxRouter::NodeSpace::TokenizerNodeRuntime;
using namespace GlxRouter::NodeSpace;

int main() {
    std::string input_que_name = "input0";
    std::string output_que_name = "output0";
    std::cout << 0 << std::endl;
    std::cout << 0.5 << std::endl;
    std::cout << 1 << std::endl;
    Singleton<QueueManager>::Get().AddQueue(input_que_name);
    Singleton<QueueManager>::Get().AddQueue(output_que_name);
    TokenizerExecThread tokenizer_exec_thread(input_que_name, output_que_name, "Internlm2");
    tokenizer_exec_thread.InitThread();
    std::string mock_stream_id = "123";
    std::shared_ptr<QueueData> input_data = std::make_shared<ServerOutputQueueData>(mock_stream_id, "hello, world, I Like China!");
    std::cout << 2 << std::endl;
    Singleton<QueueManager>::Get().GetQueue(input_que_name).Push(input_data);
    // tokenizer_exec_thread.Process();
    std::shared_ptr<QueueData> output_data;
    Singleton<QueueManager>::Get().GetQueue(output_que_name).Pop(output_data);
    std::cout << 3 << std::endl;
    std::vector<int>* input_ids = static_cast<std::vector<int>*>(output_data->GetData());
    std::cout << "input_ids: " ;
    for (const auto& val: *input_ids) {
        std::cout << val << ' ';
    }
    tokenizer_exec_thread.Control();
    return 0;
}
