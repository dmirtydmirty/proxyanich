#pragma once
#include <pthread.h>
#include <memory>
#include <atomic>

#include "queue/queue.h"



class Worker {
public:

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

