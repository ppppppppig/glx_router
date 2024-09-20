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
#include <optional>
#include <unistd.h>

// #include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>

#include "../proto_generate/hello.pb.h"
#include "../proto_generate/hello.grpc.pb.h"

using grpc::Channel;
using grpc::ClientAsyncReaderWriter;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using hello::HelloMsg;
using TagType = std::function<void(bool)>;//本人使用的tag是函数指针,函数指针使用的是绑定了类的类对象函数.

using namespace std;

class GrpcStreamClientInstance;
class GrpcStreamClientThread {
public:
    GrpcStreamClientThread(): msgNum(0),isClose(false){};
    ~GrpcStreamClientThread();
    void run();
    void close();
    bool sendMsg(const HelloMsg& msg);
    bool isTimeElapsed(struct timeval now,struct timeval last,int64_t ms);

    std::atomic_bool isClose;
private:
    queue<HelloMsg> msgQueue;
    std::atomic_int msgNum;
    timed_mutex writeLock;    
    GrpcStreamClientInstance* grpcStreamClientInstance;
};

const static std::string server_address("127.0.0.1:8860");
class GrpcStreamClientInterface {
public:
    virtual void connected(bool ok) = 0; //接入服务器成功或者失败
    virtual void readDone(bool ok) = 0;  //读到一帧新消息
    virtual void writeDone(bool ok) = 0; //写入完成一帧消息到服务端
    virtual void disconnect(bool ok) = 0; //链接被断开,不管谁发送的断开指令
};


class GrpcStreamClientInstance : public GrpcStreamClientInterface{
private:
   
public:
    GrpcStreamClientInstance(std::shared_ptr<Channel> channel,grpc::CompletionQueue* inputCq,GrpcStreamClientThread* runThread);
    virtual ~GrpcStreamClientInstance(){};
    void connected(bool ok) override;
    void readDone(bool ok) override;
    void writeDone(bool ok) override;
    void disconnect(bool ok) override;
    bool asycSendMsg(HelloMsg& msg);
    void close(){clientContext.TryCancel();};
    /*  用于标记关闭链接是谁发起的，如果是GrpcStreamClientInstance发起的，在调用GrpcStreamClientThread->close()之前
        haveClose就会为true，否则GrpcStreamClientThread::close()就不是GrpcStreamClientInstance调用的，GrpcStreamClientThread
        需要通知GrpcStreamClientInstance，有人想要关闭stream链接 */
    std::atomic_bool haveClose;
    std::atomic_bool isConnect;//用于记录是否已经连接上服务端
private:
    std::unique_ptr<hello::HelloService::Stub> stub;
    grpc::CompletionQueue* cq;//Completion Queue
    ClientContext clientContext;//每一个stream都有自己的clientContext
    std::unique_ptr<ClientAsyncReaderWriter<HelloMsg, HelloMsg>> stream;
    Status status;
    //函数指针
    TagType connectedFunc;//连接服务端时触发
    TagType readDoneFunc;//读到新消息时触发
    TagType writeDoneFunc;//发送一帧消息成功后触发
    TagType disconnectFunc;//stream断开时触发
    //inputMsg用来接收消息，用stream.Read()中绑定
    HelloMsg inputMsg;    
    //onWrite用来区分stream有没有在发送消息，如果stream在发送，则只需要将消息写入writeBuffer，
    //否则要使用stream.Write()触发stream的发送;
    bool onWrite;
    //写缓存
    queue<HelloMsg> writeBuffer;
    //close时，需要通知GrpcStreamClientThread关闭程序
    GrpcStreamClientThread* threadInstance;
        
};

