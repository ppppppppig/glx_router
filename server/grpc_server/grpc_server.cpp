#include <vector>

#include "grpc_server.h"
#include "../queue_manager/queue_manager.h"
using namespace GlxRouter;

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

void GrpcStreamServerPushThread::ThreadFun() {
    try{
        while (running_) {
            std::shared_ptr<Queue::QueueData> input_data_ptr;
            bool is_get_data = Singleton<Queue::QueueManager>::Get().GetQueue(input_que_name_).TryPop(input_data_ptr);
            if (!is_get_data) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "queue " << input_que_name_ << " has no data." << std::endl;
            } else {
                auto ve_ptr = static_cast<std::vector<int>*>(input_data_ptr->GetData());
                std::cout << "get data: stream_id=" << input_data_ptr->GetStreamId() << " data size is : " << ve_ptr->size() << std::endl;
                for (int i =0; i < ve_ptr->size(); i++) {
                    std::cout << (*ve_ptr)[i] << " ";
                }
                HelloMsg temp;
                temp.set_id(1);
                temp.set_msg("traverse");
                Singleton<Queue::StreamManager>::Get().GetStream(input_data_ptr->GetStreamId())->AsycSendMsg(temp);
                std::cout << "get data" << std::endl;
            }
        }
    } catch(const std::exception& e) {        
        std::cout << e.what() << std::endl;   
    }
}

void GrpcStreamServerPushThread::InitThread() {
    running_ = true;
    thread_ = std::thread(&GrpcStreamServerPushThread::ThreadFun, this);
}

void GrpcStreamServerPushThread::Control() {
    // 只能停止
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}