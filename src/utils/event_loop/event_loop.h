#pragma once
#include <liburing.h>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <set>

// #include "queue/queue.h"
#include "indexed_list/indexed_list.h"
#include "recv_buffers/recv_buffers.h"

class event_loop;

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

    EMPTY = UINT8_MAX 
};

struct event_object
{
    void (*callback)(event_object& event);
    event_loop* el = nullptr;
    uint64_t u64 = -1;
    int fd = -1;
    int rv = -1;
    uint32_t flags = 0;
    event_type type = EMPTY;
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
    event_loop();

    int init(size_t buffs_size, size_t buffs_cnt);

    void deinit();
    
    int prep_accept(int sock_fd, void (*callback)(event_object& event));
    int perp_recv(int sock_fd, void (*callback)(event_object& event));
    int prep_send(int sock_fd, size_t buf_id, size_t len, void (*callback)(event_object& event));
    int prep_send(int sock_fd, const char* data, size_t len, void (*callback)(event_object& event));
    void free_buf(size_t buf_id);

    int process();

    int start(int pipe_fd);
    
    int stop();
   
    int handle_accept(int32_t event_result);
    int handle_read(int32_t event_result, int conn_sock_fd, int used_buf_id);
    int handle_worker_event();

    indexed_list<event_object> events;

    io_uring m_ring{};
    io_uring_buf_ring* m_br{nullptr};
    int m_br_size{};
    recv_buffers_t buffs;
};