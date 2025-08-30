#pragma once
#include <pthread.h>
#include <boost/lockfree/queue.hpp>
#include <memory>
#include <atomic>


struct result {
    int ret;
    uint64_t u64;
};

struct task{
    int (*task_function)(void*);
    void* arg;
    uint64_t u64;
};

class Worker {
public:
    using submition_queue = boost::lockfree::queue<task>;
    using complition_queue = boost::lockfree::queue<result>;

    void init(const std::shared_ptr<submition_queue>& subQueue, const std::shared_ptr<complition_queue>& comQueue, const std::function<int()>& notificator = nullptr);
    int run();
    void stop();

private:

    static void* worker_function(void* ptr);
    std::shared_ptr<submition_queue> m_subQueue = nullptr;
    std::shared_ptr<complition_queue> m_comQueue = nullptr;
    std::function<int()> m_notificator;
    pthread_t m_thread;
    std::atomic_bool m_stop = false;
};

