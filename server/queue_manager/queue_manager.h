#pragma once

#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <cassert>

#include "../utils.h"

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

private:
    std::map<std::string, Queue> str_to_que_; // 只在初始化时构造该map，后续不会再更新或写，所以使用非并发map即可
};

} // namespace Queue
} // namespace GlxRouter
