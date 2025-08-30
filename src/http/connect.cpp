#include <connect.h>
#include <string.h>

#include "http_status_codes.h"

std::string regex_str = R"(CONNECT\s+)";  

std::regex connect_regex(
    regex_str,
    std::regex_constants::ECMAScript | std::regex_constants::icase
);

bool is_connect(const std::string& req) {
    std::smatch matches;
    return std::regex_search(req, matches, connect_regex);
}

int parse_connect(std::string req, connect_request& req_struct) {


    std::smatch matches;
    if (std::regex_search(req, matches, connect_regex)) {
        req_struct.host = matches[2];
        if (matches[3].matched) {
            req_struct.port = matches[3];
        } else {
            req_struct.port = "433";
        }
    } else {
        return HTTP_BAD_REQUEST;
    }
    return HTTP_OK;

}