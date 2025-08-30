#include <connect.h>
#include <string.h>
#include <regex>

#include "http_status_codes.h"

const char *regex_str = "^CONNECT\\s+(.+):(\\d+)\\s+HTTP\\/(\\d.\\d)\\r?\\nHost: \\1:\\2\\r?\\n";

const std::regex connect_regex(
    regex_str,
    std::regex_constants::ECMAScript);

int parse_connect(std::string req, connect_request &req_struct)
{

    std::smatch matches;
    if (std::regex_search(req, matches, connect_regex))
    {
        req_struct.host = matches[1];
        if (matches[2].matched)
        {
            req_struct.port = matches[2];
        }
        else
        {
            req_struct.port = "433";
        }
    }
    else
    {
        return HTTP_BAD_REQUEST;
    }
    return HTTP_OK;
}