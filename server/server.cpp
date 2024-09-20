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

const static std::string server_address("0.0.0.0:8860");

class GrpcStreamServerInterface {
public:
    virtual void connected(bool ok) = 0; //新连接接入服务器
    virtual void readDone(bool ok) = 0;  //读到一帧新消息
    virtual void writeDone(bool ok) = 0; //写入完成一帧消息到客户端
    virtual void disconnect(bool ok) = 0; //服务器被动断开,不管谁发送的断开指令
};

typedef hello::HelloService::AsyncService AsyncService;

class GrpcStreamServerInstance : public GrpcStreamServerInterface{
private:
   
public:
    GrpcStreamServerInstance(AsyncService* service, grpc::ServerCompletionQueue* inputCq);
    virtual ~GrpcStreamServerInstance(){};
    void connected(bool ok) override;
    void readDone(bool ok) override;
    void writeDone(bool ok) override;
    void disconnect(bool ok) override;
    bool asycSendMsg(HelloMsg& msg);
private:
    std::string streamId;
    AsyncService* service;
    grpc::ServerCompletionQueue* cq;//Completion Queue
    ServerContext serverContext;//每一个stream都有自己的serverContext
    ServerAsyncReaderWriter<HelloMsg, HelloMsg> stream;
    //函数指针
    TagType connectedFunc;//新链接接入时触发
    TagType readDoneFunc;//读到新消息时触发
    TagType writeDoneFunc;//发送一帧消息成功后触发
    TagType disconnectFunc;//stream断开时触发
    //inputMsg用来接收消息，用stream.Read()中绑定
    HelloMsg inputMsg;    
    //onWrite用来区分stream有没有在发送消息，如果stream在发送，则只需要将消息写入writeBuffer，
    //否则要使用stream.Write()触发stream的发送;
    bool onWrite;
    //写缓存
    queue<HelloMsg> writeBuffer; // 只支持单线程写
};

//GrpcStreamServerInstance不需要其它线程交互，故不需要互斥锁
GrpcStreamServerInstance::GrpcStreamServerInstance(AsyncService* inputService,grpc::ServerCompletionQueue* inputCq):\
service(inputService),cq(inputCq),stream(&serverContext)
{
    //使用std::bind绑定对象和类对象函数得到一个函数指针
    connectedFunc = std::bind(&GrpcStreamServerInstance::connected, this, std::placeholders::_1);
    readDoneFunc = std::bind(&GrpcStreamServerInstance::readDone, this, std::placeholders::_1);
    writeDoneFunc = std::bind(&GrpcStreamServerInstance::writeDone, this, std::placeholders::_1);
    disconnectFunc = std::bind(&GrpcStreamServerInstance::disconnect, this, std::placeholders::_1);
    
    //设置serverContext，stream断开时，CompletionQueue会返回一个tag，这个tag就是输入的disconnectFunc这个函数指针
    serverContext.AsyncNotifyWhenDone(&disconnectFunc);
    //设置当新新链接connect的时候，cq返回connectedFunc作为tag
    service->Requesthello(&serverContext, &stream, cq, cq, &connectedFunc);

    onWrite = false;
}

void GrpcStreamServerInstance::connected(bool ok){
    //新建一个GrpcStreamServerInstance，一个client的grpc链接就对应一个GrpcStreamServerInstance实例
    stream.Read(&inputMsg, &readDoneFunc);
    //新的GrpcStreamServerInstance，会在构造函数中调用service->Requesthello()来绑定新链接
    new GrpcStreamServerInstance(service, cq);
    //新加入的stream对应的GrpcStreamServerInstance会被加入到GrpcStreamServerInstanceSet中，用于发送消息或者统计stream链接。

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    streamId = boost::uuids::to_string(uuid);
    singStreamMaps.insert(streamId, this);

    cout << " 当前streamID为: " << streamId << endl;  
}   

void GrpcStreamServerInstance::readDone(bool ok){
    try{
        if(!ok){
            //当ok == false，说明stream已经断开
            return;
        }
        cout << "收到消息1111,id为"<< inputMsg.id() <<",msg为" << inputMsg.msg()  << " 当前消息的streamId为： " << streamId << endl;  
        singResponseTokenQue.push(Task{streamId, inputMsg});
        stream.Read(&inputMsg, &readDoneFunc); // 这是个流式接口，需要再次调用，监听read事件
    }catch(const std::exception& e){
        cout << e.what() << endl;
    }
}

void GrpcStreamServerInstance::writeDone(bool ok){
    if(!ok){
        //当ok == false，说明stream已经断开
        return;
    }
    std::cout << "now writedone" << std::endl;
    onWrite = false;
    if(writeBuffer.empty()) {
        std::cout << "have nothing to write" << std::endl;
        return;
    }
    // 当grpc写完时，会触发writeDone，我们只需要从自定义的writeBuffer中取一帧继续写即可
    stream.Write(std::move(writeBuffer.front()), &writeDoneFunc);
    writeBuffer.pop();
    onWrite = true;
}

void GrpcStreamServerInstance::disconnect(bool ok){ 
    singStreamMaps.erase(streamId);
    // cout << "链接断开,当前链接数量为"<< singStreamMaps.size()<< endl;
    delete this;
}

bool GrpcStreamServerInstance::asycSendMsg(HelloMsg& msg){
    std::cout << "now asycsendmsg" << std::endl;
    writeBuffer.push(msg);
    if(!onWrite){
        //没有任何写操作在执行
        onWrite = true;
        stream.Write(std::move(writeBuffer.front()), &writeDoneFunc);
        writeBuffer.pop();
    }
    return true;
}


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


int main(){    
    GrpcStreamServerReceThread* grpcStreamServerReceThread = new GrpcStreamServerReceThread();
    GrpcStreamServerPushThread* grpcStreamServerPushThread = new GrpcStreamServerPushThread();
    thread receThread(std::bind(&GrpcStreamServerReceThread::run, grpcStreamServerReceThread));
    thread pushThread(std::bind(&GrpcStreamServerPushThread::run, grpcStreamServerPushThread));

    receThread.join();
    pushThread.join();
    std::cout << "server exit" << std::endl;
    return 0;
}

