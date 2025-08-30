#include <gtest/gtest.h>
#include <string>
#include "connect.h"
#include "response.h"
#include "http_status_codes.h"
#include "http_status_text.h"


TEST(Test, TestConnect) {

    std::string request = "CONNECT mrpipiskin.com:228 HTTP/1.1\r\nHost: mrpipiskin.com:228\r\nProxy-Authorization: sosal\r\n";

    ASSERT_EQ(is_connect(request), true);

    connect_request cr;

    ASSERT_EQ(parse_connect(request, cr), HTTP_OK);

    ASSERT_EQ(cr.host, "mrpipiskin.com");
    ASSERT_EQ(cr.port, "228");
}


TEST(Test, TestResponse) {
    char buff[128];
    for (const auto & el: http_status_text::map) {
        std::string resp = std::to_string(el.first) + " " + std::string(el.second) + "\r\n";
        http_response(el.first, buff);
        ASSERT_EQ(strncmp(buff, resp.c_str(), resp.size()), 0);
    }
}
