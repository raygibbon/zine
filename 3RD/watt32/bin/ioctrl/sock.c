#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <tcp.h>

#include "config.h"
#include "util.h"
#include "file.h"

#undef  DEBUG

char *V_servico, *V_endereco;
int   V_max_ligacao;

enum sockstate { CLOSED, LISTENING, OPEN, CLOSING };

struct connection
{
	tcp_Socket	sock;
	enum sockstate	state;
	struct stats	*current;
};

static struct connection	conn[V_MAX_LIGACAO];
static struct stats		*list, *from;

static unsigned short	port;
static char		addrbuf[64];
static char		buffer[1024];
static unsigned long	nameserver = 0L;

int cria_socket ( int *soquete , char *protocolo , char *servico , 
			char *endereco , int tipo )
{
        int i;

#ifndef	DEBUG
	sock_init();
	nameserver = def_nameservers[0];
	for (i = 0; i < V_MAX_LIGACAO; ++i)
	{
		conn[i].state = CLOSED;
		conn[i].current = 0;
	}
        V_endereco = strdup (_inet_ntoa(buffer, my_ip_addr));
	if (strcmp(V_servico, V_SERVICO) == 0)
		port = 5050;
	else
		port = atoi(V_servico);
        printf(_LANG("My address is %s, my port is %hd\n"), V_endereco, port);
#else
	conn[0].state = OPEN;
	create_new_connection(&list, &from);
	from->soquete = 0;
	from->hostname = strdup("?");
	from->hostip = strdup("?");
#endif
	return (0);
}

void read_bytes(int n, struct stats *status_list, void *command_list, void *port_list)
{
        int  count;
        char *bp, *command, *temp = 0;

#ifndef	DEBUG
	if ((count = sock_rbused(&conn[n].sock)) <= 0)
           return;
	if (count > sizeof(buffer) - 1)
           count = sizeof(buffer) - 1;
	(void)sock_fastread(&conn[n].sock, buffer, count);
	buffer[count] = '\0';
#else
	fgets(buffer, sizeof(buffer), stdin);
#endif
	bp = buffer;
	while ((command = strtok_r(bp, LINE_COMMAND_SEPARATOR, &temp)) != NULL)
	{
		bp = NULL;
		if (parse_user_commands(n, command, status_list,
			command_list, port_list) == SAI)
			conn[n].state = CLOSING;
	}
}

int envia_mensagem ( int n , char *mensagem )
{
#ifndef	DEBUG
  sock_fastwrite(&conn[n].sock, mensagem, strlen(mensagem));
#else
  printf(mensagem);
#endif
  return (0);
}

int server_loop ( int soquete , void *command_list , void *port_list )
{
  int                  i, n;
  struct connection    *c;
  struct watt_sockaddr client;
  struct stats         new;

  for (;;) for (n = 0; n < V_MAX_LIGACAO; ++n)
  {
    kbhit();            /* so ^C can interrupt */

    c = &conn[n];
    switch (c->state)
    {
      case CLOSED:
           tcp_listen(&c->sock, port, 0L, 0, NULL, 0);
           c->state = LISTENING;
           if (!tcp_tick(&c->sock))
              c->state = CLOSING;
           break;

      case LISTENING:
           if (!tcp_tick(&c->sock))
           {
             c->state = CLOSING;
             break;
           }
           if (!sock_established(&c->sock))
              break;
           i = sizeof(client);
           client.s_ip = 0;
           new.hostip = _getpeername(&c->sock, &client, &i) == 0 ?
                   _inet_ntoa(addrbuf, client.s_ip) : "?";
           new.hostname = (nameserver != 0 &&
                   resolve_ip(client.s_ip, buffer,sizeof(buffer)) != 0) ?
                   buffer : "?";

           printf (_LANG("Connection from %s [%s] "),
                   new.hostname, new.hostip);
           /* match addresses here */
           if (verify_connect_address(V_address_file, &new) != 0)
           {
             envia_mensagem(n, "\n\nYou don't belong here...\n\n");
             printf (_LANG("rejected\n"));
             c->state = CLOSING;
             break;
           }
           printf (_LANG("accepted\n"));
           c->state = OPEN;
           create_new_connection(&list, &from);
           c->current = from;
           from->soquete = n;
           from->hostip = strdup(new.hostip);
           from->hostname = strdup(new.hostname);
           envia_mensagem(n, PROGRAM_NAME ">");
           break;

      case OPEN:
#ifndef	DEBUG
           if (!tcp_tick(&c->sock))
           {
             c->state = CLOSING;
             break;
           }
#endif
           read_bytes(n, list, command_list, port_list);
           break;
      case CLOSING:
           free_new_connection(&list, n);
           c->current = 0;
           sock_flush(&c->sock);
           sock_close(&c->sock);
           c->state = CLOSED;
           break;
    }
  }
}

/*
 *	This don't belong here but there's no suitable file for it
 *	as we want to keep non-DOS stuff out of other C files
 *	and ioperm is  useful for debugging.
 */

int ioperm(int from, int num, int turnon)
{
#ifdef	DEBUG
        printf (_LANG("Requesting 0x%x:%d for %d\n"), from, num, turnon);
#endif
	return (0);
}
