#pragma once

#include "../abstract.h"
#include "grpc_server_instance.h"

class GrpcStreamServerReceThread: public GlxRouter::ExecThread {
public:
    GrpcStreamServerReceThread(const std::string& input_que, const std::string& server_address): input_que_name_(input_que), server_address_(server_address) {
        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        new GrpcStreamServerInstance(&service_, cq_);
    }
    void Control() override; // 停止，启动
    void Process() override {}
    void InitThread() override;
private:
    void ThreadFun();
private:
    std::string input_que_name_;
    std::thread thread_;
    bool running_;
    
    const std::string server_address_ = "0.0.0.0:3006";
    std::shared_ptr<grpc::ServerCompletionQueue> cq_;
    std::unique_ptr<Server> server_;
    AsyncService service_;
};
