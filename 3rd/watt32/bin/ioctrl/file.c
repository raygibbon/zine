/*
 * servidor_file.c - Acesso a ficheiros para o servidor IO256
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file.h"
#include "sock.h"
#include "util.h"
#include "fnmatch.h"
#include "b64encod.h"

char 
 *V_command_file,
 *V_password_file,
 *V_ports_file,
 *V_stdout_file,
 *V_stderr_file,
 *V_root_directory,
 *V_address_file;

int read_command_file ( char *name , commands **list )
{
   FILE *file;
   char read_line[MAX_LINE_LENGTH];
   commands *current_command=NULL , *temp;
   int number=0 , count=0;
   printf (_LANG("Reading command file...\n"));
   if ( name == NULL ) return(ERRO);
   if ( (file=fopen ( name , "r" ))==NULL)
     {
	perror(name);
	return (ERRO);
     }
   while ( !feof(file))
     {
	if (fscanf ( file, " %[^\n]s" , read_line )==1)
	  {
	     number++;
	     if (read_line[0]!='#')
	       {
		  if ( parse_command_line ( read_line , COMMAND_SEPARATOR , &temp ) == OK )
		    {
		       count++;
		       if ( current_command == NULL )
			 {
			    current_command = temp;
			    current_command->next = NULL;
			    *list = current_command;
			 }
		       else
			 {
			    current_command->next = temp;
			    current_command = current_command->next;
			    current_command->next = NULL;
			 }
		    } /* fim do parse_line */
		  else
                    printf (_LANG("%s %d: %s ...ERROR (line ignored)\a\n") ,
			    name , number , read_line );
	       }
	  } /* fim do fscanf */
     } /* fim do while */
   fclose ( file );
   printf (_LANG("Commands read: %d\n") , count );
   return ( OK );
}

int parse_command_line ( char *read_line , char *separator , commands **line )
{
   char *Line , *part;
   *line = (commands*)malloc ( sizeof ( commands ));
   Line = strdup ( read_line );
   /* Command Name: */
   if ( (part = strtok ( Line , separator ))==NULL)
     return (ERRO);
   (*line)->command_name = strdup ( part );
   /* Command Value: */
   if ( (part = strtok ( NULL , separator ))==NULL)
     return (ERRO);
   (*line)->command_value = strdup ( part );
   /* Command Port: */
   if ( (part = strtok ( NULL , separator ))==NULL)
     return (ERRO);
   if ( ((*line)->port = strtol ( part , NULL , 0 ))==0)
     return ( ERRO);
   /* Command bit: */
   if ( (part = strtok ( NULL , separator ))==NULL)
     return (ERRO);
   (*line)->bit = strtol ( part , NULL , 0 );
   if (( (*line)->bit < 0 ) || (*line)->bit > 7) return (ERRO);
   /* Command bit value: */
   if ( (part = strtok ( NULL , separator ))==NULL)
     return (ERRO);
   (*line)->bit_value = strtol ( part , NULL , 0 );
   if (( (*line)->bit_value != 0 ) && (*line)->bit_value != 1) return (ERRO);
   if ( (part = strtok ( NULL , separator ))==NULL)
     (*line)->init_value = FALSE;
   else
     (*line)->init_value = TRUE;
   return ( OK );
}
		
void show_command ( commands *list )
{
   if ( list == NULL ) return;
   printf (
     _LANG("Device: %s\tOption: %s\nPort: %4d (0x%04x)\tBit: %d\tValue: %d\n"),
     list->command_name , list->command_value,
     list->port , list->port , list->bit , list->bit_value );
}

