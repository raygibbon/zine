/*
 * config.h - Configuracao para o servidor
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

#define PROGRAM_NAME     "IO256server"
#define PROGRAM_VERSION  "0.1"
#define AUTHOR_NAME      "Nuno Sucena <slug@student.dee.uc.pt>"

/* Separacao dos campos do ficheiro de comandos */
#define COMMAND_SEPARATOR ",; \t"

/* Separacao dos campos do ficheiro de palavras chave */
#define PASSWORD_SEPARATOR ",;: \t"

/* Separacao das palavras fornecidas pelo utilizador ligado */
#define USER_COMMAND_SEPARATOR ",;: \t"

/* Separacao entre sequencias de comandos: */
#define LINE_COMMAND_SEPARATOR "\r\n"

/* Comprimento maximo de uma linha de um ficheiro */
#define MAX_LINE_LENGTH 1024
/* Comprimento maximo de armazenamento temporario */
#define MAX_BUFFER 1024

#define OK 0
#define NOTOK !OK
#define ERRO -1
#define SAI 1

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE !TRUE
#endif

#define V_ADDRESS_FILE "address.cfg"
#define V_COMMAND_FILE "devices.cfg"
#define V_PASSWORD_FILE "password.cfg"
#define V_PORTS_FILE "ports.tmp"
#define V_STDOUT_FILE "servidor.log"
#define V_STDERR_FILE "servidor.err"
#ifndef	__TURBOC__
#define V_ROOT_DIRECTORY "./"
#else
#define V_ROOT_DIRECTORY "."
#endif

/* Endereco do interface de escuta. 0.0.0.0 para todas */
#define V_ENDERECO "0.0.0.0"

/* Nome do servico no /etc/services correspondente ao suporte IO256 */
#define V_SERVICO "io256"

/* Expiracao de uma comunicacao ( em segundos ): */
#define CONNECTION_TIMEOUT 30

/* Número máximo de ligações por defeito: */
#define V_MAX_LIGACAO 10

#define SINAL_COMANDO SIGUSR1

/* Portability hacks for ioctrl */
#if defined(unix) || defined(__DJGPP__)|| defined(__HIGHC__)
#include <unistd.h>
#endif

