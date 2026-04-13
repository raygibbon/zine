#include <string.h>

#include "mime.h"

static int ext_equals(const char *ext, const char *want) {
    while (*ext != '\0' && *want != '\0') {
        char a;
        char b;

        a = *ext;
        b = *want;
        if (a >= 'A' && a <= 'Z') {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b) {
            return 0;
        }
        ++ext;
        ++want;
    }

    return *ext == '\0' && *want == '\0';
}

const char *zhttp_mime_type(const char *path) {
    const char *dot;
    const char *cursor;

    dot = NULL;
    cursor = path;
    while (*cursor != '\0') {
        if (*cursor == '.') {
            dot = cursor;
        }
        ++cursor;
    }

    if (dot == NULL) {
        return "application/octet-stream";
    }

    if (ext_equals(dot, ".html") || ext_equals(dot, ".htm")) {
        return "text/html";
    }
    if (ext_equals(dot, ".css")) {
        return "text/css";
    }
    if (ext_equals(dot, ".js")) {
        return "application/javascript";
    }
    if (ext_equals(dot, ".txt")) {
        return "text/plain";
    }
    if (ext_equals(dot, ".gif")) {
        return "image/gif";
    }
    if (ext_equals(dot, ".png")) {
        return "image/png";
    }
    if (ext_equals(dot, ".jpg") || ext_equals(dot, ".jpeg")) {
        return "image/jpeg";
    }
    if (ext_equals(dot, ".ttf")) {
        return "font/ttf";
    }

    return "application/octet-stream";
}
