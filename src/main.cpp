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
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "connect.h"
#include "response.h"
#include "http_status_codes.h"
#include "event_loop/event_loop.h"
#include "config.h"
#include "http_status_text.h"
#include "itc/itc.h"
#include "dp_thread.h"

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


boost::lockfree::spsc_queue<itc_message, boost::lockfree::capacity<10>> DP_thread_queue;
boost::lockfree::spsc_queue<itc_message, boost::lockfree::capacity<10>> CP_thread_queue;

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


    ev.el->perp_recv(sock_fd, 0, handle_http_connect);    
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
    spdlog::info("starting server at, {}()!", __func__);

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

    dp_thread dpt;

    dpt.start();

    while (global::proceed)
    {
        el.process();
    }
    
    dpt.stop();

    if (sock_fd >0) {
        close(sock_fd);
    }
    // el.start(pipefd[0]);
    // el.stop();
    spdlog::info("server correctly stopped at, {}()!", __func__);
}
