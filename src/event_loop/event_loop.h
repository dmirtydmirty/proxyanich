#pragma once
#include <liburing.h>
#include <unordered_map>
#include <memory>
#include <atomic>

#include "queue/queue.h"

struct event_t
{
    int fd;
    unsigned type;
};

enum EVENT_TYPE
{
    ACCEPT,
    READ,
    WRITE,
    WORK,
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

    event_loop(std::shared_ptr<complition_queue> cq, std::shared_ptr<submition_queue> sq);

    int init(size_t max_client_cnt);

    void deinit();
    
    int start();
    
    int stop();
   
private:    
    std::unordered_map<int, int> m_forwarding_rules{};
    std::shared_ptr<complition_queue> m_cq{};
    std::shared_ptr<submition_queue> m_sq{};
    io_uring m_ring{};
    uint8_t* m_buffs{nullptr};
    io_uring_buf_ring* m_br{nullptr};
    int m_pipefds[2];
    int m_sock_fd{};
    std::atomic_bool m_stop{false};
};