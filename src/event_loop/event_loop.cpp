#include <sys/socket.h>
#include <netinet/in.h>

#include "event_loop.h"
#include "http_status_codes.h"
 
event_loop::event_loop(std::shared_ptr<complition_queue> cq, std::shared_ptr<submition_queue> sq) :
    m_cq(cq),
    m_sq(sq)
{}

int event_loop::init(size_t max_client_cnt)
{

    m_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (m_sock_fd == -1)
    {
        perror("event_loop::init() creation socket error");
        return -1;
    }

    const int optval = 1;
    if (setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("event_loop::init() setsockopt error");
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(15001);

    if (bind(m_sock_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    {
        perror("event_loop::init() binding socket error");
        return -1;
    }

    listen(m_sock_fd, max_client_cnt);

    io_uring_params params{};

    if (int ret = io_uring_queue_init_params(max_client_cnt, &m_ring, &params); ret == -1)
    {
        fprintf(stderr, "event_loop::init() io_uring_queue_init error, %s", strerror(-ret));
        return -1;
    }

    posix_memalign((void**)&m_br, 64, max_client_cnt * sizeof(io_uring_buf));
    posix_memalign((void**)&m_buffs, 64, max_client_cnt * MAX_HTTP_HEADER_LEN);

    for (size_t i = 0; i < max_client_cnt; ++i) {
        io_uring_buf_ring_add(m_br, m_buffs + MAX_HTTP_HEADER_LEN * i, MAX_HTTP_HEADER_LEN, i, io_uring_buf_ring_mask(max_client_cnt), i);
    }

    io_uring_buf_ring_advance(m_br, max_client_cnt);

    io_uring_buf_reg reg = {
        .ring_addr = (__u64)m_br,
        .ring_entries = (uint32_t)max_client_cnt,
        .bgid = 0,

    };
    
    if (int ret = io_uring_register_buf_ring(&m_ring, &reg, 0); ret != 0)  {
        fprintf(stderr, "event_loop::init() io_uring_register_buf_ring error %s", strerror(-ret));
        return -1;
    }

    return 0;
}


void event_loop::deinit() {
    free(m_buffs);
    free(m_br);
}

int event_loop::start()
{
    event_t accept_event{
        .fd = m_sock_fd,
        .type = ACCEPT,
    };

    auto* sqe = io_uring_get_sqe(&m_ring);

    io_uring_prep_multishot_accept(sqe, m_sock_fd, nullptr, 0, 0);
    io_uring_sqe_set_data(sqe, &accept_event);
    io_uring_submit(&m_ring);


    while (m_stop)
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
            auto event = (event_t*)io_uring_cqe_get_data(cqe);

            switch (event->type)
            {
            case ACCEPT:

                break;
            
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
