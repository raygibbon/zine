#include <string.h>

#include "../shared/zpath.h"
#include "http_parse.h"

static int token_copy(char *dest, int dest_size, const char **cursor) {
    int i;

    while (**cursor == ' ') {
        ++(*cursor);
    }

    i = 0;
    while (**cursor != '\0' && **cursor != ' ' && **cursor != '\r' && **cursor != '\n') {
        if (i >= dest_size - 1) {
            return 0;
        }
        dest[i++] = **cursor;
        ++(*cursor);
    }
    dest[i] = '\0';

    return i > 0;
}

int zhttp_parse_request_line(const char *buffer, ZHttpRequest *request) {
    const char *cursor;
    char version[16];

    cursor = buffer;
    request->method[0] = '\0';
    request->path[0] = '\0';
    request->head_only = 0;

    if (!token_copy(request->method, sizeof(request->method), &cursor)) {
        return 0;
    }
    if (!token_copy(request->path, sizeof(request->path), &cursor)) {
        return 0;
    }
    token_copy(version, sizeof(version), &cursor);

    if (strcmp(request->method, "GET") == 0) {
        request->head_only = 0;
        return 1;
    }

    if (strcmp(request->method, "HEAD") == 0) {
        request->head_only = 1;
        return 1;
    }

    return 0;
}
