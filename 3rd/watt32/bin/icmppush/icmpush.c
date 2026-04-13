/***************************************************************************/
/*                         I C M P U S H . C                          v2.2 */
/***************************************************************************/
/* - Program that allow to send the following ICMP packets fully           */
/*   customized:                                                           */
/*    a) ICMP errors:                                                      */
/*      - Redirect.                                                        */
/*      - Source Quench.                                                   */
/*      - Time Exceeded.                                                   */
/*      - Destination Unreach.                                             */
/*      - Parameter Problem.                                               */
/*    b) ICMP information:                                                 */
/*      - Address Mask Request.                                            */
/*      - Timestamp.                                                       */ 
/*      - Information Request.                                             */
/*      - Echo Request.                                                    */
/*      - Router Solicitation (Router Discovery).                          */
/*      - Router Advertisement (Router Discovery).                         */
/***************************************************************************/
/*  Copyright (C) 1999  Slayer (tcpbgp@softhome.net)                       */
/*    This program is free software; you can redistribute it and/or modify */
/*    it under the terms of the GNU General Public License as published by */
/*    the Free Software Foundation; either version 2 of the License, or    */
/*    (at your option) any later version.                                  */
/*    This program is distributed in the hope that it will be useful,      */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/*    GNU General Public License for more details.                         */
/*    You should have received a copy of the GNU General Public License    */
/*    along with this program; if not, write to the Free Software          */
/*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            */
/***************************************************************************/
/* echo "Ideas & comments" | /bin/mail tcpbgp@softhome.net                 */
/* echo "Destructive opinions & flames" >> /dev/null                       */
/* sorry for the bad english. :)                                           */
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "portable.h"
#include "compat.h"
#include "misc.h"
#include "packet.h"
#include "err_msg.h"

#ifdef SPANISH
#define LNG 0
#else
#define LNG 1
#endif

/************************/
/* Global variables ... */
/************************/
extern char *version;
extern char max_gbg;
char *prog;
u_short verbose = 0;
u_short more_verbose = 0;
u_short is_pattern = 0;
u_short resolve_it = 1;
/****************************/
/* ... end global variables */
/****************************/


#define LINE fprintf(stdout, \
     "-----------------------------------------------------\n")

/***************************/
/* Functions prototypes... */
/***************************/
void adjust_sock( int );
int  envia_icmp_err( char *, struct my_pack * );
int envia_icmp_info( char *, struct my_pack *, u_short, u_short );
int add_garbage( char *, u_long, int );
void fill_garbage( char *, u_long );
void    put_routers( struct my_pack *, struct id_rdiscovery * );
void       envia_ip( int, u_long, struct sockaddr_in *, int, int, int, int,
                     u_char *);
void      send2sock( int, char *, int, struct sockaddr * );

void            dont_print( struct in_addr, struct icmp2 * );
void      print_info_reply( struct in_addr, struct icmp2 * );
void print_timestamp_reply( struct in_addr, struct icmp2 * );
void   print_address_reply( struct in_addr, struct icmp2 * );
void    print_router_reply( struct in_addr, struct icmp2 * );
void      print_echo_reply( struct in_addr, struct icmp2 * );

void  proto_tcp64( union data_hdr *, u_short, u_short );
void  proto_udp64( union data_hdr *, u_short, u_short );
void proto_icmp64( union data_hdr *, u_short, u_short );

void  read_icmp( int, char *, int );
void print_pack( char *, int, u_char, u_char, u_short, u_short );
void print_err_pack ( struct icmp2 * );
int    in_cksum( u_short *, int );
char *pasa( struct sockaddr_in * );
u_long day2milisecs( void );
void      salir( int );
void salir_con_error( int );
void       help( void );
void timeout_func( int );

void init_packet_struct( struct my_pack * );
extern void parsea_args( char **, struct my_pack * );
#ifdef SOLARIS
extern long random();
extern int srandom( unsigned);
#else
extern int get_mtu( struct sockaddr_in * );
#endif
/*******************************/
/* ...end functions prototypes */
/*******************************/


