/********************************************************/
/* These are the functions that parse the command line  */
/* arguments.                                           */
/* The routers IP addresses of a Router Advertisement   */
/* is implemented on a linked list.                     */
/********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "portable.h"
#include "compat.h"
#include "packet.h"
#include "misc.h"

/******************/
/* ICMP codes ... */
/******************/
char *cod_redirect[]={ "net",
                       "host",
                       "serv-net",
                       "serv-host",
                       "max"
                     };

char *cod_time[]={ "ttl",
                   "frag",
                   "max"
                 };
                                      
char *cod_unreach[]={ "net-unreach",  
                      "host-unreach", 
                      "prot-unreach",
                      "port-unreach",
                      "frag-needed",
                      "sroute-fail",
                      "net-unknown",
                      "host-unknown",
                      "host-isolated",
                      "net-ano",
                      "host-ano",
                      "net-unr-tos",
                      "host-unr-tos",
                      "com-admin-prohib",
                      "host-precedence-viol",
                      "precedence-cutoff",
                      "max"
                    };
/**********************/
/* ... end ICMP codes */
/**********************/


extern u_short is_pattern;
extern u_short resolve_it;
extern u_short verbose;
extern u_short more_verbose;
extern char *version;
extern char *prog;
char max_gbg = 0;


/***************************/
/* Functions prototypes... */
/***************************/
void add_router(struct my_pack *, u_long, u_long );
int existe_host( char *, u_long * );
int existe_codigo( char *, char **, int );
void parsea_args( char **, struct my_pack * );

extern void salir( int );
extern void help( void );

extern int get_iface_out( struct sockaddr_in *, struct sockaddr_in * );
void recorre_lista(struct my_pack *);
/*******************************/
/* ...end functions prototypes */
/*******************************/



