#ifndef NET_WATT32_H
#define NET_WATT32_H

typedef struct ZNetHandle *ZNetSocket;

int znet_init(void);
void znet_shutdown(void);
int znet_listen(int port, ZNetSocket *server_socket);
int znet_accept(ZNetSocket server_socket, ZNetSocket *client_socket);
int znet_recv(ZNetSocket socket_id, char *buffer, int buffer_size);
int znet_send(ZNetSocket socket_id, const char *buffer, int length);
void znet_close(ZNetSocket socket_id);
int znet_stop_requested(void);

#endif
