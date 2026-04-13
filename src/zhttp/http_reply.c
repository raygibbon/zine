#include <stdio.h>
#include <string.h>

#include "http_reply.h"

static int send_text(ZNetSocket client, const char *text) {
    return znet_send(client, text, (int)strlen(text)) >= 0;
}

static const char *cache_header_for(const char *mime_type) {
    if (strcmp(mime_type, "text/css") == 0 ||
        strcmp(mime_type, "application/javascript") == 0 ||
        strcmp(mime_type, "image/gif") == 0 ||
        strcmp(mime_type, "image/png") == 0 ||
        strcmp(mime_type, "image/jpeg") == 0 ||
        strcmp(mime_type, "font/ttf") == 0) {
        return "Cache-Control: public, max-age=3600\r\n";
    }

    return "Cache-Control: no-cache\r\n";
}

int zhttp_send_status(ZNetSocket client, int status, const char *reason, const char *mime_type, long content_length) {
    char header[220];

    sprintf(header,
        "HTTP/1.0 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "%s"
        "Connection: close\r\n"
        "\r\n",
        status,
        reason,
        mime_type,
        content_length,
        cache_header_for(mime_type));

    return send_text(client, header);
}

int zhttp_send_text_response(ZNetSocket client, int status, const char *reason, const char *body) {
    if (!zhttp_send_status(client, status, reason, "text/html", (long)strlen(body))) {
        return 0;
    }

    return send_text(client, body);
}

int zhttp_send_cached_no_content(ZNetSocket client) {
    return send_text(client,
        "HTTP/1.0 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "Cache-Control: public, max-age=3600\r\n"
        "Connection: close\r\n"
        "\r\n");
}