/************************/
/* Yeaaaaaaaahhh !!     */
/* A parser function !! */
/************************/
void parsea_args( char **args, struct my_pack *packet )
{
   int max_cod=0, ptr;
   char *code_icmp="NOTHING", **array_aux=NULL, *punt, *router, *pref;
   char **aux_args = args;
   u_long router_addr, preference;
   struct sockaddr_in aux;
   struct protoent *proto;


   /**************************************/
   /* This first nasty thing is looking  */
   /* for the verbose options. This      */
   /* makes the output more clear before */
   /* parsing the rest of options.       */
   /* Ok, is'nt a good thing but it      */
   /* works. ;)                          */ 
   /**************************************/
   ++aux_args;
   while ( *aux_args != (char *)NULL )
   {
      if ( !strcmp("-v", *aux_args) || !strcmp("--verbose",*aux_args) )
      {
         verbose = 1;
         aux_args++;
         continue;
      }
      if ( !strcmp("-vv", *aux_args) || !strcmp("--more_verbose",*aux_args) )
      {
         more_verbose = verbose = 1;
         break;
      }
      aux_args++;
   }

#ifdef WATT32
   if (verbose || more_verbose)
      dbug_init();
   sock_init();
#endif


   /************************/
   /* Here begins the true */
   /* parser. :)           */
   /************************/ 
   ++args;
   while ( *args != (char *)NULL )
   {
      if ( !strcmp("-sp", *args) || !strcmp("--spoof", *args) )
      {
         if ( *++args == NULL )
            salir(11);
         if ( existe_host( *args, (u_long *)&(packet->ip_spoof) ) )
            salir(11);
         args++;
         continue;
      }
      
      if ( !strcmp("-c", *args) || !strcmp("--code", *args) )
      {
         if ( *++args == NULL )
            salir(7);
         code_icmp = *args;
         args++;
         continue;
      }

      if ( !strcmp("-prot", *args) || !strcmp("--protocol",*args) )
      {
         if ( *++args == NULL )
            salir(14);
         if ( ( proto = getprotobyname( *args )) == NULL )
            salir(14);
         packet->protocol = proto->p_proto;
         args++;
         continue;
      }      
      
      if ( !strcmp("-psrc", *args) || !strcmp("--port_src",*args) )
      {
         if ( *++args == NULL )
            salir(19);
         packet->p_origen = atoi(*args);
         args++;
         continue;
      }      

      if ( !strcmp("-pdst", *args) || !strcmp("--port_dest",*args) )
      {
         if ( *++args == NULL )
            salir(20);
         packet->p_destino = atoi(*args);
         args++;
         continue;
      }      

      if ( !strcmp("-orig", *args) || !strcmp("--orig_host", *args) )
      {
         if ( *++args == NULL )
            salir(8);
         if ( existe_host( *args, (u_long *)&(packet->orig) ) )
            salir(8);
         args++;
         continue;
      }

      if ( !strcmp("-h", *args) || !strcmp("--help", *args) )
      {
         help();
      }
      
      if ( !strcmp("-V",*args) || !strcmp("--version",*args) )
      {
         fprintf(stdout, "%s %s\n", prog, version );
         exit(0);
      }      

      if ( !strcmp("-du",*args) || !strcmp("--dest_unreach",*args) )
      {
         packet->tipo_icmp = ICMP_DEST_UNREACH;
         array_aux = cod_unreach;
         max_cod = MAX(cod_unreach);
         args++;
         continue;
      }            

      if ( !strcmp("-sq", *args) || !strcmp("--src_quench",*args) )
      {
         packet->tipo_icmp = ICMP_SOURCE_QUENCH;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }

      if ( !strcmp("-red", *args) || !strcmp("--redirect",*args) )
      {
         packet->tipo_icmp = ICMP_REDIRECT;
         array_aux = cod_redirect;
         max_cod = MAX(cod_redirect);
         args++;      
         continue;
      }

      if ( !strcmp("-tx", *args) || !strcmp("--time_exc",*args) )
      {
         packet->tipo_icmp = ICMP_TIME_EXCEEDED;
         array_aux = cod_time;
         max_cod = MAX(cod_time);
         args++;      
         continue;
      }

      if ( !strcmp("-tstamp", *args) || !strcmp("--timestamp",*args) )
      {
         packet->tipo_icmp = ICMP_TIMESTAMP;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }

      if ( !strcmp("-mask", *args) || !strcmp("--mask_req",*args) )
      {
         packet->tipo_icmp = ICMP_ADDRESS;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }

      if ( !strcmp("-info", *args) || !strcmp("--info_req",*args) )
      {
         packet->tipo_icmp = ICMP_INFO_REQUEST;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }

      if ( !strcmp("-rts", *args) || !strcmp("--router_solicit",*args) )
      {
         packet->tipo_icmp = ICMP_ROUTER_SOLICIT;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }

      if ( !strcmp("-rta", *args) || !strcmp("--router_advert",*args) )
      {
         packet->tipo_icmp = ICMP_ROUTER_ADVERT;
         packet->cod_icmp = 0;
         if ( *++args == NULL )
            salir(26);
         punt = *args;
         if ( ( router = strtok( punt, "/")) != NULL )
         {
            if ( existe_host( router, (u_long *)&(router_addr) ) )
               salir(26);
            if ( (pref = strtok( NULL, "/")) != NULL )
               preference = atoi(pref);
            else
               preference = PREFERENCE_DFL;
            add_router( packet, router_addr, preference );
         }
         else
            fprintf(stderr, "strtok() error -> %s\n", sys_errlist[errno]);         
         args++;
         continue;
      }

      if ( !strcmp("-lt", *args) || !strcmp("--lifetime",*args) )
      {
         if ( *++args == NULL )
            salir(21);
         packet->lifetime = atoi(*args);
         args++;      
         continue;
      }

      if ( !strcmp("-echo", *args) || !strcmp("--echo_req",*args) )
      {
         packet->tipo_icmp = ICMP_ECHO_REQUEST;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }

      if ( !strcmp("-to", *args) || !strcmp("--timeout",*args) )
      {
         if ( *++args == NULL )
            salir(21);
         packet->timeout = atoi(*args);
         args++;      
         continue;
      }

      if ( !strcmp("-gw", *args) || !strcmp("--gateway",*args) )
      {
         if ( *++args == NULL )
            salir(5);
         if ( existe_host( *args, (u_long *)&(packet->gway) ) )
            salir(5);
         args++;      
         continue;
      }

      if ( !strcmp("-dest", *args) || !strcmp("--route_dest",*args) )
      {
         if ( *++args == NULL )
            salir(6);
         if ( existe_host( *args, (u_long *)&(packet->dest_red) ) )
            salir(6);
         args++;      
         continue;
      }

      if ( !strcmp("-id", *args) || !strcmp("--echo_id",*args) )
      {
         if ( *++args == NULL )
            salir(22);      
         packet->p_origen = atoi(*args);
         args++;      
         continue;
      }

      if ( !strcmp("-seq", *args) || !strcmp("--echo_seq",*args) )
      {
         if ( *++args == NULL )
            salir(23);
         packet->p_destino = atoi(*args);
         args++;      
         continue;
      }

      if ( !strcmp("-pat", *args) || !strcmp("--pattern",*args) )
      {
         if ( *++args == NULL )
            salir(24);
         packet->pattern = *args;
         is_pattern = !is_pattern;
         packet->size_pattern = strlen(*args);
         if ( packet->size_pattern > ( SIZE_BIG - TCAB_IP - TCAB_ICMP_MSG ) )
            salir(18); // Data 2 big :)
         args++;      
         continue;
      }

      if ( !strcmp("-gbg", *args) || !strcmp("--garbage",*args) )
      {
         if ( *++args == NULL )
            salir(27);
         if ( !strcmp("max", *args) )
         {
            packet->garbage = SIZE_BIG;
            max_gbg = 1;
         }
         else
            packet->garbage = (u_long)atol(*args);
         args++;      
         continue;
      }
      
      if ( !strcmp("-param", *args) || !strcmp("--param_problem",*args) )
      {
         packet->tipo_icmp = ICMP_PARAM_PROB;
         packet->cod_icmp = 0;
         args++;      
         continue;
      }      
      
      if ( !strcmp("-ptr", *args) || !strcmp("--pointer",*args) )
      {
         if ( *++args == NULL )
            salir(28);
         if ( (ptr = atoi(*args)) > 255 )
            salir(28);
         if ( ptr < 0 )
            salir(28);         
         packet->pointer = ptr;
         args++;      
         continue;
      }            

      if ( !strcmp("-n", *args) || !strcmp("--no_resolve",*args) )
      {
         resolve_it = 0;
         args++;      
         continue;
      }
      
      if ( !strcmp("-v", *args) || !strcmp("--verbose",*args) )
      {
         args++;      
         continue;
      }
      
      if ( !strcmp("-vv", *args) || !strcmp("--more_verbose",*args) )
      {
         args++;      
         continue;
      }      

      if ( *(args+1) == NULL )
      {
         if ( packet->tipo_icmp == 255 ) /* Bad icmp type... */
            salir(4);
         if ( existe_host( *args,
              (u_long *)&(packet->destino.sin_addr.s_addr) ) )
            salir(13);
         break;
      }

      if ( verbose )
#ifdef SPANISH
         fprintf(stdout,"¿Qué c*j*n*s es esto? -> %s\n", *args);
#else
         fprintf(stdout,"What the hell is it? -> %s\n", *args);
#endif
      args++;
   } /* End while (args) */

   if ( more_verbose )
      recorre_lista(packet);

   if ( packet->destino.sin_addr.s_addr == 1 ) /* Bad destination host...*/
      salir(13);

   if ( packet->tipo_icmp == 255 ) /* Bad icmp type... */
      salir(4);
   
   if ( packet->ip_spoof == 0 )
   {
      if ( !get_iface_out( (struct sockaddr_in *)&packet->destino,
                           (struct sockaddr_in *)&aux ) )
         packet->ip_spoof = aux.sin_addr.s_addr;
   }

   switch ( packet->tipo_icmp )
   {
      case ICMP_REDIRECT: if ( (packet->cod_icmp = existe_codigo(code_icmp,
                                array_aux, max_cod)) == -1 )
                             salir(7);
                          if ( packet->dest_red == 1 )
                             salir(6);
                          if ( packet->gway == 1 )
                             packet->gway = packet->ip_spoof;
                          if (packet->orig == 1 )
                             packet->orig = packet->destino.sin_addr.s_addr;
                          break;
      case ICMP_DEST_UNREACH:if( (packet->cod_icmp = existe_codigo(code_icmp,
                                  array_aux, max_cod)) == -1 )
                               salir(7);
                             break;
      case ICMP_TIME_EXCEEDED:if( (packet->cod_icmp = existe_codigo(code_icmp,
                                  array_aux, max_cod)) == -1 )
                               salir(7);
                             break;
      case ICMP_ROUTER_ADVERT: packet->timeout = 0;
                      default: 
        packet->cod_icmp = (strcmp(code_icmp,"NOTHING")?atoi(code_icmp):0);
                               break;                             
   }

   if ( packet->orig == 1 )
      packet->orig = packet->ip_spoof;
}