/*************************/
/* ICMP types handler... */
/*************************/
struct icmp_class {
                    char *name;
                    int  class;
                    int  reply;
                    void (*print_packet)( struct in_addr, struct icmp2 *);
                  } icmp_info_or_err[]=
                  {
      { "Echo Reply", ICMP_INFO, NO_REPLY, print_echo_reply },
      { "Not implemented", 0, NO_REPLY, dont_print },
      { "Not implemented", 0, NO_REPLY, dont_print },
      { "Destination Unreach", ICMP_ERROR, NO_REPLY, dont_print },
      { "Source Quench", ICMP_ERROR, NO_REPLY, dont_print },
      { "Redirect", ICMP_ERROR, NO_REPLY, dont_print },
      { "Not implemented", 0, NO_REPLY, dont_print },
      { "Not implemented", 0, NO_REPLY, dont_print },
      { "Echo Request", ICMP_INFO, ICMP_ECHO_REPLY, dont_print },
      { "Router Advertisement", ICMP_INFO, NO_REPLY, print_router_reply },
      { "Router Solicitation", ICMP_INFO, ICMP_ROUTER_ADVERT, dont_print },
      { "Time Exceeded", ICMP_ERROR, NO_REPLY, dont_print },
      { "Parameter Problem", ICMP_ERROR, NO_REPLY, dont_print },
      { "Timestamp Request", ICMP_INFO, ICMP_TIMESTAMP_REPLY, dont_print },
      { "Timestamp Reply", ICMP_INFO, NO_REPLY, print_timestamp_reply },
      { "Info Request", ICMP_INFO, ICMP_INFO_REPLY, dont_print },
      { "Info Reply", ICMP_INFO, NO_REPLY, print_info_reply },
      { "Address Mask Request", ICMP_INFO, ICMP_ADDRESS_REPLY, dont_print },
      { "Address Mask Reply", ICMP_INFO, NO_REPLY, print_address_reply }
                  };



/***********************************/
/* Access to 64 bits protocol data */
/* to build an ICMP error packet.  */
/***********************************/
struct protocols {
                    int proto;
                    void (*func_proto)( union data_hdr *, u_short, u_short );
                 } data_protocols[] = {
                                        { IPPROTO_TCP,  proto_tcp64  },
                                        { IPPROTO_UDP,  proto_udp64  },
                                        { IPPROTO_ICMP, proto_icmp64 }
                                      }; 


/*******************************/
/* Initial function.           */
/* Make the socket and buffers */
/*******************************/
int main(int argc, char **argv)
{
  char buf[SIZE_BIG];
  int  sock, tam_icmp;
  u_short proc_id;
  struct protoent *proto;
  struct my_pack packet;
  
  iniciamem( buf, sizeof(buf) );
  
  prog = argv[0];
  proc_id = getpid();

  if ( argc == 1 )
  {
     fprintf(stdout,"%s %s\n", prog, version);
#ifdef SPANISH     
     fprintf(stdout,"Ejecuta '%s -h' para ver la ayuda.\n",prog);
#else
     fprintf(stdout,"Try '%s -h' to display the help.\n",prog);
#endif     
     exit(0);
  }

  init_packet_struct( (struct my_pack *)&packet );
  
  parsea_args( argv, (struct my_pack *)&packet );


  if ( ( proto = getprotobyname("icmp")) == NULL )
     salir(2);

    /* Socket RAW for the ICMP protocol ... */

  sock = socket( AF_INET, SOCK_RAW, proto->p_proto);

  if ( sock < 0 )
     salir_con_error(3);

#ifndef WATT32
/*****************************
 * This is *NOT* a good idea *
 * Use it at your own risk!  *
 * * * * * * * * * * * * * * * 
  if ( !geteuid() ) 
     setuid( getuid() );
 ****************************/
#endif

  adjust_sock(sock);

  if ( icmp_info_or_err[packet.tipo_icmp].class )  /* ICMP error ... */
     tam_icmp = envia_icmp_err( buf, (struct my_pack *)&packet );
  else
  {
     srandom(time(0));  /* ICMP Information message ... */
     tam_icmp = envia_icmp_info( buf, (struct my_pack *)&packet, proc_id,
                                 random()%0xFFFF );
  }

  if ( more_verbose )
#ifdef SPANISH
     fprintf(stdout," -> Total tamaño ICMP = %d bytes\n", tam_icmp);
#else
     fprintf(stdout," -> ICMP total size = %d bytes\n", tam_icmp);
#endif

  envia_ip( sock, packet.ip_spoof, (struct sockaddr_in *)&packet.destino,
            TTL_DFL, ICMP_ERR_DFL_TOS, IPPROTO_ICMP, tam_icmp, buf );

  if ( verbose )
#ifdef SPANISH     
     fprintf(stdout,"ICMP %s enviado a %s (%s)\n",
             icmp_info_or_err[packet.tipo_icmp].name, argv[argc-1],
              inet_ntoa( packet.destino.sin_addr) );
#else                    
     fprintf(stdout,"ICMP %s packet sent to %s (%s)\n",
             icmp_info_or_err[packet.tipo_icmp].name, argv[argc-1],
              inet_ntoa( packet.destino.sin_addr) );
#endif
                    
   if ( !icmp_info_or_err[packet.tipo_icmp].class && packet.timeout )
   {
      if ( verbose )
#ifdef SPANISH
         fprintf(stdout, "\nRecibiendo respuestas ICMP ...\n");
#else
         fprintf(stdout, "\nReceiving ICMP replies ...\n");
#endif
      read_icmp( sock, buf, packet.timeout );
   } 
   salir(0);
   return(0);
}


