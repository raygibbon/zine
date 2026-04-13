/*
 * servidor_util.c - Algumas funcoes úteis para o servidor
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <malloc.h>
#include <tcp.h>

#include "config.h"
#include "io.h"
#include "util.h"
#include "file.h"
#include "sock.h"

void mostra_versao (void)
{
   fprintf (stderr, _LANG("%s %s por %s\n%s %s gcc %s\n\n"),
	    PROGRAM_NAME, PROGRAM_VERSION , AUTHOR_NAME,
            __DATE__ , __TIME__ , VERSION);
}

void show_memory_usage (void)
{
#ifdef mstats
   struct mstats memoria;
   memoria = mstats();
   printf ("bytes_total: %d\nbytes_used: %d\n",
	   memoria.bytes_total , memoria.bytes_used );
#endif
}

void mostra_ajuda(void)
{
   printf (_LANG("\t-r\tDirectoria onde correrá o servidor\n"));
   printf (_LANG("\t-d\tFicheiro de configuração de aparelhos\n"));
   printf (_LANG("\t-a\tFicheiro com os utilizadores\n"));
   printf (_LANG("\t-p\tFicheiro onde são guardados os valores correntes dos portos\n"));
   printf (_LANG("\t-l\tFicheiro de LOG\n"));
   printf (_LANG("\t-w\tFicheiro de erros de LOG\n"));
   printf (_LANG("\t-f\tFicheiro para autenticação dos endereços\n"));
   printf (_LANG("\t-e\tEndereço do interface de escuta\n"));
   printf (_LANG("\t-s\tNome do serviço\n"));
   printf (_LANG("\t-m\tNúmero máximo de ligações simultâneas\n"));
   printf (_LANG("\t-c\tMostra configuração corrente\n"));
   printf (_LANG("\t-v\tMostra a versão do programa\n"));
   printf (_LANG("\t-h\tMostra esta ajuda\n\n"));
}

void mostra_configuracao(void)
{
   printf (_LANG("Root Directory\t\t%s\n"),    V_root_directory);
   printf (_LANG("Device file\t\t%s\n"),       V_command_file);
   printf (_LANG("Users file\t\t%s\n"),        V_password_file);
   printf (_LANG("Temporary Ports File\t%s\n"),V_ports_file);
   printf (_LANG("Normal Log File\t\t%s\n"),   V_stdout_file);
   printf (_LANG("Error Log File\t\t%s\n"),    V_stderr_file );
   printf (_LANG("Address File\t\t%s\n"),      V_address_file);
   printf (_LANG("Listen Address\t\t%s\n"),    V_endereco);
   printf (_LANG("Service name\t\t%s\n"),      V_servico);
   printf (_LANG("Max. Connections\t%d\n"),    V_max_ligacao);
}

int parse_server_options ( int argc , char *argv[] )
{
   char c;
   int mostra_config=0;
   V_command_file = NULL;
   V_password_file = NULL;
   V_ports_file = NULL;
   V_stdout_file = NULL;
   V_stderr_file = NULL;
   V_servico = NULL;
   V_endereco = NULL;
   V_root_directory = NULL;
   V_address_file = NULL;
   V_max_ligacao = V_MAX_LIGACAO;
   mostra_versao();
   opterr = 0;
   while (( c = getopt ( argc , argv , "cvVhHd:a:e:s:p:m:w:l:r:f:" ))!=-1 )
     switch (c)
     {
      case '?' : /* Opcao desconhecida ou invalida */
        fprintf (stderr,
                 _LANG("'-%c' - Opção inválida ou especificada de modo incorrecto\a\n\n"),
		 optopt);
	mostra_ajuda();
	return (ERRO);
	break;
      case 'd': /* Ficheiro de comandos */
	V_command_file = strdup ( optarg );
	break;
      case 'a': /* Ficheiro de palavras-chave */
	V_password_file = strdup ( optarg );
	break;
      case 'p': /* Ficheiro onde se guardam os valores dos portos */
	V_ports_file = strdup ( optarg );
	break;
      case 'l': /* Ficheiro para onde e' feita a captura do stdout */
	V_stdout_file = strdup ( optarg );
	break;
      case 'w': /* Ficheiro para onde e' feita a captura do stderr */
	V_stderr_file = strdup ( optarg );
	break;
      case 'f': /* Ficheiro para autenticação dos endereços */
	V_address_file = strdup ( optarg );
	  break;
      case 'r': /* Directoria onde corre o servidor */
	V_root_directory = strdup ( optarg );
	break;
      case 'e': /* Endereco do interface de escuta */
	V_endereco = strdup ( optarg );
	break;
      case 's': /* Nome do servico de escuta */
	V_servico = strdup ( optarg );
	break;
      case 'm': /* Número máximo de ligações */
	if ( (V_max_ligacao = strtol ( optarg , NULL , 0 )) < 1 )
	  {
             fprintf (stderr,
                      _LANG("Número máximo de ligações '%s' inválido! Usando '%d'\a\n\n"),
		      optarg , V_max_ligacao = V_MAX_LIGACAO );
	  }
	break;
      case 'v': /* Versão do programa */
      case 'V':
	return (ERRO);
	break;
      case 'h': /* Ajuda */
      case 'H':
	mostra_ajuda();
	return (ERRO);
	break;
      case 'c':
	mostra_config = 1;
	break;
      default:
	break;
     }

   if ( V_command_file == NULL ) V_command_file = strdup ( V_COMMAND_FILE );
   if ( V_password_file == NULL ) V_password_file = strdup ( V_PASSWORD_FILE );
   if ( V_endereco == NULL ) V_endereco = strdup ( V_ENDERECO );
   if ( V_servico == NULL ) V_servico = strdup ( V_SERVICO );
   if ( V_ports_file == NULL ) V_ports_file = strdup ( V_PORTS_FILE );
   if ( V_stdout_file == NULL ) V_stdout_file = strdup ( V_STDOUT_FILE );
   if ( V_stderr_file == NULL ) V_stderr_file = strdup ( V_STDERR_FILE );
   if ( V_root_directory == NULL ) V_root_directory = strdup ( V_ROOT_DIRECTORY );
   if ( V_address_file == NULL ) V_address_file = strdup ( V_ADDRESS_FILE );
   if ( mostra_config == 1 )
     mostra_configuracao();
   return (OK);
}

