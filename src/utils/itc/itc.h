#pragma once
#include <cstdint>
#include <utility>

enum class msg_type: uint8_t {
    NEW_CONNECTION,
    CLIENT_DISCONNECTED,
    SERVER_DISCONNECTED,
    EMPTY = UINT8_MAX
};

struct itc_message
{
    union 
    {
        uint32_t sock_fd;
        std::pair<uint32_t, uint32_t> sock_fd_pair;
    };
    
    msg_type type;
};
