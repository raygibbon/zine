/*
 * 
 * servidor_sock.h - Suporte para Rede para o servidor
 * 
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

#define BIND_TYPE 1
#define CONNECT_TYPE !BIND_TYPE

#define SOCKET 1
#define NSOCKET !SOCKET

#define UNKNOWN "unknown"


extern int cria_socket ( int *soquete , char *protocolo , char *servico , 
			char *endereco , int tipo );
extern char *nome_maquina ( int soquete );
extern char *endereco_maquina ( int soquete );
extern void child_loop ( int soquete );
extern int server_loop ( int soquete , void *command_list , void *port_list );
extern int envia_mensagem ( int soquete , char *mensagem );
extern int ioperm(int from, int num, int turnon);

extern char *V_servico;
extern char *V_endereco;
extern int V_max_ligacao;