GrpcStreamClientInstance::GrpcStreamClientInstance(std::shared_ptr<Channel> channel,grpc::CompletionQueue* inputCq,
 GrpcStreamClientThread* runThread):stub(hello::HelloService::NewStub(channel)),cq(inputCq),threadInstance(runThread)
{
    //使用std::bind绑定对象和类对象函数得到一个函数指针
    connectedFunc = std::bind(&GrpcStreamClientInstance::connected, this, std::placeholders::_1);
    readDoneFunc = std::bind(&GrpcStreamClientInstance::readDone, this, std::placeholders::_1);
    writeDoneFunc = std::bind(&GrpcStreamClientInstance::writeDone, this, std::placeholders::_1);
    disconnectFunc = std::bind(&GrpcStreamClientInstance::disconnect, this, std::placeholders::_1);

    //发起grpc连接，不管是成功还是失败，cq.AsyncNext都会返回connectedFunc这个tag，成功时，tag对应的ok为true
    stream = stub->Asynchello(&clientContext, cq,&connectedFunc);

    /*  当在此处设置Finish时，如果服务端不存在,grpc会先返回disconnectFunc的tag，然后再返回connectedFunc，ok为false的tag.
        因为我们正常都是在disconnectFunc里面设置delete this，那么执行connectedFunc时就会报错:pure virtual method called.
        所以等客户端已经连接服务器端再设置Finish或者让GrpcStreamClientThread来delete  GrpcStreamClientInstance都可以解决问题. */
    // stream->Finish(&status,&disconnectFunc);
    onWrite = false;
    isConnect = false;
    haveClose = false;
}
void GrpcStreamClientInstance::connected(bool ok){
    if(ok){
        cout << "连接服务器成功"<< endl;  
        isConnect = true;
        //设置Finish的tag，stream断开时，CompletionQueue会返回一个tag，这个tag就是输入的disconnectFunc这个函数指针
        stream->Finish(&status,&disconnectFunc);
        stream->Read(&inputMsg,&readDoneFunc);        
    }else{
        cout << "Client链接服务端失败"<< endl; 
        haveClose = true;
        threadInstance->close();
    }
}   

void GrpcStreamClientInstance::readDone(bool ok){
    try{
        if(!ok){
            //当ok == false，说明stream已经断开
            return;
        }
        if(inputMsg.msg() == "quit"){
            cout << "客户端被服务端通知需要主动断开链接"<< endl; 
            close();
            return;
        }
        cout << "收到消息,id为"<< inputMsg.id() <<",msg为" << inputMsg.msg() << endl;  
        stream->Read(&inputMsg,&readDoneFunc);
    }catch(const std::exception& e){     
        cout << e.what() << endl;   
    }
}
void GrpcStreamClientInstance::writeDone(bool ok){
    if(!ok){
        //当ok == false，说明stream已经断开
        return;
    }
    onWrite = false;
    if(writeBuffer.empty())return;
    //当grpc写完时，会触发writeDone，我们只需要从自定义的writeBuffer中取一帧继续写即可
    stream->Write(std::move(writeBuffer.front()),&writeDoneFunc);
    writeBuffer.pop();
    onWrite = true;
}

void GrpcStreamClientInstance::disconnect(bool ok){
    cout << "Client已经断开"<< endl; 
    haveClose = true;
    threadInstance->close();   
}
bool GrpcStreamClientInstance::asycSendMsg(HelloMsg& msg){
    writeBuffer.push(msg);
    if(!onWrite){
        //没有任何写操作在执行
        onWrite = true;
        stream->Write(std::move(writeBuffer.front()),&writeDoneFunc);
        writeBuffer.pop();
    }
    return true;
}


bool GrpcStreamClientThread::isTimeElapsed(struct timeval now,struct timeval last,int64_t ms){
    int64_t sub = (now.tv_sec - last.tv_sec)*1000;
    sub  = sub + (now.tv_usec - last.tv_usec)/1000;
    return sub > ms ? true :false;
}