int parse_user_commands ( int soquete , char *command_line , status *list_status , void *commands_list , void *ports_list)
{
   char *command_list[] = { "list" , "exit" , "quit" , "user" , "password" , 
      "help" , "command" , "show" , "ports" , "version" , NULL
   };
   char buffer[MAX_BUFFER];
   char *command , *option , *type, *buffer_line=NULL , *temp;
   commands *current;
   status *current_status , *temp_status;
   int n , comando=0 , error=0;
   if ( (current_status = find_new_connection ( list_status , soquete ))==NULL)
     return ( ERRO );
   buffer_line = strdup ( command_line );
   if ( (command = strtok_r ( buffer_line , USER_COMMAND_SEPARATOR , &temp ))==NULL)
     {
	free ( buffer_line );
	return ( ERRO );
     }
   for ( n=0; command_list[n]!=NULL ; n++)
     if (strcmp (command, command_list[n]) == 0)
     {
	comando=n+1;
	break;
     }
   printf ("%s@%s (%s) => %s\n" ,
	   ( current_status->login !=NULL ) ? current_status->login : UNKNOWN ,
	    current_status->hostname ,
	   current_status->hostip , command);
#if 0
   printf (_LANG("Comando == %d\n") , comando );
#endif
   option = strtok_r ( NULL , USER_COMMAND_SEPARATOR , &temp);
   type = strtok_r ( NULL , USER_COMMAND_SEPARATOR , &temp);
   switch ( comando )
     {
      case 1: /* list */
	if ( current_status->ok != OK ) 
        {
          envia_mensagem (soquete , _LANG("I don't know who you are...\n"));
          break;
        }
	current = commands_list;
	while ( current != NULL )
	  {
	     sprintf ( buffer , "%s -> %s\n" ,
		      current->command_name , current->command_value );
	     envia_mensagem ( soquete , buffer );
	     current = current->next;
	  }
	break;
      case 2: /* exit */
      case 3: /* quit */
	free ( buffer_line );
	return ( SAI );
	break;
      case 4: /* user */
	if ( option != NULL )
	  {
	     free ( current_status->login );
	     current_status->login = strdup ( option );
	     current_status->login_ok = OK;
	     if ( current_status->password_ok == OK )
	       current_status->ok = check_password_file ( V_password_file , current_status->login , current_status->password );
	  }
	else error = envia_mensagem ( soquete , "Need more!\n");
	break;
      case 5: /* password */
	if ( option != NULL )
	  {
	     free ( current_status->password );
	     current_status->password = strdup ( option );
	     current_status->password_ok = OK;
	     if ( current_status->login_ok == OK )
	       current_status->ok = check_password_file ( V_password_file , current_status->login , current_status->password );
	  }
	else error = envia_mensagem ( soquete , "Need more!\n");
	break;
      case 6: /* help */
	for ( n=0 ; command_list[n]!=NULL ; n++)
	  {
	     error = envia_mensagem ( soquete , command_list[n]);
	     error = envia_mensagem ( soquete , " ");
	  }
	error = envia_mensagem ( soquete , "\n" );
	break;
      case 7: /* command */
	if ( current_status->ok != OK ) 
	  { envia_mensagem ( soquete , "I don't know who you are...\n"); break; }
	if ( (option != NULL) && (type != NULL) )
	  {
	     if ( execute_command ( commands_list , ports_list , option , type ) == OK)
	       error = envia_mensagem ( soquete , "Done.\n");
	     else
	       error = envia_mensagem ( soquete , "Problem!\n");
	  }
	else error = envia_mensagem ( soquete , "Need more!\n");
	break;
      case 8: /* show connected users */
	if ( current_status->ok != OK ) 
	  { envia_mensagem ( soquete , "I don't know who you are...\n"); break; }
	temp_status = list_status;
	while ( temp_status != NULL )
	  {
	     sprintf (buffer , "%s@%s (%s) %s\n" ,
		      ( temp_status->login != NULL ) ? temp_status->login : UNKNOWN,
		      temp_status->hostname , temp_status->hostip ,
		      (temp_status->soquete == soquete ) ? "(YOU)" : "" );
	     error = envia_mensagem ( soquete , buffer );
	     temp_status = temp_status->next;
	  }
	break;
      case 9: /* ports */
	if ( current_status->ok != OK ) 
	  { envia_mensagem ( soquete , "I don't know who you are...\n"); break; }
	show_port_list ( ports_list , &soquete , SOCKET );
	break;
      case 10:
        sprintf (buffer, _LANG("%s %s por %s\n%s %s gcc %s\n\n"),
		 PROGRAM_NAME, PROGRAM_VERSION , AUTHOR_NAME,
                 __DATE__ , __TIME__ , VERSION);
	error = envia_mensagem ( soquete , buffer );
	break;
      default:
	error = envia_mensagem ( soquete , "Unknown command\n");
	break;
     }
   free ( buffer_line );
   error = envia_mensagem ( soquete , PROGRAM_NAME );
   if ( current_status->ok == OK )
     error = envia_mensagem ( soquete , " OK>" );
   else
     error = envia_mensagem ( soquete , ">" );
   return ( OK );
}

