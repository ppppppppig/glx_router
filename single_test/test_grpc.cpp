#include <string>
#include <iostream>

#include "../server/grpc_server/grpc_server.h"

int main() {

    std::string input_que = "input0";
    std::string server_address = "0.0.0.0:3006";

    GrpcStreamServerReceThread grpc_server(input_que, server_address);
    grpc_server.InitThread();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    grpc_server.Control();
    std::cout << "bye bye" << std::endl;
}
