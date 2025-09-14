#include <sys/socket.h>
#include <netinet/in.h>

#include "event_loop.h"
#include "http_status_codes.h"
#include "http_status_text.h"
#include "recv_buffers.h"

#define READ_BGID 0 
recv_buffers_t recv_buffers;

event_loop::event_loop(std::shared_ptr<complition_queue> cq, std::shared_ptr<submition_queue> sq) :
    m_cq(cq),
    m_sq(sq)
{}

int event_loop::init(size_t max_client_cnt)
{

    if (recv_buffers.alloc(MAX_HTTP_HEADER_LEN, max_client_cnt) != 0) {
        fprintf(stderr, "event_loop::init() allocation recv buffer failed\n");
        return -1;
    }

    m_br_size = max_client_cnt;
    m_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (m_sock_fd == -1)
    {
        perror("event_loop::init() creation socket error\n");
        return -1;
    }

    const int optval = 1;
    if (setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("event_loop::init() setsockopt error\n");
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(15001);

    if (bind(m_sock_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    {
        perror("event_loop::init() binding socket error\n");
        return -1;
    }

    listen(m_sock_fd, m_br_size);

    io_uring_params params{};

    if (int ret = io_uring_queue_init_params(m_br_size, &m_ring, &params); ret == -1)
    {
        fprintf(stderr, "event_loop::init() io_uring_queue_init error, %s\n", strerror(-ret));
        return -1;
    }
    int page_size = getpagesize();
    if ( int ret = posix_memalign((void**)&m_br, page_size, m_br_size * sizeof(io_uring_buf)); ret != 0){
        fprintf(stderr, "event_loop::init() posix_memalign m_br error, %s\n", strerror(ret));
        return -1;
    }

    io_uring_buf_ring_init(m_br);

    for (size_t i = 0; i < m_br_size; ++i) {
        int mask = io_uring_buf_ring_mask(m_br_size);
        io_uring_buf_ring_add(m_br, recv_buffers[i], MAX_HTTP_HEADER_LEN, i, mask, i);
    }

    io_uring_buf_ring_advance(m_br, m_br_size);

    io_uring_buf_reg reg = {
        .ring_addr = (__u64)m_br,
        .ring_entries = (uint32_t)m_br_size,
        .bgid = READ_BGID,

    };
    
    if (int ret = io_uring_register_buf_ring(&m_ring, &reg, 0); ret != 0)  {
        fprintf(stderr, "event_loop::init() io_uring_register_buf_ring error %s\n", strerror(-ret));
        return -1;
    }

    return 0;
}


void event_loop::deinit() {
    free(m_br);
}

int event_loop::start(int pipe_fd)
{
    char trash[1];
    event_t accept_event{};
    accept_event.fd = m_sock_fd;
    accept_event.type = ACCEPT;
    
    auto* sqe = io_uring_get_sqe(&m_ring);

    io_uring_prep_multishot_accept(sqe, m_sock_fd, nullptr, 0, 0);
    io_uring_sqe_set_data(sqe, (void*)accept_event.u64);

    event_t worker_event{};
    worker_event.fd = pipe_fd;
    worker_event.type = WORKER;
    
    sqe = io_uring_get_sqe(&m_ring);

    io_uring_prep_read(sqe, pipe_fd, trash, 1, 0);
    io_uring_sqe_set_data(sqe, (void*)worker_event.u64);
    io_uring_submit(&m_ring);
    
    fprintf(stdout, "starting event loop\n");
    while (not m_stop)
    {
        io_uring_cqe* cqe;
        if (int ret = io_uring_wait_cqes(&m_ring, &cqe, 1, nullptr, nullptr); ret != 0) {
            fprintf(stderr, "event_loop::start() io_uring_wait_cqes error %s", strerror(-ret));
            continue;
        }

        unsigned head;
        unsigned cqe_cnt{};
        io_uring_for_each_cqe(&m_ring, head, cqe)
        {
            event_t event; 
            event.u64 = (__u64)io_uring_cqe_get_data(cqe);
            int32_t event_result = cqe->res;

            switch (event.type)
            {
            case ACCEPT: {

                handle_accept(event_result);
                break;
            }
            case READ: {
                int used_buf_id = cqe->flags >> IORING_CQE_BUFFER_SHIFT;

                handle_read(event_result, event.fd, used_buf_id);
                break;
            }
            case WORKER: {
                handle_worker_event();
                sqe = io_uring_get_sqe(&m_ring);

                io_uring_prep_read(sqe, pipe_fd, trash, 1, 0);
                io_uring_sqe_set_data(sqe, (void*)worker_event.u64);
                io_uring_submit(&m_ring);
                break;
            }
            case WRITE: {
                int conn_sock_fd = event.fd;
                if (not m_active_connections.contains(conn_sock_fd)) {
                    fprintf(stdout, "Closing connection with %d\n", conn_sock_fd);
                    close(conn_sock_fd);
                }
            }
            default:
                break;
            }
            cqe_cnt++;
        }

        io_uring_cq_advance(&m_ring, cqe_cnt);
    }
    

    return 0;
}

int event_loop::stop()
{
    return 0;
}

int event_loop::handle_accept(int32_t event_result)
{
    if (event_result == -1)
    {
        fprintf(stdout, "acception failed %s", strerror(-event_result));
    } else 
    {
        const int conn_sock_fd = event_result;
        m_active_connections.insert(conn_sock_fd);
        fprintf(stdout, "connection created %d\n", conn_sock_fd);
    
        event_t read_event{};
        read_event.fd = conn_sock_fd;
        read_event.type = READ;
    
        auto* sqe = io_uring_get_sqe(&m_ring);
        io_uring_prep_read(sqe, conn_sock_fd, NULL, 4096, 0);
        io_uring_sqe_set_data(sqe, (void*)read_event.u64);
        sqe->flags |= IOSQE_BUFFER_SELECT;
        sqe->buf_group = READ_BGID;
        io_uring_submit(&m_ring);
    
        // sqe = io_uring_get_sqe(&m_ring);
        // io_uring_prep_timeout(sqe, new __kernel_timespec{5, 0}, 1, 0);
        // sqe->flags |= IOSQE_IO_LINK;
        
        // event_t timeout_event{.fd = conn_sock_fd, .type = TIMEOUT};
    }

    return 0;
}

int event_loop::handle_read(int32_t event_result, int conn_sock_fd, int used_buf_id)
{
    const int bytes_read = event_result;
    uint8_t *data = recv_buffers[used_buf_id]; 
    if (bytes_read == 0) {
        fprintf(stdout, "client has been disconnated\n");
        m_active_connections.erase(conn_sock_fd);
    } else if (bytes_read < 0) {
        fprintf(stdout, "reading error %s\n", strerror(bytes_read));
        m_active_connections.erase(conn_sock_fd);
    } else {
        std::string msg{(char*)data, (__u64)bytes_read};
        fprintf(stdout, "received request: %s from %d\n", msg.c_str(), conn_sock_fd);

        auto* queue_entry = new queue_entry_t;

        queue_entry->u64 = conn_sock_fd;
        queue_entry->buf_id = used_buf_id;
        queue_entry->data_len = bytes_read;

        m_sq->push(queue_entry);
    }
    return 0;
}

int event_loop::handle_worker_event()
{
    queue_entry_t* queue_entry;
    m_cq->pop(queue_entry);
    
    auto used_buf_id = queue_entry->buf_id; 
    auto return_code = queue_entry->return_code;
    auto client_sock_fd = (int)queue_entry->u64;
    auto server_sock_fd = queue_entry->return_value;

    fprintf(stdout, "resolve result %d %s\n", queue_entry->return_value, http_status_text::text(queue_entry->return_code));

    if (return_code != HTTP_OK) {
        m_active_connections.erase(client_sock_fd);
    } else {
        m_forwarding_rules.insert({client_sock_fd, server_sock_fd});
        m_forwarding_rules.insert({server_sock_fd, client_sock_fd});
    }
    
    auto* response = http_status_text::text(return_code);
    event_t write_event;
    write_event.type = WRITE;
    write_event.fd = client_sock_fd;

    auto * sqe = io_uring_get_sqe(&m_ring);
    io_uring_prep_send(sqe, client_sock_fd, response, strlen(response), 0);
    io_uring_sqe_set_data(sqe, (void*)write_event.u64);


    delete queue_entry;

    io_uring_buf_ring_add(m_br, recv_buffers[used_buf_id], MAX_HTTP_HEADER_LEN, used_buf_id, io_uring_buf_ring_mask(m_br_size), used_buf_id);
    io_uring_buf_ring_advance(m_br, 1);

    return 0;
}