/********************************/
/* Adjust the raw socket values */
/* Send and receive buffers,    */
/* broadcasting ...             */
/********************************/
void adjust_sock( int sock )
{
  int on = 1, tam_sock_buf = SIZE_BIG;

  if ( setsockopt( sock, IPPROTO_IP, IP_HDRINCL, (char *)&on, sizeof(on) )
 == -1 )
     salir_con_error(25);

  if ( setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on) )
 == -1 )
     if ( more_verbose)
        fprintf(stderr,"%s: SO_BROADCAST -> %s\n", prog, sys_errlist[errno] );

  if ( setsockopt( sock, SOL_SOCKET, SO_RCVBUF, (char *)&tam_sock_buf,
                   sizeof(tam_sock_buf) ) == -1 )
     if ( more_verbose )
        fprintf(stderr,"%s: SO_RCVBUF -> %s\n", prog, sys_errlist[errno] );

  if (setsockopt( sock, SOL_SOCKET, SO_SNDBUF, (char *)&tam_sock_buf,
                  sizeof(tam_sock_buf) ) == -1 )
     if ( more_verbose )
        fprintf(stderr,"%s: SO_SNDBUF -> %s\n", prog, sys_errlist[errno] );
}



/****************************************/
/* Build an ICMP information packet and */
/* put it into the address 'buffer'.    */ 
/* Return the packet length in bytes.   */
/****************************************/
int envia_icmp_info( char *buffer, struct my_pack *packet, u_short id,
                     u_short seqnum )
{
  int tam = TCAB_ICMP_MSG;
  u_long actual_time;
  struct icmp2 *icp = (struct icmp2 *) buffer;
  
  icp->icmp_type   = packet->tipo_icmp;
  icp->icmp_code   = packet->cod_icmp;
  icp->icmp_cksum  = 0;

  icp->icmp_id = id;
  icp->icmp_seq = seqnum;

  switch ( packet->tipo_icmp )
  {
     case ICMP_TIMESTAMP: icp->icmp_otime = htonl( day2milisecs() );
                          icp->icmp_rtime = 0; 
                          icp->icmp_ttime = 0; 
                          tam += TDATA_TIMESTAMP;
                          break;
     case ICMP_INFO_REQUEST: 
                            break;
     case ICMP_ADDRESS: icp->icmp_mask = htonl(packet->maskaddr);
                        tam += TDATA_ADDRESS;
                        break;
     case ICMP_ROUTER_SOLICIT: icp->icmp_reserved = 0;
                               break;
     case ICMP_ROUTER_ADVERT:
                    icp->icmp_num_addr = packet->num_routers;
                    icp->icmp_addr_entry_size = 2;
                    icp->icmp_lifetime = htons(packet->lifetime);
                    put_routers( packet, (struct id_rdiscovery *) (buffer +
                                sizeof(struct icmp_hdr) + TCAB_RDISC) );
                    tam =  sizeof(struct icmp_hdr) + TCAB_RDISC +
                           (TDATA_RDISC * packet->num_routers) ;
                    break;
     case ICMP_ECHO_REQUEST: if ( packet->size_pattern )
                             {
                                copymem( packet->pattern, icp->icmp_data,
                                         packet->size_pattern );
                                tam += packet->size_pattern;
                             }
                             else
                             {
                                actual_time = htonl( day2milisecs() );
                                copymem( (char *)&actual_time, icp->icmp_data,
                                         sizeof(actual_time) );
                                tam += sizeof(actual_time);
                             }
                             break;
  }
  
  tam = add_garbage( buffer, packet->garbage, tam);
 
  icp->icmp_cksum = in_cksum( (u_short *)icp, tam );
 
  return( tam );
}


/**************************/
/* Control the garbage.   */
/* Return the size of the */
/* garbage added to the   */
/* packet + the ICMP hdr. */
/**************************/
int add_garbage( char *buffer, u_long garbage, int tam )
{
  if ( garbage && ( tam < SIZE_BIG) )
  {
    if ( garbage > ( SIZE_BIG - TCAB_IP - tam ) )
    {
       if ( verbose )
       if ( garbage != SIZE_BIG )
#ifdef SPANISH
          fprintf(stdout," -> Tamaño datos basura demasiado grande, usando m\141ximo (%d bytes)\n",
                         SIZE_BIG - TCAB_IP - tam);
#else
          fprintf(stdout," -> Size of data garbage too big, using maximum (%d bytes)\n",
                         SIZE_BIG - TCAB_IP - tam);
#endif
       fill_garbage( buffer + tam, (SIZE_BIG - TCAB_IP - tam ) );
       tam = (SIZE_BIG - TCAB_IP);
    }
    else
    {
       fill_garbage( buffer + tam, garbage );
       tam += garbage;
    }
  }
  return tam;
}


