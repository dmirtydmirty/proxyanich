#pragma once
#include <cstdint>
#include <utility>

enum class itc_message_type: uint8_t {
    NEW_CONNECTION,
    CLIENT_DISCONNECTED,
    SERVER_DISCONNECTED,
    EMPTY = UINT8_MAX
};

struct itc_message
{
    union 
    {
        int sock_fd;
        struct {
            int client_fd;
            int server_fd;
        } sock_pair;
    };
    
    itc_message_type type = itc_message_type::EMPTY;
};