void create_new_connection ( status **list , status **new_pointer )
{
   status *current=*list , *newe;
   if ( current != NULL )
     while ( current->next != NULL ) current=current->next;
   if ( current == NULL )
     {
	current = ( status *) malloc ( sizeof(status));
	(*list)=current;
	current->previous = NULL;
	current->next = NULL;
     }
   else
     {
	newe = ( status *) malloc ( sizeof(status));
	newe->previous = current;
	current->next = newe;
	newe->next = NULL;
	current = newe;
     }
   current->login = NULL;
   current->password = NULL;
   current->hostname = NULL;
   current->hostip = NULL;
   current->login_ok = NOTOK;
   current->password_ok = NOTOK;
   current->ok = NOTOK;
   (*new_pointer) = current;
}

status *find_new_connection ( status *list , int soquete )
{
   status *current=list;
   while ( current != NULL )
     {
	if ( current->soquete == soquete )
	  return (current);
	current = current->next;
     }
   return (NULL);
}

void free_new_connection ( status **list , int soquete )
{
   status *current, *previous, *next;
   if (*list==NULL)
     return;
   if ( (current = find_new_connection ( *list , soquete ))==NULL)
     return;
   if ((previous = current->previous)!=NULL)
     previous->next = current->next;
   if ((next = current->next)!=NULL)
     next->previous = current->previous;
   if ( current == *list )
     *list = current->next;
   free ( current->login );
   free ( current->password );
   free ( current->hostname );
   free ( current->hostip );
   free ( current );
}

char *craig_strtok ( char **handle, char *string, char *delim )
/*
 * Função cedida por Craig Knudsen <cknudsen@radix.net>
 * 
 * para fazer substituir a função strtok
 * 
 * com ligeiras alterações de formatação
 * 
 */
{
   char *last = *handle;
   char *ptr, *start;
   if ( string == NULL ) 
     {
	string = last;
	if ( string == NULL ) 
	  return ( NULL );
     } 
   else
     *handle = NULL;
   ptr = start = string;
   do 
     {
	if ( strncmp ( ptr, delim, strlen ( delim ) ) == 0 ) 
	  {
	     /* found the token */
	     *ptr = '\0';
	     last = ptr + strlen ( delim );
	     *handle = last;
	     return ( start );
	  }
	ptr++;
     }
   while ( *ptr != '\0' );
   /* not found */
   last = NULL;
   *handle = last;
   return ( string );
}

/* Reentrant string tokenizer.  Generic version.
Copyright (C) 1991, 1996 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Parse S into tokens separated by characters in DELIM.
   If S is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
	char s[] = "-abc-=-def";
	char *sp;
	x = strtok_r(s, "-", &sp);	// x = "abc", sp = "=-def"
	x = strtok_r(NULL, "-=", &sp);	// x = "def", sp = NULL
	x = strtok_r(NULL, "=", &sp);	// x = NULL
		// s = "abc\0-def\0"
*/
char *strtok_r (char *s, const char *delim, char **save_ptr)
{
   char *token;
   
   if (s == NULL)
     s = *save_ptr;
   
   /* Scan leading delimiters.  */
   s += strspn (s, delim);
   if (*s == '\0')
     return NULL;
   
   /* Find the end of the token.  */
   token = s;
   s = strpbrk (token, delim);
   if (s == NULL)
     /* This token finishes the string.  */
     *save_ptr = strchr (token, '\0');
   else
     {
	/* Terminate the token and make *SAVE_PTR point past it.  */
	*s = '\0';
	*save_ptr = s + 1;
     }
   return token;
}