/***************************/
/* Fill the buffer with    */
/* "data" size of garbage. */
/***************************/
void fill_garbage( char *buf, u_long data )
{
  u_char flag=0;

   if ( more_verbose )
#ifdef SPANISH
      fprintf(stdout," -> Tama\161o datos basura = %ld bytes\n", data );
#else
      fprintf(stdout," -> Data garbage size = %ld bytes\n", data );
#endif

   while ( data-- )
      *buf++ = ((flag=!flag) ? 33 : 72);
}


/***************************/
/* Fetch the routers from  */
/* the linked list and put */
/* them into the buffer.   */
/***************************/
void put_routers( struct my_pack *packet, struct id_rdiscovery *data)
{
   struct router *cursor = packet->router;
   while (cursor)
   {
      data->router_addr.s_addr = cursor->address;
      data->pref_level.s_addr = htonl(cursor->pref);
      cursor = cursor->next;
      ++data;
   }
}


u_long day2milisecs( void )
{
  struct timeval tiempo;

  gettimeofday( (struct timeval *) &tiempo, NULL);
  
  return ( (tiempo.tv_sec%86400) * 1000  /* From 2day secs 2 milisecs */
           +
            tiempo.tv_usec / 1000   /* From microsecs 2 milisecs */
          );                        /* Milisegundos en el mismo segundo */
}


/**************************************************/
/* Build an ICMP error packet and put it into the */
/* address 'buffer'.                              */
/* Also build the IP header and the 64 data bits  */
/* of the original datagram.                      */
/* Return the packet length in bytes.             */
/**************************************************/
int envia_icmp_err ( char *buffer, struct my_pack *packet )
{
  struct icmp2 *icp = (struct icmp2 *) buffer;
  union data_hdr *mix = (union data_hdr *) malloc(TCAB_64DATA);
  int elemento, tam;

  if ( !mix )
      salir(12);

  iniciamem( mix, TCAB_64DATA);
  
  icp->icmp_type   = packet->tipo_icmp;
  icp->icmp_code   = packet->cod_icmp;
  icp->icmp_cksum  = 0;

  icp->icmp_gwaddr.s_addr = 0;
  
  icp->icmp_ip.ip_hl  = TCAB_IP >> 2;
  icp->icmp_ip.ip_v   = 4;
  icp->icmp_ip.ip_tos = 0x0000;
  icp->icmp_ip.ip_len = htons( TCAB_IP + TCAB_ICMP + TCAB_64DATA);
  icp->icmp_ip.ip_id  = htons(0x4A2F);
  icp->icmp_ip.ip_off = 0x0000;
  icp->icmp_ip.ip_ttl = TTL_DFL;
  icp->icmp_ip.ip_p   = packet->protocol;
  icp->icmp_ip.ip_sum = 0x0000;

  switch( packet->tipo_icmp )
  {  
     case ICMP_REDIRECT: icp->icmp_ip.ip_src.s_addr = packet->orig;
                         icp->icmp_ip.ip_dst.s_addr = packet->dest_red;
                         icp->icmp_gwaddr.s_addr = packet->gway;
                         break;
     case ICMP_PARAM_PROB: icp->icmp_pptr = packet->pointer;
                  default: icp->icmp_ip.ip_src.s_addr = packet->destino.sin_addr.s_addr; 
                           icp->icmp_ip.ip_dst.s_addr = packet->orig;
  }

  icp->icmp_ip.ip_sum = in_cksum( (u_short *)(&(icp->icmp_ip)), TCAB_IP );
  
  for ( elemento=0; elemento < MAX(data_protocols); elemento++ )
  {
     if ( packet->protocol == data_protocols[elemento].proto )
     {
        (*data_protocols[elemento].func_proto)( mix, packet->p_origen,
         packet->p_destino);
        elemento=32767;
        break;
     }
  }
  
  if ( elemento != 32767)
     salir(10);

  copymem( mix, icp + 1, TCAB_64DATA ); 

  tam = add_garbage( buffer, packet->garbage, (TCAB_ICMP + TCAB_64DATA) );

  icp->icmp_cksum = in_cksum( (u_short *)icp, tam );

  return( tam );
}


void proto_tcp64( union data_hdr *mix, u_short porto, u_short portd)
{
   mix->cab_tcp.source = htons(porto);      /* Source port         */
   mix->cab_tcp.dest   = htons(portd);      /* Destination port    */
   mix->cab_tcp.seq    = htonl(0xC010C005); /* TCP sequence number */
}

