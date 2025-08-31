#include "resolve_task.h"

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <memory>

int resolve_task(void *ptr)
{
    auto pair = std::unique_ptr<std::pair<std::string, std::string>>(
        static_cast<std::pair<std::string, std::string>*>(ptr));
    printf("resolving: %s:%s\n", pair->first.c_str(), pair->second.c_str());

    addrinfo* pai;
    addrinfo hints{};

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (int ret = getaddrinfo(pair->first.c_str(), pair->second.c_str(), &hints, &pai); ret != 0) {
        fprintf(stderr, "Resolve error %s\n", gai_strerror(ret));
        return -1;
    }

    for (addrinfo* info = pai; info != nullptr;  info = info->ai_next) {
        int sock_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock_fd == -1) {
            continue;
        }
        if (connect(sock_fd, info->ai_addr, info->ai_addrlen) == 0){
            return sock_fd;
        }
        close(sock_fd); 
    }

    freeaddrinfo(pai);
    return -1;
}