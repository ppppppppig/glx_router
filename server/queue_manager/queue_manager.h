#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_queue.h>
#include <cassert>

#include "../utils.h"

namespace GlxRouter {
namespace Queue{

class QueueData {
public:
    QeueueData(size_t stream_id): stream_id_(stream_id) {}

    virtual void* GetData() = 0;

    size_t GetStreamId() {
        return stream_id_;
    }
private:
    size_t stream_id_;
};

class ServerOutputQueueData: QueueData {
public:
    void* GetData() override {
        return static_cast<void*>(&prompts);   
    }

private:
    std::string prompts;
};

using bound_queue = tbb::concurrent_bounded_queue<std::shared_ptr<QueueData>>();

class Queue {
public:
    ResponseTokenQueue() {
        msgQueue_.set_capacity(MAX_QUEUE_SIZE);
    }

    void Pop(std::shared_ptr<QueueData>& msg) {
        queue_.pop(msg);
    }

    // Try to pop from the queue (non-blocking)
    bool TryPop(std::shared_ptr<QueueData>& msg) {
        return queue_.try_pop(msg);
    }

    // Push into the queue
    void Push(std::shared_ptr<QueueData> msg) {
        queue_.push(std::move(msg));
    }

private:
    const int MAX_QUEUE_SIZE = 1 << 12;
    bound_queue queue_;
};

class QueueManger {
public:
    void AddQueue(const std::string& que_name) {
        str_to_que_.emplace(que_name, bound_queue());
    }

    bound_queue GetQue(const std::string& que_name) {
        auto iter = str_to_que_.find(que_name);
        if (iter != str_to_que_.end()) {
            return iter->second;
        }
        assert(1 == 0); // 永远不应该走到这里来
    }

private:
    std::map<std::string, bound_queue> str_to_que_; // 只在初始化时构造该map，后续不会再更新或写，所以使用非并发map即可
};

} // namespace Queue
} // namespace GlxRouter
