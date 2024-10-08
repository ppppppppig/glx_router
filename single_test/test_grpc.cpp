#include <string>
#include <iostream>

#include "../server/grpc_server/grpc_server.h"
#include "../server/tokenizer/tokenizer.h"

int main() {

    std::string input_que = "input0";
    std::string tokenizer_output_que = "tokenizer_output";
    std::string server_address = "0.0.0.0:3006";

    Singleton<GlxRouter::Queue::QueueManager>::Get().AddQueue(input_que);
    Singleton<GlxRouter::Queue::QueueManager>::Get().AddQueue(tokenizer_output_que);

    GrpcStreamServerReceThread grpc_server(input_que, server_address);
    grpc_server.InitThread();

    GlxRouter::NodeSpace::TokenizerExecThread tokenizer_thread(input_que, tokenizer_output_que, "Internlm2");
    tokenizer_thread.InitThread();

    // GrpcStreamServerPushThread grpc_push_server(tokenizer_output_que);
    // grpc_push_server.InitThread();

    std::this_thread::sleep_for(std::chrono::seconds(300));
    grpc_server.Control();
    std::cout << "bye bye" << std::endl;
}
