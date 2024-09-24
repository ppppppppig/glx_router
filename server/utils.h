#pragma once
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <boost/uuid/uuid.hpp> // 生成 UUID 所需
#include <boost/uuid/uuid_generators.hpp> // 随机生成器
#include <boost/uuid/uuid_io.hpp> // 支持输出 UUID

#include "../proto_generate/hello.pb.h"
#include "../proto_generate/hello.grpc.pb.h"

using hello::HelloService;
using hello::HelloMsg;
using boost::uuids::uuid;

class GrpcStreamServerInstance;
class StreamMaps {
public:
    StreamMaps(): stream_maps_(), accessor_() {}

    // 如果key已经存在，返回false
    bool insert(const std::string& key, GrpcStreamServerInstance* value) {
        if (stream_maps_.insert(accessor_, key)) {
            accessor_->second = value;
        } else {
            return false;
        }
        return true;
    }

    bool update(const std::string& key, GrpcStreamServerInstance* value) {
        if (stream_maps_.find(accessor_, key)) {
            accessor_->second = value;
        } else {
            return false;
        }
        return true;
    }

    bool sendMsg(const std::string& key, HelloMsg& msg);

    bool erase(const std::string& key) {
        if (stream_maps_.find(accessor_, key)) {
            stream_maps_.erase(accessor_);
            return true;
        }
        return false;
    }

private:
    tbb::concurrent_hash_map<std::string, GrpcStreamServerInstance *> stream_maps_;
    tbb::concurrent_hash_map<std::string, GrpcStreamServerInstance *>::accessor accessor_;
};


struct Task {
    std::string streamId;
    HelloMsg helloMsg;
};

class ResponseTokenQueue {
public:
    ResponseTokenQueue() {
        msgQueue_.set_capacity(MAX_QUEUE_SIZE);
    }

    void pop(Task& msg) {
        msgQueue_.pop(msg);
    }

    bool try_pop(Task& msg){
        if (msgQueue_.try_pop(msg)) {
            return true;
        } 
        return false;
    }

    void push(Task msg) {
        msgQueue_.push(msg);
    }

private:
    const int MAX_QUEUE_SIZE = 1 << 12;
    tbb::concurrent_bounded_queue<Task> msgQueue_;
};

template <typename T>
class Singleton {
public:
    static T& Get() {
        static T instance; // 声明一个静态实例，确保只创建一次
        return instance;
    }

    // 删除构造函数、拷贝构造函数和赋值操作符
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    Singleton() {} // 私有构造函数，防止直接实例化
};
