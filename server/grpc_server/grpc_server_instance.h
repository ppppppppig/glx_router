#pragma once

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <thread>
#include <queue>

#include "../../proto_generate/hello.pb.h"
#include "../../proto_generate/hello.grpc.pb.h"
#include "../utils.h"
#include "../queue_manager/queue_manager.h"

using EventHandler = std::function<void(bool)>;

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

using hello::HelloMsg;

typedef hello::HelloService::AsyncService AsyncService;

// gaolingxiao: 可能存在内存泄露问题
class GrpcStreamServerInstance {
private:
   
public:
    GrpcStreamServerInstance(AsyncService* service, std::shared_ptr<grpc::ServerCompletionQueue> inputCq, const std::string& output_que);
    ~GrpcStreamServerInstance(){};
    void Connected(bool ok);
    void ReadDone(bool ok);
    void WriteDone(bool ok);
    void Disconnect(bool ok);
    bool AsycSendMsg(HelloMsg& msg);
private:
    std::string stream_id_;                                         // 由uuid生成的stream_id
    AsyncService* service_;                     
    std::shared_ptr<grpc::ServerCompletionQueue> cq_;               // Completion Queue
    ServerContext server_context_;                                  // 每一个stream都有自己的serverContext
    ServerAsyncReaderWriter<HelloMsg, HelloMsg> stream_;            // 函数指针
    EventHandler connected_func_;                                   // 新链接接入时触发
    EventHandler read_done_func_;                                   // 读到新消息时触发
    EventHandler write_done_func_;                                  // 发送一帧消息成功后触发
    EventHandler disconnect_func_;                                  // stream断开时触发
    HelloMsg input_msg_;                                            // inputMsg用来接收消息，用stream.Read()中绑定
    bool on_write_;                                                 // 判断是否要添加一个写事件
    std::queue<HelloMsg> write_buffer_;      
    std::string output_que_name_;                
};