void proto_udp64( union data_hdr *mix, u_short porto, u_short portd )
{
   mix->cab_udp.source  = htons(porto);  /* Source port      */
   mix->cab_udp.dest    = htons(portd);  /* Destination port */
   mix->cab_udp.uh_ulen = htons(12);     /* Length           */
   mix->cab_udp.uh_sum  = htons(0xFA13); /* Checksum         */
}

void proto_icmp64( union data_hdr *mix, u_short identif, u_short num_seq )
{
   mix->cab_echo.type  = ICMP_ECHO_REQUEST;
   mix->cab_echo.code  = 0x0;        /* Echo Request                 */
   mix->cab_echo.cksum = 0x0000;     /* Checksum                     */
   mix->cab_echo.id    = identif;    /* Echo Request ID              */
   mix->cab_echo.seq   = num_seq;    /* Echo Request sequence number */
   mix->cab_echo.cksum = in_cksum( (u_short *)mix, TCAB_64DATA );
}


/******************************************/
/* Build an IP datagram adding the data   */
/* 'paquete' and sending it to the socket */
/* 's'.                                   */
/* Make the fragmentation if necessary,   */
/* getting the MTU value of the outgoing  */
/* interface (linuz only).                */
/******************************************/
void envia_ip( int s, u_long orig, struct sockaddr_in *vict, int ttl, int tos,
               int prot, int lenpack, u_char *paquete )
{
  int mtu = SIZE_BIG-TCAB_IP, len, len_pack = lenpack, offset_pack = 0;
  char buf_ip[SIZE_BIG];
  struct ip2 *ip_p = (struct ip2 *) buf_ip;
  
  iniciamem( buf_ip, SIZE_BIG );

  ip_p->ip_hl  = TCAB_IP >> 2;
  ip_p->ip_v   = 4;
  ip_p->ip_tos = tos;
  ip_p->ip_ttl = ttl;
  ip_p->ip_id  = htons(0x3372);
  ip_p->ip_p   = prot;
/* If you want to calculate the IP header checksum
 * you must delete the comments of the next line
 * because the Linux kernel fill in automatically:
 *
 *  ip_p->ip_sum = in_cksum( (u_short *)ip_p, TCAB_IP );   */  
  ip_p->ip_sum = 0x0000;
  ip_p->ip_src.s_addr = orig;
  ip_p->ip_dst.s_addr = vict->sin_addr.s_addr;

#ifdef LINUX
  mtu = get_mtu( vict ) - TCAB_IP;
#endif

  if ( more_verbose )
  {
     fprintf(stdout, " -> MTU = %d bytes\n", mtu + TCAB_IP);
#ifdef SPANISH  
     fprintf(stdout," -> Longitud total (ICMP + IP) = %d bytes\n", len_pack
             + TCAB_IP);
#else
     fprintf(stdout," -> Total packet size (ICMP + IP) = %d bytes\n",
             len_pack + TCAB_IP);
#endif
  }
               
  while ( len_pack > 0 )
  {
     len = len_pack;  
     if ( len > mtu )
        len = mtu;
     copymem( paquete, ip_p + 1, len ); // Copy the data ... 
     ip_p->ip_len = htons(TCAB_IP + len); 
     ip_p->ip_off = htons(offset_pack >> 3);
     len_pack-=len;

#ifdef LINUX     
     if ( (offset_pack + mtu) <= 65515 ) /* 65535 - 20 minimum IP header */
     {
#endif     
        if ( len_pack > 0 )
           ip_p->ip_off |= htons(IP_MF);
        send2sock( s, buf_ip, ntohs(ip_p->ip_len), (struct sockaddr *)vict );
#ifdef LINUX        
     }      
     else
     {            /* Last packet must *NOT* have the IP_MF */
        if ( verbose && !max_gbg && ( lenpack > (offset_pack + mtu) ) )
#ifdef SPANISH
           fprintf(stderr," -> ¡Lo siento! No puedo enviar más de %d bytes en total\n",
                   offset_pack + mtu + TCAB_IP);
#else        
           fprintf(stderr," -> Sorry! Cannot send more than %d total bytes\n",
                   offset_pack + mtu + TCAB_IP);
#endif
        send2sock( s, buf_ip, ntohs(ip_p->ip_len), (struct sockaddr *)vict );
        break;
     }
#endif     
     paquete+=len;                              
     offset_pack+=len;
  }
}


/***************************/
/* Send datalen 'len' from */
/* 'buf_ip' to socket 's'. */
/***************************/
void send2sock( int s, char *buf_ip, int len, struct sockaddr *host )
{
  int n;
  n = sendto( s, buf_ip, len, 0, host, sizeof(struct sockaddr_in) );

  if ( n < 0 )
     salir_con_error(9);
  
  if ( (n != len) && verbose)
#ifdef SPANISH
      fprintf(stderr,"!!Cuidado!! --> Bytes enviados = %d\n",n);
#else
      fprintf(stderr,"Warning!! --> Bytes sent = %d\n",n);
#endif              
}



