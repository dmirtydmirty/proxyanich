#pragma once

#include <unordered_map>

struct http_status_text{
    http_status_text(uint16_t) = delete;
    static inline std::unordered_map<uint16_t, const char*> map = {
        {200, "Connection Established"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {405, "Method Not Allowed"},
        {407, "Proxy Authentication Required"},
        {502, "Bad Gateway"},
        {504, "Gateway Timeout"},
        {200, "Connection Established"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {405, "Method Not Allowed"},
        {407, "Proxy Authentication Required"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
    };

    static const char* text(uint16_t code) {
        if (not map.contains(code)) {
            return "";
        }
        return map[code];
    }
    

};


