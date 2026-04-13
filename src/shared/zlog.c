#include <stdio.h>

#include "zlog.h"

#ifndef ZHTTP_LOG_EACH_REQUEST
#define ZHTTP_LOG_EACH_REQUEST 0
#endif

void zlog_info(const char *message) {
    printf("%s\n", message);
}

void zlog_request(const char *method, const char *path, int status) {
#if !ZHTTP_LOG_EACH_REQUEST
    if (status < 400) {
        return;
    }
#endif
    printf("%s %s -> %d\n", method, path, status);
}
