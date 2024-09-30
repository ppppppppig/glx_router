#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <queue>
#include <unordered_set>
#include <mutex>
#include <time.h>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <unistd.h>
#include <boost/uuid/uuid.hpp> // 生成 UUID 所需
#include <boost/uuid/uuid_generators.hpp> // 随机生成器
#include <boost/uuid/uuid_io.hpp> // 支持输出 UUID
#include <iostream>

#include "../proto_generate/hello.pb.h"
#include "../proto_generate/hello.grpc.pb.h"
#include "utils.h"

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

using TagType = std::function<void(bool)>;//本人使用的tag是函数指针,函数指针使用的是绑定了类的类对象函数.

using namespace std;


StreamMaps& singStreamMaps = Singleton<StreamMaps>::Get();
ResponseTokenQueue& singResponseTokenQue = Singleton<ResponseTokenQueue>::Get();

typedef hello::HelloService::AsyncService AsyncService;



bool StreamMaps::sendMsg(const std::string& key, HelloMsg& msg) {
        if (stream_maps_.find(accessor_, key)) {
            accessor_->second->asycSendMsg(msg);
        } else {
            return false;
        }
        return true;
    }



class GrpcStreamServerReceThread {
public:

    void run();

};

void GrpcStreamServerReceThread::run(){
    try{
        std::unique_ptr<grpc::ServerCompletionQueue> cq;
        AsyncService service;
        ServerBuilder builder;

        // builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 10000);
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        cq = builder.AddCompletionQueue();
        std::unique_ptr<Server> server_ = builder.BuildAndStart();
        new GrpcStreamServerInstance(&service, cq.get());

        while (true) {
            void * tag;
            bool ok;
            //阻塞100毫秒，gpr_time_from_millis()函数的单位是毫秒,输入的是tag和ok的地址，cq->AsyncNext()会把结果写到地址对应的内存上
            grpc::ServerCompletionQueue::NextStatus status = cq->AsyncNext(&tag, &ok,\
            gpr_time_from_millis(100, GPR_TIMESPAN));

            if(status ==  grpc::ServerCompletionQueue::NextStatus::GOT_EVENT){
                //grpc服务器有新的事件，强制转换tag从void * 到 std::function<void(bool)> *，即void *(bool) 函数指针
                TagType* functionPointer = reinterpret_cast<TagType*>(tag);
                //通过函数指针functionPointer调用函数GrpcStreamServerInstance::xxx
                (*functionPointer)(ok);
            }
        }
    } catch(const std::exception& e) {        
        cout << e.what() << endl;   
    }
}


class GrpcStreamServerPushThread {
public:
    void run() {
        Task responseToken;
        while(1) {
            singResponseTokenQue.pop(responseToken);
            singStreamMaps.sendMsg(responseToken.streamId, responseToken.helloMsg);
        }
    }

};


void AddNeededQueue(const std::vector<std::string>& ve) {
    for (const auto& str: ve) {
        Singleton<QueueManager>::Get().AddQueue(str);
    }
}

int main() {

    std::vector<std::string> ve{'input0', 'output0'};
    AddNeededQueue(ve);

    GrpcStreamServerReceThread* grpcStreamServerReceThread = new GrpcStreamServerReceThread(ve[0]);
    GrpcStreamServerPushThread* grpcStreamServerPushThread = new GrpcStreamServerPushThread(ve[1]);
    thread receThread(std::bind(&GrpcStreamServerReceThread::run, grpcStreamServerReceThread));
    thread pushThread(std::bind(&GrpcStreamServerPushThread::run, grpcStreamServerPushThread));

    receThread.join();
    pushThread.join();
    std::cout << "server exit" << std::endl;
    return 0;
}