int check_password_file ( char *nome , char *login , char *chave )
{
   FILE *file;
   char read_line[MAX_LINE_LENGTH];
   char *temp=NULL, *r_login , *r_password , salt[2];
   int certa=0;
   printf (_LANG("Checking password file...\n"));
   if (( login == NULL ) || ( chave == NULL ) || (nome == NULL ))
     return ( ERRO );
   if ( ( file = fopen ( nome , "r" )) == NULL )
     {
	perror (nome);
	return ( ERRO );
     }
   while (!feof(file))
     {
	if (fscanf ( file, " %[^\n]s" , read_line )==1)
	  {
	     if (read_line[0]!='#')
	       {
		  free ( temp );
		  temp = strdup ( read_line );
		  r_login = strtok ( temp , PASSWORD_SEPARATOR );
		  r_password = strtok ( NULL , PASSWORD_SEPARATOR );
		  if (( r_login != NULL ) && ( r_password != NULL ))
		    {
		       salt[0]=r_password[0];
		       salt[1]=r_password[1];
		       if ((strcmp ( r_login , login ) == 0)
			   &&
			   ( strcmp ( r_password , crypt ( chave , salt )) == 0))
			 {
			    certa=1;
			    break;
			 }
		    }
	       }
	  }
     }
   fclose ( file );
   free ( temp );
   if (certa==1 ) 
     return (OK); 
   else 
     return(ERRO);
}

void create_port_list ( commands *listc , portos **listp )
{
   commands *current_command;
   portos *current_port=NULL;
   int count=0;
   current_command = listc;
   *listp = current_port;
   printf (_LANG("Creating Port list...\n"));
   while ( current_command != NULL )
     {
	if ( find_port_in_list( *listp , current_command->port )==NULL)
	  {
	     count++;
#if 0
             printf (_LANG("Command port: %d (0x0%0x)\n") ,
                     current_command->port , current_command->port);
#endif
	     if ( current_port == NULL )
	       {
		  current_port = ( portos *)malloc( sizeof ( portos ));
		  current_port->next=NULL;
		  current_port->port = current_command->port;
		  current_port->value = 0;
		  *listp = current_port;
	       }
	     else
	       {
		  current_port->next = (portos *)malloc ( sizeof (portos));
		  current_port = current_port->next;
		  current_port->port = current_command->port;
		  current_port->value = 0;
		  current_port->next = NULL;
	       }
	  }
	current_command = current_command->next;
     }
   printf (_LANG("Number of Ports: %d\n") , count );
}

portos *find_port_in_list ( portos *list , int port_number )
{
   portos *current=list;
   while ( current != NULL )
     {
	if ( current->port == port_number )
	  return (current);
	current = current->next;
     }
   return (NULL);
}

void show_port_list ( portos *list , int *descriptor , int tipo)
{
   portos *current=list;
   char line[MAX_LINE_LENGTH];
   char bits[9];
   int n;
   while ( current != NULL )
     {
	memset ( bits , '0' , sizeof(bits));
	bits[8] = '\0';
	for ( n=0 ; n<=7 ; n++ )
	  if ((((current->value) >> n ) & 1 ) == 1 ) bits[7-n]='1';
        sprintf ( line , _LANG("Port: %4d (0x%04x) => %4d (0x%04x) [%s]\n"),
		 current->port , current->port , current->value , 
		 current->value , bits );
	if ( tipo == SOCKET )
	  envia_mensagem ( *descriptor , line );
	else
	  fprintf ( (FILE *)descriptor , line );
	current = current->next;
     }
}

void free_command_list ( commands **list )
{
   commands *current, *temp;
   current = *list;
   while ( current != NULL )
     {
	temp = current->next;
	free ( current->command_name );
	free ( current->command_value );
	free ( current );
	current = temp;
     }
}

int save_port_list ( char *name , portos *list )
{
   FILE *file;
   portos *current=list;
   printf (_LANG("Saving port values...\n"));
   if ( (name == NULL ) || ( list == NULL ))
     return (ERRO);
   if ( (file=fopen ( name , "w" ))==NULL)
     {
	perror(name);
	return (ERRO);
     }
   while ( current != NULL )
     {
	fprintf ( file , "%d %d\n" , current->port , current->value);
	current = current->next;
     }
   fclose (file);
   return (OK);
}

int read_port_list ( char *name , portos *list )
{
   FILE *file;
   portos *current;
   int port , value , count=0;
   printf (_LANG("Reading old port values...\n"));
   if ( (name==NULL) || (list ==NULL)) return ( ERRO );
   if ( (file=fopen ( name , "r" ))==NULL)
     {
	perror(name);
	return (ERRO);
     }
   while ( !feof(file) )
     {
	if ( fscanf ( file , "%d %d" , &port , &value ) == 2 )
	  {
	     if ( (current = find_port_in_list ( list , port))!=NULL)
	       {
		  current->value = value;
		  count++;
	       }
	  }
	else
	  fscanf ( file , "%*[^\n]" );
     }
   printf (_LANG("Ports read: %d\n"), count);
   fclose (file);
   return (OK);
}

