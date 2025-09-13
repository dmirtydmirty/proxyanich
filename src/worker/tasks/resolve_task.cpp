#include "resolve_task.h"
#include "recv_buffers.h"
#include "queue/queue.h"
#include "connect.h"
#include "http_status_codes.h"

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <memory>

extern recv_buffers_t recv_buffers;

void resolve_task(void *ptr)
{
    auto* queue_entry = (queue_entry_t*)(ptr);
    uint8_t* buf = recv_buffers[queue_entry->buf_id];
    std::string user_request((char* )buf, queue_entry->data_len);
    connect_request cr;

    int rc = parse_connect(user_request, cr);
    if (rc != HTTP_OK) {

        queue_entry->return_code = rc;
        queue_entry->return_value = -1;
        return;
    }
    printf("resolving: %s:%s\n", cr.host.c_str(), cr.port.c_str());

    addrinfo* pai;
    addrinfo hints{};

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (int ret = getaddrinfo(cr.host.c_str(), cr.port.c_str(), &hints, &pai); ret != 0) {
        fprintf(stderr, "Resolve error %s\n", gai_strerror(ret));
        queue_entry->return_code = HTTP_BAD_REQUEST;
        queue_entry->return_value = -1;
        return;
    }

    for (addrinfo* info = pai; info != nullptr; info = info->ai_next) {
        int sock_fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock_fd == -1) {
            continue;
        }
        if (connect(sock_fd, info->ai_addr, info->ai_addrlen) == 0){
            fprintf(stdout, "Connection established\n");
            queue_entry->return_code = HTTP_OK; 
            queue_entry->return_value = sock_fd;
            return;
        }
        close(sock_fd); 
    }

    freeaddrinfo(pai);
    queue_entry->return_code = HTTP_SERVER_UNAVALIBLE; 
    queue_entry->return_value = -1;
}