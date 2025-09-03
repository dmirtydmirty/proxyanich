#include "worker.h"

void Worker::init(const std::shared_ptr<submition_queue> &subQueue, const std::shared_ptr<complition_queue> &comQueue, const std::function<int()>& notificator)
{
    m_subQueue = subQueue;
    m_comQueue = comQueue;
    m_notificator = notificator;
}

int Worker::run()
{
    if (int ret = pthread_create(&m_thread, nullptr, worker_function, this); ret != 0) {
        fprintf(stderr, "Worker::run(): pthread_create() %s", strerror(ret));
        return -1;
    }
    m_stop = false;
    return 0; 

}

void Worker::stop()
{
    m_stop = true;
}

void* Worker::worker_function(void* ptr)
{
    auto current = reinterpret_cast<Worker*>(ptr);
    while(not current->m_stop){

        task_t task;
        current->m_subQueue->pop(task);

        result_t result{
            .ret = task.task_function(task.arg), 
            .u64 = task.u64
        };
        while (not current->m_comQueue->push(result)) {
            fprintf(stdout, "push failed repush\n");
        }
        if (current->m_notificator){
            int ret = current->m_notificator();
            fprintf(stdout, "notify ret: %d\n", ret);
            
        }
    }
    return nullptr;
}