/*************************/
/* Read ICMP packets ... */
/*************************/
sigjmp_buf myjmp;
void read_icmp( int sock_lec, char *buf_snd, int timeout )
{
  char buf_rcv[SIZE_BIG];
  sigset_t mysignal_set;
  struct sockaddr origen;
  struct icmp2 *icp = (struct icmp2 *) buf_snd;
  int n, tam = sizeof(struct sockaddr);

  sigemptyset( &mysignal_set );
  sigaddset( &mysignal_set, SIGALRM );

#ifdef SIGALRM
  if ( signal( SIGALRM, timeout_func ) == SIG_ERR )
     salir(17);

  alarm( timeout );  
#endif

  for ( ; ; )  
  {
     if ( sigsetjmp(myjmp, 1) != 0 )
        break;
     iniciamem( buf_rcv, SIZE_BIG );

     sigprocmask(SIG_UNBLOCK, &mysignal_set, NULL); /* Unblock ... */
     n = recvfrom( sock_lec, buf_rcv, sizeof(buf_rcv), 0,
                   &origen, &tam);
     sigprocmask(SIG_BLOCK, &mysignal_set, NULL);  /* ... Block */
     
     if ( n < 0 )
          salir(16);   /* recvfrom error */

     print_pack( buf_rcv, n, icp->icmp_type, icp->icmp_code, icp->icmp_id, 
                 icp->icmp_seq );
  }
}


/********************************/
/* Print a received ICMP packet */
/********************************/
void print_pack( char *buf_rcv, int n, u_char type, u_char code, u_short id,
                u_short seqnum )
{
  struct icmp2 *icmp_rcv;
  struct ip2 *ip = (struct ip2 *) buf_rcv;
  struct hostent *thishost;
  
  icmp_rcv = (struct icmp2 *)((char *)ip + (ip->ip_hl << 2) );

  if ( (icmp_rcv->icmp_type > MAX(icmp_info_or_err) )
      || 
       (icmp_rcv->icmp_type != (icmp_info_or_err[type].reply) ) 
     )
  {
     if ( more_verbose )
     {
        LINE;
        fprintf(stdout, "%s ...\n", inet_ntoa(ip->ip_src));
        print_err_pack( icmp_rcv );
        LINE;
     }
     return;
  }

  if ( icmp_rcv->icmp_type != ICMP_ROUTER_ADVERT )
  {
     if ( icmp_rcv->icmp_id != id ) /* Control of ID and Seq can't be made */
     {                              /* within an ICMP Router Advertismenet */ 
        LINE;
#ifdef SPANISH
        fprintf(stdout, "%-15s -> Id de ICMP incorrecto...\n",
                inet_ntoa(ip->ip_src) );
#else
        fprintf(stdout, "%-15s -> Wrong ICMP Id...\n",
                inet_ntoa(ip->ip_src) );
#endif
        if ( more_verbose )
           print_err_pack ( icmp_rcv );

        LINE;
        return; 
     }

     if ( icmp_rcv->icmp_seq != seqnum )
     { 
        LINE;
#ifdef SPANISH
        fprintf(stdout,"%-15s -> Número de secuencia incorrecto...\n",
             inet_ntoa(ip->ip_src));
#else
        fprintf(stdout,"%-15s -> Incorrect Sequence number...\n",
             inet_ntoa(ip->ip_src));
#endif

        if ( more_verbose )
           print_err_pack( icmp_rcv );

        LINE;
        return;
     }
  }

  if ( resolve_it )
  {
     thishost = gethostbyaddr((char *)&ip->ip_src, 4, AF_INET );
     if ( !thishost )
        fprintf(stdout, "%-15s ", inet_ntoa(ip->ip_src));
     else 
        fprintf(stdout, "%-15s ", thishost->h_name);
  }
  else
     fprintf(stdout, "%-15s ", inet_ntoa(ip->ip_src));

     /* Go to the appropiate ICMP print reply function ...*/ 
  (*icmp_info_or_err[icmp_rcv->icmp_type].print_packet)
                                                     ( ip->ip_src, icmp_rcv );
}


void dont_print( struct in_addr ip_src, struct icmp2 *icmp_rcv )
{
#ifdef SPANISH
   fprintf(stdout, "(%s 0x%X) -> Comorrl? No es posible\n",
                   icmp_info_or_err[icmp_rcv->icmp_type].name,
                   icmp_rcv->icmp_type);
#else
   fprintf(stdout, "(%s 0x%X) -> Uuuu? Not possible\n",
                   icmp_info_or_err[icmp_rcv->icmp_type].name,
                   icmp_rcv->icmp_type);
#endif
}


