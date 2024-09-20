#include "utils.h"
#include "abstract.h"

class RouterNode: public Node {
public:
    void run() override {
        while(true) {
            if (batch->ShouldAddTask()) {
                newBatch = ConstructNewBatch();
                FusionForward(batch, newBatch);
                batch->Merge(newBatch);
                // 从队列中获取新的Task
            }
            BatchForward(batch);
        }
    }

    Task get() override {

    }

    void put(Task* ) override {

    }

private:
    // ContinousBatching batch;
    // std::shared_ptr<Batch> batch;
};