int redirecciona_log ( char *nome_stdout , char *nome_stderr )
{
   FILE *log1 , *log2;
   if ((log1 = freopen ( nome_stderr , "w" , stderr ))==NULL)
     {
	perror (nome_stderr);
	return ( ERRO );
     }
   setvbuf ( log1 , NULL , _IOLBF , BUFSIZ );
   if ((log2 = freopen ( nome_stdout , "w" , stdout ))==NULL)
     {
	perror (nome_stdout);
	return ( ERRO );
     }
   setvbuf ( log2 , NULL , _IOLBF , BUFSIZ );
   return ( OK );
}


signed int verify_address_on_file ( char *nome , char *endereco )
/*
 * nome - nome do ficheiro que contem os enderecos (wildcards podem ser usadas)
 * endereco - endereco na forma a.b.c.d ou aaaa.bbbb.cccc.dddd
 * 
 * retorna: -> 0 em caso de erro na abertura do ficheiro.
 *                    ou se o endereco nao faz parte do ficheiro
 *          -> -numero de linha  se o endereco e' nao autorizado
 *          -> +numero de linha  se o endereco e' autorizado
 */ 
{
   FILE *ficheiro;
   char read_line[MAX_LINE_LENGTH];
   int contador=0;
   if ( (ficheiro = fopen ( nome , "r" ))==NULL)
     {
	perror(nome);
	return (0);
     }
   while ( !feof ( ficheiro ))
     {
	if ( fscanf ( ficheiro , " %40s" , read_line )==1 )
	  {
	     contador++;
	     switch (read_line[0])
	       {
		case '!': /* endereco negado */
		  if (fnmatch ( &read_line[1], endereco , FNM_CASEFOLD )==0)
		    {
		       fprintf (stderr, "%s (%d) : %s -> %s\n", 
				nome, contador, read_line , endereco);
		       fclose(ficheiro);
		       return (-contador);
		    }
		  break;
		case '#': /* comentario , logo ignora o resto da linha */
		  fscanf ( ficheiro , "%*[^\n]" );
		  break;
		default: /* endereco normal */
		  if ( fnmatch ( read_line, endereco , FNM_CASEFOLD )==0)
		    {
		       fprintf (stderr, "%s (%d) : %s -> %s\n", 
				nome, contador, read_line , endereco);
		       fclose(ficheiro);
		       return (contador);
		    }
		  break;
	       }
	     /* Ignora o resto da linha */
	     fscanf ( ficheiro , "%*[^\n]" );
	  }
     }
   fclose ( ficheiro );
   return (0);
}

int verify_connect_address ( char *nome , void *current )
/*
 * current - strutura da descricao da ligacao 
 * ficheiro - nome do ficheiro onde procurar os enderecos
 * 
 * retorna : OK - se o endereco de ligacao do socket e' autorizado
 *           e ERRO caso contrario
 */

{
   status *temp=(status *)current;
   signed int ok1,ok2;
   int endereco_valido=0;

   if (temp->hostname && temp->hostip)
     {
#if 0
        fprintf (stderr , _LANG("Nome: %s\tIP: %s\n") ,
                 temp->hostname , temp->hostip );
#endif
	ok1 = verify_address_on_file ( nome, temp->hostname);
	ok2 = verify_address_on_file ( nome, temp->hostip);
	if (((ok1==0) && (ok2>0)) || /* um valido e o outro inexistente */
	    ((ok2==0) && (ok1>0)) || /* um valido e o outro inexistente */
	    ((ok1>0) && (ok2>0))) /* <- os dois validos */
	  endereco_valido=1;
	
	/* um valido e outro invalido e faz a comparacao da posicao no
         * ficheiro ,ie, em termos de prioridade :
         */
	if (((ok2>0) && (ok1<0) && (ok2<abs(ok1))) ||
	    ((ok1>0) && (ok2<0) && (ok1<abs(ok2)))) 
	  endereco_valido=1;
     }
   if (endereco_valido==1) 
     return(OK);
   else 
     return(ERRO);
}
