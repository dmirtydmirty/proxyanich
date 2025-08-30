#include "worker.h"

void Worker::init(const std::shared_ptr<submition_queue> &subQueue, const std::shared_ptr<complition_queue> &comQueue, const std::function<int()>& notificator)
{
    m_subQueue = subQueue;
    m_comQueue = comQueue;
    m_notificator = notificator;
}

int Worker::run()
{
    m_stop = false;
    if (int ret = pthread_create(&m_thread, nullptr, worker_function, this)) {
        return 0; 
    } else {
        return -1;
    }

}

void Worker::stop()
{
    m_stop = true;
}

void* Worker::worker_function(void* ptr)
{
    auto current = reinterpret_cast<Worker*>(ptr);
    while(not current->m_stop){
        if (current->m_subQueue->empty()) {
            usleep(2);
        } else {
            submition_queue::value_type task;
            current->m_subQueue->pop(task);

            complition_queue::value_type res{
                .ret = task.task_function(task.arg), 
                .u64 = task.u64
            };
            while (not current->m_comQueue->push(res)) {
                fprintf(stdout, "push failed repush\n");
            }
            if (current->m_notificator){
                int ret = current->m_notificator();
                fprintf(stdout, "notify ret: %d\n", ret);
            }
        }
    }
    return nullptr;
}
