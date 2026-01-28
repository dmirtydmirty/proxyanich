#pragma once
#include <optional>
#include <string>

struct config_t {
    std::optional<std::string> log_file;
};

int read_config(const std::string& path);