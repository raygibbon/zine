#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../shared/zfile.h"
#include "../shared/zpath.h"
#include "http_reply.h"
#include "mime.h"
#include "serve_file.h"

static char g_local_path[ZPATH_MAX];
static char g_full_path[ZPATH_MAX];
static char g_file_buffer[ZHTTP_FILE_BUFFER];

static int is_favicon_path(const char *path) {
    return strcmp(path, "/favicon.ico") == 0 ||
           strcmp(path, "/FAVICON.ICO") == 0;
}

#if ZHTTP_TIMING_LOG
static long elapsed_ms(clock_t start) {
    return (long)(((clock() - start) * 1000L) / CLOCKS_PER_SEC);
}

static void serve_log(const char *event, const char *path, long value, clock_t start) {
    printf("file %s %s %ld %ldms\n", event, path, value, elapsed_ms(start));
}
#else
#define serve_log(event, path, value, start) ((void)0)
#endif

static int map_request_path(const char *root_dir, const char *url_path, char *dest, int dest_size) {
    int in;
    int out;
    char ch;

    in = 0;
    out = 0;
    if (url_path[0] == '/') {
        in = 1;
    }

    while (url_path[in] != '\0' && url_path[in] != '?' && url_path[in] != '#') {
        if (out >= (int)sizeof(g_local_path) - 1) {
            return 0;
        }

        ch = url_path[in];
        if (ch == '/') {
            ch = '\\';
        }

        if ((out == 0 && ch == '\\') || ch == ':' || (ch == '.' && url_path[in + 1] == '.')) {
            return 0;
        }

        g_local_path[out++] = ch;
        ++in;
    }
    g_local_path[out] = '\0';

    if (g_local_path[0] == '\0') {
        zpath_join(dest, dest_size, root_dir, "INDEX.HTM");
        return 1;
    }

    zpath_join(dest, dest_size, root_dir, g_local_path);
    return 1;
}

int zhttp_serve_request(ZNetSocket client, const char *root_dir, const ZHttpRequest *request) {
    FILE *fp;
    long file_size;
    long sent;
    int count;
    int first_chunk;
    clock_t start;

    start = clock();
    sent = 0L;
    first_chunk = 1;

    if (is_favicon_path(request->path)) {
        zhttp_send_cached_no_content(client);
        serve_log("204", request->path, 0L, start);
        return 204;
    }

    if (!map_request_path(root_dir, request->path, g_full_path, sizeof(g_full_path))) {
        zhttp_send_text_response(client, 404, "Not Found", "<html><body><h1>404 Not Found</h1></body></html>\r\n");
        serve_log("404", request->path, 0L, start);
        return 404;
    }

    fp = zfile_open_read(g_full_path);
    if (fp == NULL) {
        zhttp_send_text_response(client, 404, "Not Found", "<html><body><h1>404 Not Found</h1></body></html>\r\n");
        serve_log("404", request->path, 0L, start);
        return 404;
    }

    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if (file_size < 0L) {
        file_size = 0L;
    }

    if (!zhttp_send_status(client, 200, "OK", zhttp_mime_type(g_full_path), file_size)) {
        fclose(fp);
        serve_log("header-fail", request->path, 0L, start);
        return 500;
    }

    if (!request->head_only) {
        while ((count = (int)fread(g_file_buffer, 1, sizeof(g_file_buffer), fp)) > 0) {
            if (znet_send(client, g_file_buffer, count) < 0) {
                fclose(fp);
                serve_log("send-fail", request->path, sent, start);
                return 500;
            }
            sent += (long)count;
            if (first_chunk) {
                serve_log("first", request->path, sent, start);
                first_chunk = 0;
            }
        }
    }

    fclose(fp);
    serve_log("done", request->path, sent, start);
    return 200;
}
