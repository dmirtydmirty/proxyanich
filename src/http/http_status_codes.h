#pragma once

#include "constants.h"
enum {

    HTTP_OK = 200,

    HTTP_BAD_REQUEST = 400,
    HTTP_FORBIDDEN = 403,
    HTTP_METHOD_NOT_ALLOWED = 405,
    HTTP_PROXY_AUTH_REQUIRED = 407,

    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVER_UNAVALIBLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504
    
} HttpStatusCode;