/******************************/
/* Add a router to the linked */
/* list of Advertisements     */
/* entries.  :)               */
/******************************/
void add_router(struct my_pack *init, u_long router_addr, u_long preference)
{
   struct router *cursor = init->router;

   if ( init->router == NULL ) 
   {
      init->router = (struct router *)malloc(sizeof(struct router));
      if ( init->router == NULL )
      {
         fprintf(stderr, "Malloc error -> %s\n", sys_errlist[errno]);
         exit(0);
      }
      init->num_routers++;
      init->router->address = router_addr;
      init->router->pref = preference;
      init->router->next = NULL;
      return;
   }
      
   do
   {
      if ( !cursor->next )
      {
         if ( init->num_routers == (MAX_ROUTERS - 1))
         {
            if ( verbose )
               fprintf(stdout, "Sorry, no more than %d routers allowed.\n",
                             MAX_ROUTERS - 1);
            return;
         }
         cursor->next = (struct router *)malloc(sizeof(struct router));
         if (!cursor)
         {
            fprintf(stderr, "Malloc error -> %s\n", sys_errlist[errno]);
            return;
         }
         init->num_routers++;
         cursor->next->address = router_addr;
         cursor->next->pref = preference;
         cursor->next->next = NULL;
         return;
      }
      cursor = cursor->next;
   }while ( cursor );
}


