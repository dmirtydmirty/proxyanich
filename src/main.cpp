#include <stdio.h>
#include <string.h>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <liburing.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <unordered_map>
#include <memory>

#include "connect.h"
#include "response.h"
#include "http_status_codes.h"
#include "event_loop/event_loop.h"
#include "config.h"
#include "http_status_text.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

constexpr inline size_t MAX_CLIENT_COUNT = 1 << 10;
constexpr inline size_t BUFFLEN = UINT16_MAX;

namespace global {
    extern config_t config;
    volatile bool proceed = true;

};

void int_handler(int) {
    global::proceed = false;
    spdlog::debug("interuption");
}

// struct event_t
// {
//     int fd;
//     unsigned type;
// };

// enum EVENT_TYPE
// {
//     ACCEPT,
//     READ,
//     WRITE,
//     WORK,
// };

// struct client
// {
//     int conn_fd;
//     unsigned status;
// };
 
// enum CLIENT_STATUS
// {
//     NEW,
//     RESOLVING,
//     AUTHORIZED
// };


int resolve_dns(const connect_request& rq ) {
    spdlog::debug("resolving: {}:{}", rq.host.c_str(), rq.port.c_str());

    addrinfo* pai;
    addrinfo hints{};

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (int ret = getaddrinfo(rq.host.c_str(), rq.port.c_str(), &hints, &pai); ret != 0) {
        spdlog::error("Resolve error {}", gai_strerror(ret));
        return -HTTP_BAD_REQUEST;
    }

    for (addrinfo* info = pai; info != nullptr; info = info->ai_next) {
        int sock_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock_fd == -1) {
            continue;
        }
        if (connect(sock_fd, info->ai_addr, info->ai_addrlen) == 0){
            auto* addr = (sockaddr_in*)info->ai_addr;
            spdlog::info("Connection with {}:{} established: addr: {}", 
                rq.host.c_str(), rq.port.c_str(), inet_ntoa(addr->sin_addr));
            freeaddrinfo(pai);
            return sock_fd;
        }
        close(sock_fd); 
    }
    return -HTTP_SERVER_UNAVALIBLE;
}

int init_logger(){
    if (global::config.log_file) {
        spdlog::set_default_logger(spdlog::basic_logger_mt("logger", "logs/log.log"));
    }
    spdlog::set_level(spdlog::level::trace);
    spdlog::set_error_handler([](const std::string& msg) {
        fprintf(stderr, "logger err %s \n", msg.c_str());
    });
    return 0;
}

void close_socket(event_object& ev){
    close(ev.fd);
}

void handle_http_connect(event_object& ev){

    size_t bytes_recv = ev.rv;
    size_t bid = ev.flags >> IORING_CQE_BUFFER_SHIFT;
    auto msg = std::string_view(ev.el->buffs[bid], bytes_recv);  
    spdlog::debug("Received {}", msg.data());

    connect_request rq;

    if (int code = parse_connect(msg, rq); code != HTTP_OK) {
        const char* resp = http_status_text::text(code);
        ev.el->prep_send(ev.fd, resp, strlen(resp), close_socket);
    } else {
        int rslv = resolve_dns(rq);
        if (rslv < 0) {
            const char* resp = http_status_text::text(-rslv);
            ev.el->prep_send(ev.fd, resp, strlen(resp), close_socket);
        } else {
            spdlog::info("connected");
            const char* resp = http_status_text::text(HTTP_OK);
            ev.el->prep_send(ev.fd, resp, strlen(resp), close_socket);
            close(rslv);
        }
    }
    ev.el->free_buf(bid);
}

void handle_connection(event_object& ev) {
    int sock_fd = ev.rv;

    sockaddr_in saddr;
    socklen_t saddrlen = sizeof(saddr);
    getsockname(sock_fd, (sockaddr*)&saddr, &saddrlen);
    spdlog::debug("new connaction from {}:{}", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));


    ev.el->perp_recv(sock_fd, handle_http_connect);    
}

int setup_tcp_socket(uint16_t port, int backlog) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        spdlog::error("creation socket failure: {}", strerror(errno));
        return -1;
    }

    const int optval = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        spdlog::error("setsockopt failure: {}", strerror(errno));
        return -1;
    }

    sockaddr_in saddr{
        .sin_port = htons(port), 
        .sin_addr = INADDR_ANY
    };

    if (bind(sock_fd, (sockaddr*)&saddr, sizeof(saddr)) == -1) {
        spdlog::error("socket binding failure: {}", strerror(errno));
        return -1;
    }

    if (listen(sock_fd, backlog) == -1) {
        spdlog::error("socket listen failure: {}", strerror(errno));
        return -1;
    }
    return sock_fd;
}

