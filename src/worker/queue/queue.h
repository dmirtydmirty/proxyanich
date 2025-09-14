#pragma once
#include <semaphore.h>
#include <boost/lockfree/queue.hpp>


struct result_t {
    int ret;
    uint64_t u64;
};

struct task_t {
    int (*task_function)(void*);
    void* arg;
    uint64_t u64;
};


struct queue_entry_t {
    uint64_t u64;
    size_t buf_id;
    size_t data_len;
    int return_value;
    uint16_t return_code;
};

template <typename value_type>
class queue {
    public:
    
    queue (size_t size) : m_queue(size) {}
    int init() {
        return sem_init(&m_sem, 0, 0);
    }
    
    int deinit() {
        return sem_destroy(&m_sem); 
    }
    
    bool push(const value_type& val) {
        auto rc = m_queue.push(val);
        if (rc) {
            sem_post(&m_sem);
        }
        return rc;
    }
    
    void pop(value_type& rv ) {
        sem_wait(&m_sem);
        m_queue.pop(rv);
    }
    private:
    boost::lockfree::queue<value_type> m_queue;
    sem_t m_sem{};
};

using submition_queue = queue<queue_entry_t*>;
using complition_queue = queue<queue_entry_t*>;