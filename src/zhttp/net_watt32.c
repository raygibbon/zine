#include "net_watt32.h"

#ifdef ZHTTP_USE_WATT32

#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <tcp.h>

#include "zhttp.h"

extern BOOL _watt_do_exit;

typedef struct ZNetNativeServer ZNetNativeServer;
typedef struct ZNetListenSlot ZNetListenSlot;

struct ZNetListenSlot {
    tcp_Socket tcp;
    int index;
    int listening;
};

struct ZNetHandle {
    tcp_Socket *tcp;
    ZNetNativeServer *server;
    ZNetListenSlot *slot;
    int is_server;
    int is_open;
};

struct ZNetNativeServer {
    ZNetListenSlot slots[ZHTTP_LISTEN_SLOTS];
    int port;
    int active_slots;
};

static ZNetNativeServer g_server;
static struct ZNetHandle g_server_handle;
static struct ZNetHandle g_client_handle;
static int g_stop_requested;

static int check_stop_key(void) {
    int ch;

    if (g_stop_requested) {
        return 1;
    }

    if (!kbhit()) {
        return 0;
    }

    ch = getch();
    if (ch == 24 || ch == 3 || ch == 27) {
        g_stop_requested = 1;
        return 1;
    }

    return 0;
}

#if ZHTTP_TIMING_LOG
static void znet_log(const char *event, int slot_index) {
    if (slot_index >= 0) {
        printf("net %s slot %d\n", event, slot_index);
    } else {
        printf("net %s\n", event);
    }
}
#else
#define znet_log(event, slot_index) ((void)0)
#endif

static int listen_again(ZNetNativeServer *server, ZNetListenSlot *slot) {
    memset(&slot->tcp, 0, sizeof(slot->tcp));

    if (!tcp_listen(&slot->tcp, (WORD)server->port, 0UL, 0, NULL, 0)) {
        slot->listening = 0;
        znet_log("listen-fail", slot->index);
        return 0;
    }

    sock_mode((sock_type *)&slot->tcp, SOCK_MODE_BINARY | TCP_MODE_NONAGLE);
    slot->listening = 1;
    znet_log("listening", slot->index);
    return 1;
}

int znet_init(void) {
    _watt_do_exit = 0;
    g_stop_requested = 0;
    return sock_init() == 0;
}

void znet_shutdown(void) {
    sock_exit();
}

int znet_listen(int port, ZNetSocket *server_socket) {
    int i;

    memset(&g_server, 0, sizeof(g_server));
    memset(&g_server_handle, 0, sizeof(g_server_handle));
    memset(&g_client_handle, 0, sizeof(g_client_handle));

    g_server.port = port;

    for (i = 0; i < ZHTTP_LISTEN_SLOTS; ++i) {
        g_server.slots[i].index = i;
        if (listen_again(&g_server, &g_server.slots[i])) {
            ++g_server.active_slots;
        }
    }

    if (g_server.active_slots == 0) {
        printf("tcp_listen failed on port %d\n", port);
        return 0;
    }
    printf("zhttp slots: %d/%d\n", g_server.active_slots, ZHTTP_LISTEN_SLOTS);

    g_server_handle.tcp = NULL;
    g_server_handle.server = &g_server;
    g_server_handle.slot = NULL;
    g_server_handle.is_server = 1;
    g_server_handle.is_open = 1;

    *server_socket = &g_server_handle;
    return 1;
}

int znet_accept(ZNetSocket server_socket, ZNetSocket *client_socket) {
    ZNetNativeServer *server;
    ZNetListenSlot *slot;
    int i;

    if (server_socket == NULL || !server_socket->is_server) {
        return 0;
    }

    server = server_socket->server;
    if (server == NULL) {
        return 0;
    }

    for (i = 0; i < ZHTTP_LISTEN_SLOTS; ++i) {
        slot = &server->slots[i];
        if (!slot->listening) {
            listen_again(server, slot);
            continue;
        }

        if (!tcp_tick((sock_type *)&slot->tcp)) {
            slot->listening = 0;
            znet_log("restart", slot->index);
            listen_again(server, slot);
            continue;
        }

        if (!tcp_established(&slot->tcp)) {
            continue;
        }

        memset(&g_client_handle, 0, sizeof(g_client_handle));
        g_client_handle.tcp = &slot->tcp;
        g_client_handle.server = server;
        g_client_handle.slot = slot;
        g_client_handle.is_server = 0;
        g_client_handle.is_open = 1;
        slot->listening = 0;

        sock_mode((sock_type *)&slot->tcp, SOCK_MODE_BINARY | TCP_MODE_NONAGLE);
        znet_log("accepted", slot->index);

        *client_socket = &g_client_handle;
        return 1;
    }

    return 0;
}

