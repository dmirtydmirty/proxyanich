#include <semaphore.h>
#include <boost/lockfree/queue.hpp>

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