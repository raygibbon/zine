#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#include "zhttp.h"

int zhttp_parse_request_line(const char *buffer, ZHttpRequest *request);

#endif
