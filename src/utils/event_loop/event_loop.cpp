#include <sys/socket.h>
#include <netinet/in.h>

#include "event_loop.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


#define READ_BGID 0 

event_loop::event_loop()
{}

int event_loop::init(size_t buffs_size, size_t buffs_cnt )
{
    events.init(buffs_cnt * 10);

    if (buffs.alloc(buffs_cnt, buffs_size) != 0) {

        spdlog::critical("event_loop::init() allocation recv buffer failed");
        return -1;
    }

    m_br_size = buffs_cnt;

    io_uring_params params{};

    if (int ret = io_uring_queue_init_params(m_br_size, &m_ring, &params); ret == -1)
    {
        spdlog::critical("event_loop::init() io_uring_queue_init error: {}", strerror(-ret));
        return -1;
    }
    int page_size = getpagesize();
    spdlog::debug("buffer ring will be page aligned: {} bytes", page_size);
    if ( int ret = posix_memalign((void**)&m_br, page_size, m_br_size * sizeof(io_uring_buf)); ret != 0){
        spdlog::critical("event_loop::init() posix_memalign m_br error: {}", strerror(ret));
        return -1;
    }

    io_uring_buf_ring_init(m_br);

    for (size_t i = 0; i < m_br_size; ++i) {
        int mask = io_uring_buf_ring_mask(m_br_size);
        io_uring_buf_ring_add(m_br, buffs[i], buffs_size, i, mask, i);
    }

    io_uring_buf_ring_advance(m_br, m_br_size);

    io_uring_buf_reg reg = {
        .ring_addr = (__u64)m_br,
        .ring_entries = (uint32_t)m_br_size,
        .bgid = READ_BGID,

    };
    
    if (int ret = io_uring_register_buf_ring(&m_ring, &reg, 0); ret != 0)  {
        spdlog::critical("event_loop::init() io_uring_register_buf_ring error: {}", strerror(-ret));
        return -1;
    }
    spdlog::debug("event loop initialized");
    return 0;
}


void event_loop::deinit() {
    free(m_br);
}

int event_loop::start(int pipe_fd)
{
    return -1;
}

int event_loop::stop()
{
    return -1;
}

int event_loop::handle_accept(int32_t event_result)
{
    return -1;
}

int event_loop::handle_read(int32_t event_result, int conn_sock_fd, int used_buf_id)
{
    return -1;
}

int event_loop::handle_worker_event()
{
    return -1;
}

int event_loop::prep_accept(int sock_fd, void (*callback)(event_object &event), uint64_t u64 )
{   
    event_object event{.callback = callback, .el = this, .u64 = u64, .fd = sock_fd, .type = event_type::ACCEPT};
    
    auto* sqe = io_uring_get_sqe(&m_ring);
    auto idx = events.allocate(event);

    io_uring_prep_multishot_accept(sqe, sock_fd, nullptr, 0, 0);
    io_uring_sqe_set_data(sqe, (void*)idx);
    io_uring_submit(&m_ring);

    return 0;
}

int event_loop::perp_recv(int sock_fd, int flags, void (*callback)(event_object &event), uint64_t u64 )
{
    event_object event{.callback = callback, .el = this, .u64 = u64, .fd = sock_fd, .type = event_type::READ};
    auto idx = events.allocate(event);

    auto* sqe = io_uring_get_sqe(&m_ring);
    io_uring_prep_recv(sqe, sock_fd, NULL, 4096, flags);
    io_uring_sqe_set_data(sqe, (void*)idx);
    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = READ_BGID;
    io_uring_submit(&m_ring);
    return 0;
}

int event_loop::prep_send(int sock_fd, size_t buf_id, size_t len, void (*callback)(event_object &event), uint64_t u64)
{
    event_object event{.callback = callback, .el = this, .u64 = u64, .bid = buf_id, .fd = sock_fd, .type = event_type::WRITE};
    auto idx = events.allocate(event);

    auto* sqe = io_uring_get_sqe(&m_ring);
    char* data = buffs[buf_id];
    io_uring_prep_send(sqe, sock_fd, data, len, 0);
    io_uring_sqe_set_data(sqe, (void*)idx);
    io_uring_submit(&m_ring);
    return 0;
}

int event_loop::prep_send(int sock_fd, const char *data, size_t len, void (*callback)(event_object &event), uint64_t u64)
{
    event_object event{.callback = callback, .el = this, .u64 = u64, .fd = sock_fd, .type = event_type::WRITE};
    auto idx = events.allocate(event);

    auto* sqe = io_uring_get_sqe(&m_ring);
    io_uring_prep_send(sqe, sock_fd, data, len, 0);
    io_uring_sqe_set_data(sqe, (void*)idx);
    io_uring_submit(&m_ring);
    return 0;
}

void event_loop::free_buf(size_t buf_id)
{
    io_uring_buf_ring_add(m_br, buffs[buf_id], buffs.m_bufs_len, buf_id,
                    io_uring_buf_ring_mask(m_br_size), 0);
	io_uring_buf_ring_advance(m_br, 1);
}

int event_loop::process()
{   
    io_uring_cqe* cqe;

    __kernel_timespec ts{.tv_sec = 0, .tv_nsec = 50'000ull};
    if (int ret = io_uring_wait_cqes(&m_ring, &cqe, 1, &ts, nullptr); ret != 0) {
        // spdlog::error("{}: io_uring_wait_cqes error {}", __func__, strerror(-ret));
        // return -1;
    }

    unsigned head;
    unsigned cqe_cnt{};
    io_uring_for_each_cqe(&m_ring, head, cqe)
    {

        auto idx = (__u64)io_uring_cqe_get_data(cqe);

        auto* event = events[idx];
        event->rv = cqe->res;
        event->flags = cqe->flags;

        if (event->callback) {
            event->callback(*event);
        }
        cqe_cnt++;
    }

    io_uring_cq_advance(&m_ring, cqe_cnt);

    return 0;
}
