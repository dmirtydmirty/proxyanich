#include "worker.h"
#include "tasks/resolve_task.h"

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
    fprintf(stdout, "Worker::run(): worker thread started\n");
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

        queue_entry_t* queue_entry;
        current->m_subQueue->pop(queue_entry);

        resolve_task(queue_entry);

        while (not current->m_comQueue->push(queue_entry)) {
            fprintf(stdout, "push failed repush\n");
        }
        if (current->m_notificator){
            int ret = current->m_notificator();
            fprintf(stdout, "notify ret: %d\n", ret);
            
        }
    }
    return nullptr;
}