/***********************************/
/* Print an ICMP Info Reply packet */
/***********************************/
void print_info_reply( struct in_addr ip_src, struct icmp2 *icmp_rcv )
{
   fprintf(stdout, "-> %s\n", icmp_info_or_err[icmp_rcv->icmp_type].name);
}


/*****************************************/
/* Print an ICMP Time stamp Reply packet */
/*****************************************/
void print_timestamp_reply( struct in_addr ip_src, struct icmp2 *icmp_rcv )
{
   struct tm *tiempo;
   u_long aux_temp;

      /* Time when this reply was sent by the dest ... */
   aux_temp = ntohl(icmp_rcv->icmp_ttime)/1000;
   tiempo = localtime((time_t *)&aux_temp);

   if ( more_verbose )
   {
#ifdef SPANISH
      fprintf(stdout, "-> %s transmitido a las %.2d:%.2d:%.2d\n",
            icmp_info_or_err[icmp_rcv->icmp_type].name, tiempo->tm_hour,
            tiempo->tm_min, tiempo->tm_sec);
#else
      fprintf(stdout, "-> %s transmited at %.2d:%.2d:%.2d\n",
            icmp_info_or_err[icmp_rcv->icmp_type].name, tiempo->tm_hour,
            tiempo->tm_min, tiempo->tm_sec);
#endif   
   }
   else
      fprintf(stdout, "-> %.2d:%.2d:%.2d\n", tiempo->tm_hour,
                       tiempo->tm_min, tiempo->tm_sec);
}


/*******************************************/
/* Print an ICMP Address Mask Reply packet */
/*******************************************/
void print_address_reply( struct in_addr ip_src, struct icmp2 *icmp_rcv )
{
   struct in_addr mascara;
   mascara.s_addr = icmp_rcv->icmp_mask;

   if ( more_verbose )
      fprintf(stdout, "-> %s (%s)\n",
          icmp_info_or_err[icmp_rcv->icmp_type].name, inet_ntoa(mascara));
   else
      fprintf(stdout, "-> %s\n", inet_ntoa( mascara ) );
}


/*********************************************/
/* Print an ICMP Router Advertisement packet */
/*********************************************/
void print_router_reply( struct in_addr ip_src, struct icmp2 *icmp_rcv )
{
   int entry;
   u_int life;
   struct id_rdiscovery *data_rdisc;
   struct hostent *thishost;
   
   fprintf(stdout, "-> %s\n", icmp_info_or_err[icmp_rcv->icmp_type].name);

   data_rdisc = (struct id_rdiscovery *) &icmp_rcv->icmp_rdiscovery;
   
   for ( entry=1; entry <= icmp_rcv->icmp_num_addr; entry++, data_rdisc++)
   {
#ifdef SPANISH
      fprintf(stdout, "           Dirección ");
#else
      fprintf(stdout, "             Address ");
#endif
      if ( resolve_it )
      {
         thishost = gethostbyaddr((char *)&data_rdisc->router_addr, 4,
                                   AF_INET );
         if ( !thishost )
            fprintf(stdout, "%-2d= %s ", entry,
                                       inet_ntoa(data_rdisc->router_addr));
         else 
            fprintf(stdout, "%-2d= %s ", entry, thishost->h_name);
      }
      else
         fprintf(stdout, "%-2d= %s ", entry,
                                  inet_ntoa(data_rdisc->router_addr));
      fprintf(stdout, " Prefer. -> %ld\n",
                      ntohl(data_rdisc->pref_level.s_addr) );
    }

    fprintf(stdout, "              Lifetime = ");

    life = ntohs(icmp_rcv->icmp_lifetime);

    if ( life < 60 )
       fprintf(stdout, "%02d secs\n", life);
    else
    {
       if ( life < 3600 )
          fprintf(stdout, "%02d:%02d min\n", life / 60, life % 60);
       else
          fprintf(stdout, "%02d:%02d:%02d hours\n", life / 3600,
                   (life % 3600) / 60, life % 60);
    }
}


