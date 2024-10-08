#pragma once
#include <vector>
#include <memory>

#include "utils.h"
#include "queue_manager/queue_manager.h"

namespace GlxRouter {

// class NodeQueue {
// public:

// private:
// };

// struct Req {
// public:
//     virtual size_t NeedTokens() {
//         return max_total_tokens - used_tokens;
//     }

//     virtual size_t UsedToekns() {
//         return used_tokens;
//     }

// public:
//     std::string request_id;
//     std::vector<size_t> prompt;
//     std::vector<size_t> output_token_list;
//     size_t max_req_total_tokens;
//     size_t used_tokens;
// };


// class Batch {
// public:
//     Batch() {}
//     void AddReq(Req* new_req) {
//         if (new_req)
//     }
//     DelReq();
//     MergeBatch();
//     CanAddNewBatch();
//     RemainTokens();

// private:
//     std::vector<Req*> req_list;
//     size_t max_batch_total_tokens;
//     size_t max_req_size;
// };

class Node {
public:
    Node(const std::string& input_que, const std::string& output_que, const std::string& model_type):
        input_que_(input_que), output_que_(output_que), model_type_(model_type) {}
    virtual void Control() {
        std::cout << "nothing to control" << std::endl;
    }
    virtual std::shared_ptr<Queue::QueueData> Process(std::shared_ptr<Queue::QueueData>) = 0;

    inline std::string InputName() { return input_que_; }
    inline std::string OutputName() { return output_que_; }
private:
    std::string input_que_;
    std::string output_que_;

    std::string model_type_;
    // std::string parallel_group_; // 同一并行组的Node，将使用同一个线程执行。不同并行组的Node，将使用不同线程执行，并且cuda stream也不一致。
};

class NodeRuntime {
public:
    NodeRuntime() {}
    virtual bool GetInputData(std::shared_ptr<Queue::QueueData>&) = 0;
    virtual void PutOutputData(std::shared_ptr<Queue::QueueData>) = 0;
    virtual void Process() = 0;

protected:
    std::unique_ptr<Node> node_;
};


class ExecThread {

public:
    virtual void Control() = 0; // 停止，启动
    virtual void Process() = 0;
    virtual void InitThread() = 0;
};

// class InferRuntime{
// public:
//     virtual void Init();
//     virtual void Release();
//     virtual void Control(); // 停止，启动
//     virtual void Process();
// private:
//     std::list<ExecThread> exec_list_;
// };

} // namespace GlxRouter