void GrpcStreamClientThread::run(){
    try{
        grpc::CompletionQueue cq;
        std::shared_ptr<Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
        grpcStreamClientInstance = new GrpcStreamClientInstance(channel,&cq,this);

        struct timeval lastTime = {0,0};//用来记录时间，保证下面定时消息的发送
        while (!isClose){
            void * tag;
            bool ok;
            //阻塞100毫秒，gpr_time_from_millis()函数的单位是毫秒,输入的是tag和ok的地址，cq->AsyncNext()会把结果写到地址对应的内存上
            grpc::CompletionQueue::NextStatus status = cq.AsyncNext(&tag, &ok,\
            gpr_time_from_millis(100,GPR_TIMESPAN));

            if(status ==  grpc::CompletionQueue::NextStatus::GOT_EVENT){
                //grpc服务器有新的事件，强制转换tag从void * 到 std::function<void(bool)> *，即void *(bool) 函数指针
                TagType* functionPointer = reinterpret_cast<TagType*>(tag);
                //通过函数指针functionPointer调用函数GrpcStreamServerInstance::xxx
                (*functionPointer)(ok);
            }   

            //从msgQueue中取出新的消息，发送到服务端
            if( msgNum != 0){
                std::unique_lock<timed_mutex>lock(writeLock,std::defer_lock);
                chrono::milliseconds  tryTime(500);
                if(lock.try_lock_for(tryTime)){
                    //获取到了锁
                    while(grpcStreamClientInstance->isConnect && !msgQueue.empty()){
                        HelloMsg msg = msgQueue.front();
                        grpcStreamClientInstance->asycSendMsg(msg);
                        msgQueue.pop();
                    }
                }else{
                    cout << "500ms内没抢到锁" << endl;  
                    continue;
                }
            }
            //定时发送消息
            struct timeval now;
            gettimeofday(&now, NULL);
            if(isTimeElapsed(now,lastTime,10*1000)){//判断是否已经过了10秒
                lastTime = now;
                HelloMsg msg;
                msg.set_id(2);
                msg.set_msg("Hi,i am client");
                if(grpcStreamClientInstance->isConnect)grpcStreamClientInstance->asycSendMsg(msg);

            }
            //此处可以加入一些自定义的处理函数，比如记录时间等等，但是不应该阻塞太久。
        }
        //休眠5秒再退出
        cout << "GrpcStreamClientThread准备退出" << endl;  
        delete grpcStreamClientInstance;
        ::sleep(5);
        
    }catch(const std::exception& e){        
        cout << e.what() << endl;   
    }
}
GrpcStreamClientThread::~GrpcStreamClientThread(){
    
}
void GrpcStreamClientThread::close(){
    if(!grpcStreamClientInstance->haveClose){
        grpcStreamClientInstance->close();
    }else{
        isClose = true;
    }  
}

bool GrpcStreamClientThread::sendMsg(const  HelloMsg& msg){
    {
        std::unique_lock<timed_mutex>lock(writeLock,std::defer_lock);
        chrono::milliseconds  tryTime(500);
        //main函数与GrpcStreamServerThread::run()处于不同的线程，需要加锁保证线程安全
        if(lock.try_lock_for(tryTime)){
            //获取到了锁
            msgQueue.push(msg);
            msgNum++;
        }else{
            cout << "500ms内没抢到锁" << endl;  
            return false;
        }
    }
    return true; 
}

/*

*/
int main(){    
    GrpcStreamClientThread* grpcStreamClientThread = new GrpcStreamClientThread();
    thread myThread(std::bind(&GrpcStreamClientThread::run, grpcStreamClientThread));
    HelloMsg msg;
    msg.set_id(1);

    cin.sync_with_stdio(false);
    std::string input;
    while(!grpcStreamClientThread->isClose){
        if(cin.rdbuf()->in_avail() > 0){
            //使用cin.rdbuf()->in_avail() ，防止输入使用cin的时候阻塞，程序不能退出
            std::getline(std::cin, input);
            //输入exit，跳出循环结束程序。
            if(input == "exit"){
                cout << "客户端主动断开链接并退出"<< endl; 
                grpcStreamClientThread->close();
                break;
            }
            msg.set_msg(input);
            grpcStreamClientThread->sendMsg(msg);
        }
        ::usleep(100*1000);
    }
    myThread.join();
    //退出程序了，就没必要delete grpcStreamClientThread了
    //delete grpcStreamClientThread;
    return 0;
}