/**********************/
/* For debug only ... */
/**********************/
void recorre_lista(struct my_pack *init )
{
   struct router *cursor = init->router;
   struct in_addr direc;
   while(cursor)
   { 
      direc.s_addr = cursor->address;
      fprintf(stdout, " -> Router = %s - Pref = %ld\n", inet_ntoa(direc),
       cursor->pref);
      cursor = cursor->next;
   }
}


/************************************/
/* Look for the code of the ICMP    */
/* type within the array_cod.       */
/* Return -1 on error.              */
/* Return the code value on success */
/************************************/
int existe_codigo( char *cod, char **array_cod, int max_cod )
{
  short int bucle;
  for (bucle=0; bucle <= (max_cod-1); bucle++)
     if ( !strcmp( cod, array_cod[bucle]) )
        return bucle;
  return -1;  
}



/********************************************/
/* If nom_host == NULL then we want to know */
/* if the IP (in binary format) is ok.      */
/* If nom_host != NULL then we put the      */
/* IP of nom_host into bin_host.            */
/* Return 1 on error, 0 on succes.          */
/********************************************/
/* We allow the 255.255.255.255 address !!  */
/********************************************/
int existe_host( char *nom_host, u_long *bin_host )
{
  struct hostent *hinfo;
  struct sockaddr_in host_tmp;
  struct in_addr host_binario;
    
  iniciamem( (char *)&host_tmp, sizeof(host_tmp) );
  iniciamem( (char *)&host_binario, sizeof(host_binario) );

  host_tmp.sin_family = AF_INET;
  
  if ( nom_host == NULL )   /* We wanna know if the binary IP is  OK. */
  {
     if ( (hinfo = gethostbyaddr( (char *)bin_host, 4, AF_INET )) )
        return 0;
     else
        return 1;
  }  
  
  if ( inet_aton( nom_host, &host_binario) )
  {
     copymem( (char *)&host_binario, (char *)bin_host, sizeof(host_binario));
     return 0; 
  }

  if ( (hinfo = gethostbyname( nom_host )) ) /* Put nom_host into bin_host */
  {
     copymem(hinfo->h_addr, (char *)&host_tmp.sin_addr, hinfo->h_length);
     copymem( (char *) &host_tmp.sin_addr.s_addr, (char *)bin_host, sizeof( host_tmp.sin_addr.s_addr));
     return 0;
  }

  return 1;
}

