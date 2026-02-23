#pragma once

#include <unordered_map>
#include <string>

struct http_status_text{
    http_status_text(uint16_t) = delete;
    static inline std::unordered_map<uint16_t, std::string> map = {
        {200, 
            "HTTP/1.1 200 Connection Established\r\n"
            "Content-Length: 0\r\n"
            "Proxy-Agent: MyProxy/1.0\r\n"
            "\r\n"},
        
        {400, 
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Bad Request"},
        
        {403, 
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 9\r\n"
            "\r\n"
            "Forbidden"},
        
        {405, 
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Allow: CONNECT\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 18\r\n"
            "\r\n"
            "Method Not Allowed"},
        
        {407, 
            "HTTP/1.1 407 Proxy Authentication Required\r\n"
            "Proxy-Authenticate: Basic realm=\"Proxy\"\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 27\r\n"
            "\r\n"
            "Proxy Authentication Required"},
        
        {502, 
            "HTTP/1.1 502 Bad Gateway\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "Bad Gateway"},
        
        {503, 
            "HTTP/1.1 503 Service Unavailable\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 19\r\n"
            "\r\n"
            "Service Unavailable"},
        
        {504, 
            "HTTP/1.1 504 Gateway Timeout\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "\r\n"
            "Gateway Timeout"},
    };

    static const char* text(uint16_t code) {
        if (not map.contains(code)) {
            return "";
        }
        return map[code].c_str();
    }
    

};


