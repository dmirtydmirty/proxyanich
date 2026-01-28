#pragma once

#include <string>
#include <cstdint>

struct connect_request {
    std::string host{};
    std::string authorization{};
    std::string port{};
};

int parse_connect(const std::string_view& req, connect_request& req_struct);