int main(int argc, char **argv)
{
    init_logger();
    spdlog::debug("starting server at, {}()!", __func__);
    // char trash[1];
    // int pipefd[2];
    // if (pipe2(pipefd, O_DIRECT) == -1) {
    //     perror("Creation pipe error");
    //     return -1;
    // }


    // if (sq->init() != 0) {
    //     perror("Submission queue initialization");
    //     return -1;
    // }

    // if (cq->init() != 0) {
    //     perror("Complition queue initialization");
    //     return -1;
    // }

    // Worker worker1;
    // worker1.init(sq, cq, std::bind(&pipe_notificator, pipefd[1]));
    // worker1.run();

    // std::unordered_map<int, client> clients;
    // auto recv_buff = std::make_unique<char[]>(BUFFLEN);
    // auto send_buff = std::make_unique<char[]>(BUFFLEN);

    // int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // if (sock_fd == -1)
    // {
    //     perror("Creation socket error");
    //     return -1;
    // }

    // const int optval = 1;
    // if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
    //     perror("setsockopt error");
    //     return -1;
    // }

    // sockaddr_in addr;
    // addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = INADDR_ANY;
    // addr.sin_port = htons(15001);

    // if (bind(sock_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1)
    // {
    //     perror("Binding socket error");
    //     return -1;
    // }

    // listen(sock_fd, MAX_CLIENT_COUNT);

    // io_uring_params params{};
    // io_uring ring;
    // // memset(&params, 0, sizeof(params));

    // if (int ret = io_uring_queue_init_params(MAX_CLIENT_COUNT, &ring, &params); ret == -1)
    // {
    //     fprintf(stderr, "io_uring_queue_init error, %s", strerror(-ret));
    //     return -1;
    // }

    // io_uring_sqe *sqe = io_uring_get_sqe(&ring);

    // sockaddr peer_addr;
    // socklen_t peer_addr_len = sizeof(peer_addr);

    // event_t accept_event{.fd = sock_fd, .type = ACCEPT};

    // io_uring_prep_accept(sqe, sock_fd, &peer_addr, &peer_addr_len, 0);

    // io_uring_sqe_set_data(sqe, &accept_event);

    // sqe = io_uring_get_sqe(&ring);

    // event_t worker_event{.fd = pipefd[0], .type = WORK};

    // io_uring_prep_read(sqe, pipefd[0], trash, 1, 0);

    // io_uring_sqe_set_data(sqe, &worker_event);
    
    // while (true)
    // {
    //     io_uring_cqe *cqe;

    //     io_uring_submit(&ring);
    //     if (int ret = io_uring_wait_cqe(&ring, &cqe); ret != 0)
    //     {
    //         fprintf(stderr, "io_uring_wait_cqe error: %s", strerror(-ret));
    //         return -1;
    //     }
    //     event_t *ready_event = reinterpret_cast<event_t *>(io_uring_cqe_get_data(cqe));

    //     if (ready_event->type == ACCEPT)
    //     {
    //         int conn_sock_fd = cqe->res;

    //         if (conn_sock_fd < 0)
    //         {
    //             fprintf(stderr, "Aception failed %s\n", strerror(-conn_sock_fd));
    //         }
    //         else
    //         {
    //             clients.insert({conn_sock_fd, client{.conn_fd = conn_sock_fd, .status = NEW}});

    //             io_uring_sqe *sqe = io_uring_get_sqe(&ring);

    //             event_t read_event{.fd = conn_sock_fd, .type = READ};

    //             io_uring_prep_recv(sqe, conn_sock_fd, recv_buff.get(), BUFFLEN, 0);

    //             io_uring_sqe_set_data(sqe, &read_event);
    //             fprintf(stdout, "New connection %d\n", conn_sock_fd);
    //         }

    //         io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    //         io_uring_prep_accept(sqe, sock_fd, &peer_addr, &peer_addr_len, 0);

    //         io_uring_sqe_set_data(sqe, &accept_event);
    //     }
    //     else if (ready_event->type == READ)
    //     {
    //         int conn_sock_fd = ready_event->fd;
    //         uint16_t bytes = cqe->res;

    //         if (bytes < 0)
    //         {
    //             fprintf(stderr, "Recv failure");
    //             clients.erase(conn_sock_fd);
    //             close(conn_sock_fd);
    //         }
    //         else if (bytes == 0)
    //         {
    //             fprintf(stdout, "Client %d has been disconnacted\n", conn_sock_fd);
    //             clients.erase(conn_sock_fd);
    //         }
    //         else
    //         { 
    //             connect_request req;
    //             std::string msg{recv_buff.get(), bytes};
    //             if (int ret =  parse_connect(msg, req); ret != HTTP_OK) {
    //                 io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    //                 event_t write_event{.fd = conn_sock_fd, .type = WRITE};
    //                 io_uring_prep_send(sqe, conn_sock_fd, send_buff.get(), http_response(ret, send_buff.get()), 0);
    //                 io_uring_sqe_set_data(sqe, &write_event);

    //                 clients.erase(conn_sock_fd);
    //             } else {

    //                 task_t resolve{
    //                     .task_function = resolve_task, 
    //                     .arg = new std::pair<std::string, std::string>(std::move(req.host), std::move(req.port)), 
    //                     .u64 = (uint64_t)conn_sock_fd};

    //                 if (not sq->push(resolve)) {
                        
    //                     io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    //                     event_t write_event{.fd = conn_sock_fd, .type = WRITE};
    //                     io_uring_prep_send(sqe, conn_sock_fd, send_buff.get(), http_response(HTTP_SERVER_UNAVALIBLE, send_buff.get()), 0);
    //                     io_uring_sqe_set_data(sqe, &write_event);
    
    //                     clients.erase(conn_sock_fd);

    //                 }

    //                 clients[conn_sock_fd].status = RESOLVING;
    //             }
                
    //         }
    //     } 
    //     else if (ready_event->type == WORK) {
    //         result_t work_result;
    //         cq->pop(work_result);

    //         printf("resolve res:  %d\n", work_result.ret);
            
    //         int conn_sock_fd = work_result.u64;
    //         io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    //         event_t write_event{.fd = conn_sock_fd, .type = WRITE};
    //         if (work_result.ret == -1) {
    //             io_uring_prep_send(sqe, conn_sock_fd, send_buff.get(), http_response(HTTP_BAD_GATEWAY, send_buff.get()), 0);
    //             clients.erase(conn_sock_fd);
    //         } else {
    //             io_uring_prep_send(sqe, conn_sock_fd, send_buff.get(), http_response(HTTP_OK, send_buff.get()), 0);
    //         }
    //         io_uring_sqe_set_data(sqe, &write_event);

    //         sqe = io_uring_get_sqe(&ring);

    //         event_t work_event{.fd = pipefd[0], .type = WORK};
        
    //         io_uring_prep_read(sqe, pipefd[0], trash, 1, 0);
        
    //         io_uring_sqe_set_data(sqe, &work_event);
    //     }
    //     else if (ready_event->type == WRITE)
    //     {
    //         int conn_sock_fd = ready_event->fd;
    //         if (not clients.contains(conn_sock_fd)) {
    //             fprintf(stdout, "Closing connection with %d\n", conn_sock_fd);
    //             close(conn_sock_fd);
    //         }
    //     }

    //     io_uring_cqe_seen(&ring, cqe);
    // }
    // auto sq = std::make_shared<submition_queue>(MAX_CLIENT_COUNT);
    // auto cq = std::make_shared<complition_queue>(MAX_CLIENT_COUNT);


    // int pipefd[2];
    // if (pipe2(pipefd, O_DIRECT) == -1) {
    //     spdlog::critical("Creation pipe error: {}!", strerror(errno));
    //     return -1;
    // }

    // Worker worker1;
    // worker1.init(sq, cq, std::bind(&pipe_notificator, pipefd[1]));
    // worker1.run();

    signal(SIGINT, int_handler);

    event_loop el;
    if (el.init(8192, 128) != 0)
    {
        spdlog::critical("{}(): initialization event loop failed", __func__);
        el.deinit();
        return -1;
    }

    int sock_fd = setup_tcp_socket(15001, 10);

    if (sock_fd == -1) {
        spdlog::critical("creation socket failure");
        return -1;
    }

    el.prep_accept(sock_fd, handle_connection);

    while (global::proceed)
    {
        el.process();
    }
    

    if (sock_fd >0) {
        close(sock_fd);
    }
    // el.start(pipefd[0]);
    // el.stop();
}
