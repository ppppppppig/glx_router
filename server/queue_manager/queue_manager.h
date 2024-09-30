#pragma once

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <cassert>
#include <iostream>
#include <thread>
#include <map>

#include "../utils.h"

class GrpcStreamServerInstance;

namespace GlxRouter {
namespace Queue{

class QueueData {
public:
    QueueData(const std::string& stream_id): stream_id_(stream_id) {}

    virtual void* GetData() = 0;

    const std::string& GetStreamId() {
        return stream_id_;
    }
private:
    std::string stream_id_;
};

class ServerOutputQueueData: public QueueData {
public:
    ServerOutputQueueData(const std::string& stream_id, const std::string& prompts)
    : QueueData(stream_id), prompts_(prompts) {}
    void* GetData() override {
        return static_cast<void*>(&prompts_);   
    }

private:
    std::string prompts_;
};



using bound_queue = tbb::concurrent_bounded_queue<std::shared_ptr<QueueData>>;

class Queue {
public:
    Queue() {
        queue_.set_capacity(MAX_QUEUE_SIZE);
    }

    inline void Pop(std::shared_ptr<QueueData>& msg) {
        std::cout << "queue size" << Size() << std::endl;
        // std::cout << "New thread ID: " << std::this_thread::get_id() << std::endl;
        queue_.pop(msg);
    }

    // Try to pop from the queue (non-blocking)
    inline bool TryPop(std::shared_ptr<QueueData>& msg) {
        return queue_.try_pop(msg);
    }

    inline size_t Size() {
        return queue_.size();
    }

    // Push into the queue
    inline void Push(std::shared_ptr<QueueData> msg) {
        queue_.push(msg);
    }

private:
    const int MAX_QUEUE_SIZE = 1 << 12;
    bound_queue queue_;
};

class QueueManager {
public:
    void AddQueue(const std::string& que_name) {
        str_to_que_.emplace(que_name, Queue());
    }

    Queue& GetQueue(const std::string& que_name) {
        auto iter = str_to_que_.find(que_name);
        if (iter != str_to_que_.end()) {
            return iter->second;
        }
        assert(1 == 0); // 永远不应该走到这里来
    }

    void erase(const std::string& que_name) {
        str_to_que_.erase(que_name);
    }

private:
    std::map<std::string, Queue> str_to_que_; // 只在初始化时构造该map，后续不会再更新或写，所以使用非并发map即可
};

class StreamManager {
public:
    StreamManager(): stream_maps_() {}

    void AddStream(const std::string& stream_id, GrpcStreamServerInstance* stream) {
        stream_maps_.emplace(stream_id, stream);
    }

    GrpcStreamServerInstance* GetStream(const std::string& stream_id) {
        auto iter = stream_maps_.find(stream_id);
        if (iter != stream_maps_.end()) {
            return iter->second;
        }
        assert(1 == 0); // 永远不应该走到这里来
    }

    void erase(const std::string& stream_id) {
        stream_maps_.erase(stream_id);
    }

private:
    std::map<std::string, GrpcStreamServerInstance*> stream_maps_;
};

} // namespace Queue
} // namespace GlxRouter
