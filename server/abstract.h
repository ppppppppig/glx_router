#include <vector>

#include "utils.h"

class NodeQueue {
public:

private:
};

struct Req {
public:
    virtual size_t NeedTokens() {
        return max_total_tokens - used_tokens;
    }

    virtual size_t UsedToekns() {
        return used_tokens;
    }

public:
    std::string request_id;
    std::vector<size_t> prompt;
    std::vector<size_t> output_token_list;
    size_t max_req_total_tokens;
    size_t used_tokens;
};


class Batch {
public:
    Batch() {}
    void AddReq(Req* new_req) {
        if (new_req)
    }
    DelReq();
    MergeBatch();
    CanAddNewBatch();
    RemainTokens();

private:
    std::vector<Req*> req_list;
    size_t max_batch_total_tokens;
    size_t max_req_size;
};

class Node {
public:
    virtual void Init();
    virtual void Release();
    virtual void Control(); // 比如需要删除某些
    virtual void Process(Batch* batch);
private:
    std::vector<std::string> inputName;
    std::vector<std::string> outputName;

    std::string name;
    std::string parallel_group; // 同一并行组的Node，将使用同一个线程执行。不同并行组的Node，将使用不同线程执行，并且cuda stream也不一致。
};

class ExecThread {
public:
    std::vector<Node> nodeVec;

};