int znet_recv(ZNetSocket socket_id, char *buffer, int buffer_size) {
    tcp_Socket *tcp;
    DWORD timeout;

    if (socket_id == NULL || socket_id->tcp == NULL || buffer_size <= 0) {
        return -1;
    }

    tcp = socket_id->tcp;
    timeout = set_timeout(3000UL);

    while (!sock_dataready((sock_type *)tcp)) {
        if (check_stop_key()) {
            return -1;
        }

        if (!tcp_tick((sock_type *)tcp)) {
            return -1;
        }

        if (chk_timeout(timeout)) {
            return -1;
        }
    }

    return sock_fastread((sock_type *)tcp, (BYTE *)buffer, buffer_size);
}

int znet_send(ZNetSocket socket_id, const char *buffer, int length) {
    tcp_Socket *tcp;
#if ZHTTP_USE_FASTWRITE
    DWORD timeout;
    int total;
    int count;
#endif

    if (socket_id == NULL || socket_id->tcp == NULL || length < 0) {
        return -1;
    }

    if (length == 0) {
        return 0;
    }

    tcp = socket_id->tcp;
#if ZHTTP_USE_FASTWRITE
    total = 0;
    timeout = set_timeout(ZHTTP_SEND_TIMEOUT_MS);
    while (total < length) {
        if (check_stop_key()) {
            return -1;
        }

        count = sock_fastwrite((sock_type *)tcp, (const BYTE *)(buffer + total), length - total);
        if (count > 0) {
            total += count;
            timeout = set_timeout(ZHTTP_SEND_TIMEOUT_MS);
        } else if (chk_timeout(timeout)) {
            return -1;
        }

        if (!tcp_tick((sock_type *)tcp)) {
            return -1;
        }
    }
#else
    if (sock_write((sock_type *)tcp, (const BYTE *)buffer, length) != length) {
        return -1;
    }

    tcp_tick((sock_type *)tcp);
#endif
    return length;
}

void znet_close(ZNetSocket socket_id) {
    ZNetNativeServer *server;
    tcp_Socket *tcp;
    int i;

    if (socket_id == NULL || !socket_id->is_open) {
        return;
    }

    server = socket_id->server;
    socket_id->is_open = 0;

    if (socket_id->is_server) {
        if (server != NULL) {
            for (i = 0; i < ZHTTP_LISTEN_SLOTS; ++i) {
                if (server->slots[i].listening) {
                    sock_close((sock_type *)&server->slots[i].tcp);
                    tcp_tick((sock_type *)&server->slots[i].tcp);
                    server->slots[i].listening = 0;
                }
            }
        }
        return;
    }

    if (socket_id->tcp == NULL) {
        return;
    }

    tcp = socket_id->tcp;
    sock_close((sock_type *)tcp);
    tcp_tick((sock_type *)tcp);
    if (socket_id->slot != NULL) {
        znet_log("closed", socket_id->slot->index);
    }

    if (server != NULL && socket_id->slot != NULL) {
        listen_again(server, socket_id->slot);
    }
}

int znet_stop_requested(void) {
    return g_stop_requested || check_stop_key();
}

#else

int znet_init(void) {
    return 0;
}

void znet_shutdown(void) {
}

int znet_listen(int port, ZNetSocket *server_socket) {
    (void)port;
    (void)server_socket;
    return 0;
}

int znet_accept(ZNetSocket server_socket, ZNetSocket *client_socket) {
    (void)server_socket;
    (void)client_socket;
    return 0;
}

int znet_recv(ZNetSocket socket_id, char *buffer, int buffer_size) {
    (void)socket_id;
    (void)buffer;
    (void)buffer_size;
    return -1;
}

int znet_send(ZNetSocket socket_id, const char *buffer, int length) {
    (void)socket_id;
    (void)buffer;
    (void)length;
    return -1;
}

void znet_close(ZNetSocket socket_id) {
    (void)socket_id;
}

int znet_stop_requested(void) {
    return 0;
}

#endif
