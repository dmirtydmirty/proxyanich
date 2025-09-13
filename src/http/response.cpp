#include <response.h>
#include <cstdio>
#include <http_status_text.h>


int http_response(uint16_t code, char *buff)
{
    return sprintf(buff, "HTTP/1.1 %d %s\r\n", code, http_status_text::text(code));
}
