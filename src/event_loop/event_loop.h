#pragma once
#include <liburing.h>
#include <unordered_map>
#include <memory>
#include <atomic>

#include "queue/queue.h"

struct event_t
{
    union {
        __u64 u64;
        struct 
        {
            int fd;
            uint8_t type;
        };
    };
};

enum EVENT_TYPE: uint8_t
{
    ACCEPT,
    READ,
    WRITE,
    WORKER,
    TIMEOUT,
};

struct client
{
    int conn_fd;
    unsigned status;
};
 
enum CLIENT_STATUS
{
    NEW,
    RESOLVING,
    AUTHORIZED
};

class event_loop {
public:
    event_loop(std::shared_ptr<complition_queue> cq, std::shared_ptr<submition_queue> sq);

    int init(size_t max_client_cnt);

    void deinit();
    
    int start(int pipe_fd);
    
    int stop();
   
private: 

    int handle_accept();

    std::shared_ptr<complition_queue> m_cq{};
    std::shared_ptr<submition_queue> m_sq{};
    io_uring m_ring{};
    io_uring_buf_ring* m_br{nullptr};
    int m_pipefds[2];
    int m_sock_fd{};
    int m_br_size{};
    std::atomic_bool m_stop{false};
};