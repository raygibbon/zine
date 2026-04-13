/*
 * servidor_io.c - Acesso a portos de Hardware
 * 
 * 
 * (c) 1998 Nuno Sucena Almeida <slug@student.dee.uc.pt>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#define	outb(v,p)	outp(p,v)

#include "config.h"
#include "util.h"
#include "io.h"
#include "file.h"
#include "sock.h"

int send_for_real=NAO_ENVIA;

int output_bit_to_port ( int port , int *port_value , int bit , int bit_value )
{
   switch ( bit_value )
     {
	/*
	 * Bit a zero -> mascara invertida. pex:
	 * bit 3 a zero:
	 * 00000100 -> 11111011 -> valor & 11111011 -> valor com o bit 3 a zero
	 */
      case 0:
	*port_value = *port_value & ( ~ ( 1 << bit ));
	break;
	/* Bit a um ->  pex:
	 * bit 3 a um:
	 * 00000100 -> valor | 00000100 -> valor com o bit 3 a um
	 */
      case 1:
	*port_value = *port_value | ( 1 << bit );
	break;
      default:
	return ( EXIT_FAILURE );
     }
   if ( send_for_real == ENVIA ) outb ( *port_value , port );
   return ( EXIT_SUCCESS );
}

void show_bits ( int valor , int nbits)
{
   int n;
   for ( n = --nbits ; n>=0; n-- )
	printf ( "%d" , ( valor >> n ) & 1 );
   printf("\n");
}

int execute_command ( void *command_list , void *port_list , char *command , char *value )
{
   commands *current;
   portos *port;
   int ok=0;
   if ( (current = command_list) == NULL )
     return ( ERRO );
   while ( current != NULL )
     {
	if (( strcmp ( current->command_name , command ) == 0 )
	    &&
	    ( strcmp ( current->command_value , value ) == 0 ))
	  {
	     ok=1;
	     break;
	  }
	current = current->next;
     }   
   if ( ok == 1 )
     {
	show_command ( current );
	if ((port = find_port_in_list ( port_list , current->port ))!=NULL)
	  {
	     output_bit_to_port ( current->port , &(port->value) , current->bit , current->bit_value );
	     save_port_list ( V_ports_file , port_list );
/*	     show_port_list ( port_list , (int *)stdout , NSOCKET );*/
	     return ( OK );
	  }
	else
	  return ( ERRO );
     }
   else
     return ( ERRO );
}

int init_ports ( void *command_list , void *port_list )
{
   commands *current=command_list;
   portos *current_port;
   printf ("Setting initial port values...\n");
   while ( current != NULL )
     {
	if ( current->init_value == TRUE )
	  {
             printf (" %20s\t->\t%s\n" , current->command_name ,
                     current->command_value );
/*	     show_command ( current );*/
	     if ( (current_port = find_port_in_list ( port_list , current->port ))==NULL)
	       {
                  fprintf (stderr, "Couldn't find port %d (0x0%0x) in list...\n",
			   current->port , current->port );
		  return (ERRO);
	       }
	     else
	       output_bit_to_port ( current->port , &(current_port->value) ,
				   current->bit , current->bit_value );
	  }
	current = current->next;
     }
   return (OK);
}
