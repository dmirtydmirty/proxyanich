#pragma once

#include <unordered_map>

struct http_status_text{
    http_status_text(uint16_t) = delete;
    static inline std::unordered_map<uint16_t, const char*> map = {
        {200, "HTTP/1.1 200 Connection Established\n\r"},
        {400, "HTTP/1.1 400 Bad Request\n\r"},
        {403, "HTTP/1.1 403 Forbidden\n\r"},
        {405, "HTTP/1.1 405 Method Not Allowed\n\r"},
        {407, "HTTP/1.1 407 Proxy Authentication Required\n\r"},
        {502, "HTTP/1.1 502 Bad Gateway\n\r"},
        {504, "HTTP/1.1 504 Gateway Timeout\n\r"},
        {200, "HTTP/1.1 200 Connection Established\n\r"},
        {400, "HTTP/1.1 400 Bad Request\n\r"},
        {403, "HTTP/1.1 403 Forbidden\n\r"},
        {405, "HTTP/1.1 405 Method Not Allowed\n\r"},
        {407, "HTTP/1.1 407 Proxy Authentication Required\n\r"},
        {502, "HTTP/1.1 502 Bad Gateway\n\r"},
        {503, "HTTP/1.1 503 Service Unavailable\n\r"},
        {504, "HTTP/1.1 504 Gateway Timeout\n\r"},
    };

    static const char* text(uint16_t code) {
        if (not map.contains(code)) {
            return "";
        }
        return map[code];
    }
    

};


