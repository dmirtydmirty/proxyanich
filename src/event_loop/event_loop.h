#pragma once
#include <liburing.h>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <set>

#include "queue/queue.h"
#include "indexed_list.h"

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

enum event_type: uint8_t
{
    ACCEPT,
    READ,
    WRITE,
    WORKER,
    TIMEOUT,
    RECV,
    SEND,
};

struct event_object
{
    void (*callback)(event_object& event);
    uint64_t u64;
    size_t buff_idx;
    int rv;
    int fd;
    event_type type;
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

    int handle_accept(int32_t event_result);
    int handle_read(int32_t event_result, int conn_sock_fd, int used_buf_id);
    int handle_worker_event();

    // int prep_accept(int sock_fd,  void (*callback)(event_object& event));
    // int perp_read()
    int prep_event(int sock_fd,  void (*callback)(event_object& event));
    std::set<int> m_active_connections{};
    std::unordered_map<int, int> m_forwarding_rules{};

    std::shared_ptr<complition_queue> m_cq{};
    std::shared_ptr<submition_queue> m_sq{};
    io_uring m_ring{};
    io_uring_buf_ring* m_br{nullptr};
    int m_pipefds[2];
    int m_sock_fd{};
    int m_br_size{};
    std::atomic_bool m_stop{false};
};