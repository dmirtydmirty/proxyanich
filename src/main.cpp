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

#include "event_loop/event_loop.h"
#include "config.h"

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

void handle_connection(event_object& ev) {
    int sock_fd = ev.rv;

    sockaddr_in saddr;
    socklen_t saddrlen = sizeof(saddr);
    getsockname(sock_fd, (sockaddr*)&saddr, &saddrlen);
    spdlog::debug("new connaction from {}:{}", inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));


    // ev.el->perp_recv(sock_fd, 0, handle_http_connect);    
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

int setup_udp_socket(uint16_t port, int backlog) {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
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
    while (global::proceed)
    {
        el.process();
    }
    

    if (sock_fd >0) {
        close(sock_fd);
    }
    // el.start(pipefd[0]);
    // el.stop();
    spdlog::info("server correctly stopped at, {}()!", __func__);
}
