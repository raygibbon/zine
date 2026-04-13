#ifndef __PACKET_H__
#define __PACKET_H__

 /* Linked list of Routers within an ICMP Router Advertisement */
struct router {
                u_long address;
                u_long pref;
                struct router *next;
              };
              
struct my_pack { u_long ip_spoof;
                 struct sockaddr_in destino;
                 u_long gway;
                 u_long dest_red;
                 u_long orig;
                 int cod_icmp;
                 u_short tipo_icmp;
                 u_short protocol;
                 u_short p_origen;
                 u_short p_destino;
                 u_long maskaddr;
                 struct router *router;
                 u_short lifetime;
                 u_short num_routers;
                 char *pattern;
                 int size_pattern;
                 int timeout;
                 u_char pointer;
                 u_long garbage;
               };
 
 #endif