#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>

#include "../shared/zlog.h"
#include "../shared/zpath.h"
#include "http_parse.h"
#include "net_watt32.h"
#include "serve_file.h"
#include "zhttp.h"

static char g_root_dir[ZPATH_MAX];
static char g_request_buffer[ZHTTP_RECV_BUFFER];

static int parse_port(const char *text, int *port) {
    unsigned long value;
    const char *cursor;

    value = 0UL;
    cursor = text;

    if (*cursor == '\0') {
        return 0;
    }

    while (*cursor != '\0') {
        if (*cursor < '0' || *cursor > '9') {
            return 0;
        }
        value = value * 10UL + (unsigned long)(*cursor - '0');
        if (value > 65535UL) {
            return 0;
        }
        ++cursor;
    }

    if (value == 0UL) {
        return 0;
    }

    *port = (int)value;
    return 1;
}

static void print_usage(void) {
    printf("Usage:\n");
    printf("  zhttp.exe [ROOTDIR] [PORT]\n");
    printf("\n");
    printf("Examples:\n");
    printf("  zhttp.exe\n");
    printf("  zhttp.exe OUT\n");
    printf("  zhttp.exe C:\\MYBLOG\\OUT 8080\n");
}

static int should_stop(void) {
    int ch;

    if (znet_stop_requested()) {
        return 1;
    }

    if (!kbhit()) {
        return 0;
    }

    ch = getch();
    return ch == 24 || ch == 3 || ch == 27;
}

static int handle_client(ZNetSocket client, const char *root_dir) {
    ZHttpRequest request;
    int count;
    int status;

    count = znet_recv(client, g_request_buffer, sizeof(g_request_buffer) - 1);
    if (count <= 0) {
        return 0;
    }

    g_request_buffer[count] = '\0';
    if (!zhttp_parse_request_line(g_request_buffer, &request)) {
        zlog_request("BAD", "/", 400);
        return 0;
    }

    status = zhttp_serve_request(client, root_dir, &request);
    zlog_request(request.method, request.path, status);
    return 1;
}

int main(int argc, char **argv) {
    ZNetSocket server;
    ZNetSocket client;
    int port;

    zstr_copy(g_root_dir, sizeof(g_root_dir), ".");
    port = ZHTTP_DEFAULT_PORT;

    if (argc > 3) {
        print_usage();
        return 1;
    }

    if (argc >= 2) {
        zstr_copy(g_root_dir, sizeof(g_root_dir), argv[1]);
    }

    if (argc == 3 && !parse_port(argv[2], &port)) {
        printf("Invalid port %s\n", argv[2]);
        return 1;
    }

    if (!znet_init()) {
        printf("Networking not available or Watt-32 not initialized\n");
        printf("zhttp.exe requires Watt-32 and a DOS packet driver\n");
        return 1;
    }

    if (!znet_listen(port, &server)) {
        printf("Failed to listen on port %d\n", port);
        znet_shutdown();
        return 1;
    }

    printf("zhttp serving %s at http://localhost:%d\n", g_root_dir, port);
    printf("Press Ctrl-X to stop.\n");

    for (;;) {
        if (should_stop()) {
            break;
        }

        if (znet_accept(server, &client)) {
            handle_client(client, g_root_dir);
            znet_close(client);
            if (znet_stop_requested()) {
                break;
            }
        } else {
            delay(ZHTTP_IDLE_DELAY_MS);
        }
    }

    znet_close(server);
    znet_shutdown();
    return 0;
}
