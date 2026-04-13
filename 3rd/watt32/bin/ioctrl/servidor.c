/*
 * 
 * servidor.c - Servidor para o controlo da placa IO256
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <tcp.h>

#include "config.h"
#include "sock.h"
#include "file.h"
#include "io.h"
#include "util.h"

#undef  DEBUG

int main ( int argc , char *argv[] )
{
   int soquete;
   commands *command_list;
   portos *port_list , *temp;
      
   show_memory_usage();
   
   /* Leitura das opcoes na linha de comando: */
   if ( parse_server_options ( argc , argv ) == ERRO )
     return ( EXIT_FAILURE );

   /* Muda para a directoria fornecida pelo utilizador */
   if ( chdir ( V_root_directory ) == -1 )
     {
	perror (V_root_directory);
	return ( EXIT_FAILURE );
     }

#ifndef WATT32
   /* Poe em Background: */
   switch (fork())
     {
      case -1:
	perror ( "fork()" );
	exit ( EXIT_FAILURE );
      case 0:
	close ( STDIN_FILENO );
	if ( setsid() == -1 )
	  {
	     perror ("setsid()");
	     exit ( EXIT_FAILURE );
	  }
	break;
      default:
	return ( EXIT_SUCCESS );
     }
#endif

   /* Criacao do socket de escuta: */
   if ( cria_socket ( &soquete , "TCP" , V_servico , V_endereco , BIND_TYPE )!= OK )
     return ( EXIT_FAILURE );
   
#ifndef	DEBUG
   /* Redirecciona o stdout/stderr para ficheiros */
   if ( redirecciona_log ( V_stdout_file , V_stderr_file ) == ERRO )
     return ( EXIT_FAILURE );
#endif

   /* Leitura dos comandos */
   if ( read_command_file ( V_command_file , &command_list )==ERRO)
     return ( EXIT_FAILURE );

   /* Criacao da lista onde estao os portos necessarios e os seus
    * valores correntes
    */
   create_port_list ( command_list , &port_list );
   temp = port_list;
   while ( temp != NULL )
     {
	if ( ioperm ( temp->port , 1 , 1 )==-1)
	  {
             fprintf ( stderr ,_LANG("Port %d (0x0%04x): ") ,
                      temp->port , temp->port);
	     perror ("");
	     return ( EXIT_FAILURE );
	  }
	temp = temp->next;
     }
   read_port_list ( V_ports_file , port_list );
   send_for_real = ENVIA;
   if ( init_ports ( command_list , port_list )==ERRO)
     return ( EXIT_FAILURE);
   show_port_list ( port_list , (int *)stdout , NSOCKET );
   show_memory_usage();
#if 0
   /* Comando recebido por um processo filho
    */
    new_signal.sa_handler = sig_command_rcvd;
    sigemptyset (&new_signal.sa_mask );
    new_signal.sa_flags = SA_RESTART;
    sigaction ( SINAL_COMANDO , &new_signal , NULL );
   
    new_signal.sa_handler = SIG_IGN;
    sigemptyset ( &new_signal.sa_mask );
    new_signal.sa_flags = SA_RESTART;
    sigaction ( SIGPIPE , &new_signal , NULL );
#endif

#ifdef SIGPIPE
   signal ( SIGPIPE , SIG_IGN );
#endif

   if ( listen ( soquete , 1 ) == -1 )
     {
	perror ("listen");
	return(EXIT_FAILURE);
     }

   /* Server Loop: */
   server_loop ( soquete , command_list , port_list );
   close_s ( soquete );
   return(EXIT_SUCCESS);
}
