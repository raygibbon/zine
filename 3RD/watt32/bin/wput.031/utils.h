#ifndef __UTILS_H
#define __UTILS_H

#include "wput.h"
#include <stdarg.h>

#define ipaddr h_addr_list[0]

void Abort(char * msg);
void Debug(const char * fmt, ...);
char * itoa(int, char *, int);

int create_new_reply_sockfd(const char * hostname, const int c_port);
int get_ip_addr (char* hostname, char *ip);
int get_local_ip (int sockfd, char *ip);
int initialize_server_master(int * s_sock, int * s_port);
int create_data_sockfd(int s_sock);

#endif