/***********************************/
/* Print an ICMP Echo Reply packet */
/***********************************/
void print_echo_reply( struct in_addr ip_src, struct icmp2 *icmp_rcv )
{
   u_long sent_time;
  
   if ( more_verbose && !is_pattern ) 
   {
      /* Time when our request was sent by us ... */
      copymem( icmp_rcv->icmp_data, (char *)&sent_time, sizeof(sent_time));
#ifdef SPANISH
      fprintf(stdout, "-> %s con RTT = %.1f ms\n",
              icmp_info_or_err[icmp_rcv->icmp_type].name, 
              (float)(day2milisecs() - ntohl(sent_time)) );
#else
      fprintf(stdout, "-> %s with RTT = %.1f ms\n",
              icmp_info_or_err[icmp_rcv->icmp_type].name,
              (float)(day2milisecs() - ntohl(sent_time)) );
#endif
   }   
   else
   {
      if ( is_pattern )
         fprintf(stdout, "-> %s\n",
                  icmp_info_or_err[icmp_rcv->icmp_type].name);
      else
      {
         copymem( icmp_rcv->icmp_data, (char *)&sent_time, sizeof(sent_time));
         fprintf(stdout, "-> %.1f ms\n",
              (float)(day2milisecs() - ntohl(sent_time)) );
      }
   }
}



/*****************************/
/* Print an ICMP packet that */
/* doesn't correspond to us  */
/*****************************/
void print_err_pack( struct icmp2 *icmp_rcv )
{
#ifdef SPANISH
  char *type = "Desconocido";
#else
  char *type = "Unknown";
#endif
  if ( icmp_rcv->icmp_type <= MAX(icmp_info_or_err) )
     type = icmp_info_or_err[icmp_rcv->icmp_type].name;

#ifdef SPANISH
  fprintf(stdout,"         Tipo = %s (0x%X)\n", type, icmp_rcv->icmp_type);
  fprintf(stdout,"   Código = 0x%-4X  Checksum = 0x%X\n", icmp_rcv->icmp_code,
                  icmp_rcv->icmp_cksum);
  fprintf(stdout,"   Ident. = 0x%-4X   Num_sec = 0x%X\n", icmp_rcv->icmp_id,
                  icmp_rcv->icmp_seq);
#else
  fprintf(stdout, "         Type = %s (0x%X)\n", type, icmp_rcv->icmp_type);
  fprintf(stdout, "   Code = 0x%-4X  Checksum = 0x%X\n",
                   icmp_rcv->icmp_code, icmp_rcv->icmp_cksum);
  fprintf(stdout, "     Id = 0x%-4X      Seq# = 0x%X\n", icmp_rcv->icmp_id,
                  icmp_rcv->icmp_seq);
#endif
}


/***********************/
/* Our SIGALRM handler */
/**********************/
void timeout_func( int nothing )
{
   /* Do nothing ... */
   siglongjmp( myjmp, 1);
}



/**********************************************/
/* This function has been obtained from the   */
/* book "UNIX NETWORK PROGRAMMING" (1st Ed.), */
/* by Richard W. Stevens (Hyper-Guru).        */
/* ;)                                         */
/**********************************************/
int in_cksum( u_short *p, int n)
{
  register u_short answer;
  register long sum = 0;
  u_short odd_byte = 0;

  while( n > 1 )
  {
     sum += *p++;
     n -= 2;
  }
  /* mop up an odd byte, if necessary */
  if( n == 1 )
  {
      *(u_char *)(&odd_byte) = *(u_char *)p;
      sum += odd_byte;
  }

  sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
  sum += (sum >> 16);			/* add carry */
  answer = ~sum;			/* ones-complement, truncate*/
  return (answer);
}


char *pasa( struct sockaddr_in *sin )
{
  return ((char *) inet_ntoa(sin->sin_addr));
}


/***************************************/
/* Exit from program without returning */
/* a system error. The returned code   */
/*  ==  num_error.                     */
/***************************************/
void salir( int num_error )
{
  if ( num_error || verbose )
     fprintf(stderr, "%s: %s\n", prog, mensaje_err[num_error][LNG]);
  exit(num_error);
}


/***********************************/
/* Exit from program returning the */
/* system error.                   */
/* Return num_error to the system. */
/***********************************/
void salir_con_error( int num_error )
{
  fprintf(stderr,"%s: %s -> %s\n", prog, mensaje_err[num_error][LNG],
          sys_errlist[errno] );
  exit(num_error);
}


/****************************/
/* Init my packet struct    */
/* with the default values. */
/****************************/
void init_packet_struct( struct my_pack *packet )
{
   packet->ip_spoof = 0x0000;
   packet->destino.sin_family = AF_INET;
   packet->destino.sin_addr.s_addr = 0x0001;
   packet->gway = 0x0001;
   packet->dest_red = 0x0001;
   packet->orig = 0x0001;
   packet->cod_icmp = 0;
   packet->tipo_icmp = 255;
   packet->protocol = IPPROTO_TCP;
   packet->p_origen = 0;
   packet->p_destino = 0;
   packet->maskaddr = 0;
   packet->router = NULL;
   packet->lifetime = LIFETIME_DFL;
   packet->num_routers = 0;
   packet->size_pattern = 0;
   packet->timeout = TIMEOUT_DFL;
   packet->garbage = 0;
   packet->pointer = 0;
}
