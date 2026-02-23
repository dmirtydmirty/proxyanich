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

enum class event_type: uint8_t
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
    size_t bid = -1;
    int fd = -1;
    int rv = -1;
    uint32_t flags = 0;
    event_type type = event_type::EMPTY;
};


class event_loop {
public:
    event_loop();

    int init(size_t buffs_size, size_t buffs_cnt);

    void deinit();
    
    int prep_accept(int sock_fd, void (*callback)(event_object& event), uint64_t u64 = 0);
    int perp_recv(int sock_fd, int flags, void (*callback)(event_object& event), uint64_t u64 = 0);
    int prep_send(int sock_fd, size_t buf_id, size_t len, void (*callback)(event_object& event), uint64_t u64 = 0);
    int prep_send(int sock_fd, const char* data, size_t len, void (*callback)(event_object& event), uint64_t u64 = 0);

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