#include <boost/uuid/uuid.hpp> // 生成 UUID 所需
#include <boost/uuid/uuid_generators.hpp> // 随机生成器
#include <boost/uuid/uuid_io.hpp> // 支持输出 UUID

#include "grpc_server_instance.h"

using namespace GlxRouter::Queue;

//GrpcStreamServerInstance不需要其它线程交互，故不需要互斥锁
GrpcStreamServerInstance::GrpcStreamServerInstance(AsyncService* inputService, std::shared_ptr<grpc::ServerCompletionQueue> inputCq)
: service_(inputService), cq_(inputCq), stream_(&server_context_)
{

    //使用std::bind绑定对象和类对象函数得到一个函数指针
    connected_func_ = std::bind(&GrpcStreamServerInstance::Connected, this, std::placeholders::_1);
    read_done_func_ = std::bind(&GrpcStreamServerInstance::ReadDone, this, std::placeholders::_1);
    write_done_func_ = std::bind(&GrpcStreamServerInstance::WriteDone, this, std::placeholders::_1);
    disconnect_func_ = std::bind(&GrpcStreamServerInstance::Disconnect, this, std::placeholders::_1);
    
    //设置serverContext，stream断开时，CompletionQueue会返回一个tag，这个tag就是输入的disconnectFunc这个函数指针
    server_context_.AsyncNotifyWhenDone(&disconnect_func_);
    //设置当新新链接connect的时候，cq返回connectedFunc作为tag
    service_->Requesthello(&server_context_, &stream_, cq_.get(), cq_.get(), &connected_func_);

    on_write_ = false;
}

void GrpcStreamServerInstance::Connected(bool ok){
    //新建一个GrpcStreamServerInstance，一个client的grpc链接就对应一个GrpcStreamServerInstance实例
    stream_.Read(&input_msg_, &read_done_func_);
    //新的GrpcStreamServerInstance，会在构造函数中调用service->Requesthello()来绑定新链接
    new GrpcStreamServerInstance(service_, cq_);
    //新加入的stream对应的GrpcStreamServerInstance会被加入到GrpcStreamServerInstanceSet中，用于发送消息或者统计stream链接。

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::string stream_id = boost::uuids::to_string(uuid);
    Singleton<StreamManager>::Get().AddStream(stream_id, this);

    std::cout << " 当前streamID为: " << stream_id << std::endl;  
}   

void GrpcStreamServerInstance::ReadDone(bool ok){
    try{
        if(!ok){
            return;
        }
        std::cout << "收到消息1111,id为"<< input_msg_.id() <<",msg为" << input_msg_.msg()  << " 当前消息的streamId为： " << stream_id_ << std::endl;  
        // singResponseTokenQue.push(Task{stream_id_, input_msg_});
        stream_.Read(&input_msg_, &read_done_func_); // 这是个流式接口，需要再次调用，监听read事件
    }catch(const std::exception& e){
        std::cout << e.what() << std::endl;
    }
}

void GrpcStreamServerInstance::WriteDone(bool ok){
    if(!ok){
        return;
    }
    on_write_ = false;
    if(write_buffer_.empty()) {
        std::cout << "have nothing to write" << std::endl;
        return;
    }
    // 当grpc写完时，会触发writeDone，我们只需要从自定义的writeBuffer中取一帧继续写即可
    stream_.Write(std::move(write_buffer_.front()), &write_done_func_);
    write_buffer_.pop();
    on_write_ = true;
}

void GrpcStreamServerInstance::Disconnect(bool ok){ 
    Singleton<StreamManager>::Get().erase(stream_id_);
    delete this;
}

bool GrpcStreamServerInstance::AsycSendMsg(HelloMsg& msg){
    std::cout << "now asycsendmsg" << std::endl;
    write_buffer_.push(msg);
    if(!on_write_){
        //没有任何写操作在执行
        on_write_ = true;
        stream_.Write(std::move(write_buffer_.front()), &write_done_func_);
        write_buffer_.pop();
    }
    return true;
}
