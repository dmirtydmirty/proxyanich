#include <response.h>
#include <cstdio>
#include <http_status_text.h>


int http_response(uint16_t code, char *buff)
{
    return sprintf(buff, "%s", http_status_text::text(code));
}

