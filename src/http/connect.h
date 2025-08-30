#pragma once

#include <string>
#include <cstdint>
#include <regex>

struct connect_request {
    std::string host;
    std::string authorization;
    std::string port;
};

bool is_connect(const std::string& req);

int parse_connect(std::string req, connect_request& req_struct);
