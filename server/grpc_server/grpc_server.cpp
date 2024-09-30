#include "grpc_server.h"


void GrpcStreamServerReceThread::ThreadFun() {
    try{
        while (running_) {
            void * tag;
            bool ok;
            //阻塞100毫秒，gpr_time_from_millis()函数的单位是毫秒,输入的是tag和ok的地址，cq->AsyncNext()会把结果写到地址对应的内存上
            grpc::ServerCompletionQueue::NextStatus status = cq_->AsyncNext(&tag, &ok, gpr_time_from_millis(100, GPR_TIMESPAN));

            if(status ==  grpc::ServerCompletionQueue::NextStatus::GOT_EVENT){
                //grpc服务器有新的事件，强制转换tag从void * 到 std::function<void(bool)> *，即void *(bool) 函数指针
                EventHandler* functionPointer = reinterpret_cast<EventHandler*>(tag);
                //通过函数指针functionPointer调用函数GrpcStreamServerInstance::xxx
                (*functionPointer)(ok);
            }
        }
    } catch(const std::exception& e) {        
        std::cout << e.what() << std::endl;   
    }
}

void GrpcStreamServerReceThread::InitThread() {
    running_ = true;
    thread_ = std::thread(&GrpcStreamServerReceThread::ThreadFun, this);
}

void GrpcStreamServerReceThread::Control() {
    // 只能停止
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}