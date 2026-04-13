/***********************************************************/
/* Functions to determine the MTU value from a network     */
/* interface and the IP address from an outgoing interface */
/***********************************************************/
/* To solve:                                               */
/* - Under Solaris 2.5.1 it's not possible to get the      */
/*   outgoing interface without reading the routing table, */
/*   get the interface name and get the netmask from       */
/*   /etc/netmasks. This implies (among other things) the  */
/*   use of STREAMS with the /dev/ip device ...            */
/*   Maybe some day ;)                                     */
/***********************************************************/

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "portable.h"
#include "mtu.h"
#include "misc.h"

extern u_short more_verbose;

/****************************/
/* Functions prototypes ... */
/****************************/
int get_mtu( struct sockaddr_in * );
int get_iface_out( struct sockaddr_in *, struct sockaddr_in * );
int get_iface_mtu( struct sockaddr * );
int coloca_interfaz( int, char *, struct mi_ifaz * );
int equal( struct sockaddr_in *, struct sockaddr_in * );
extern char *pasa( struct sockaddr_in * );
/*******************************/
/* ...end functions prototypes */
/*******************************/


int get_mtu( struct sockaddr_in *ip2see )
{
  struct sockaddr ip_aux;

  if ( get_iface_out( ip2see, (struct sockaddr_in *)&ip_aux ) )
     return MTU_DFL;

  return ( get_iface_mtu( &ip_aux ) );
}


/**********************************/
/* Try to get the IP address from */
/* the outgoing interface "ala"   */
/* Stevens way. ;)                */
/**********************************/
int get_iface_out( struct sockaddr_in *ip2see, struct sockaddr_in *aux )
{
  int sock_rt, len, on=1;
  struct sockaddr_in iface_out;
 
  iniciamem( (char *)&iface_out, sizeof(struct sockaddr_in));
  
  sock_rt = socket(AF_INET, SOCK_DGRAM, 0 );

  if ( setsockopt( sock_rt, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on) )
       == -1 )
  { 
     if ( more_verbose )
        fprintf(stderr, "setsockopt -> %s\n", sys_errlist[errno] );
     return 1;
  }
  
  if (connect(sock_rt, (struct sockaddr *)ip2see, sizeof(struct sockaddr_in) )
       == -1 )
  {
     if ( more_verbose )
        fprintf( stderr,"connect -> %s\n", sys_errlist[errno] );
     return 1;
  }

  len = sizeof(iface_out);
  if ( getsockname( sock_rt, (struct sockaddr *)&iface_out, &len ) == -1 )
  {
     if ( more_verbose ) 
        fprintf(stderr,"getsockname -> %s\n", sys_errlist[errno] );
     return 1;
  }
  
  close(sock_rt);

  if ( more_verbose )
#ifdef SPANISH
     fprintf(stdout," -> Interfaz de salida = %s\n",
            pasa((struct sockaddr_in *)&iface_out));
#else
     fprintf(stdout," -> Outgoing interface = %s\n",
            pasa((struct sockaddr_in *)&iface_out));
#endif
 
  if ( !iface_out.sin_addr.s_addr )
     return 1;
  
  copymem( (char *)&iface_out, (char *)aux, sizeof(struct sockaddr_in));
  return 0; 
}


/***********************************************/
/* Returns the MTU from a networking interface */
/* searching for the appropriate interface     */
/* on the interfaces kernel list.              */
/* On error returns the default MTU (See       */
/* mtu.h)                                      */
/***********************************************/
int get_iface_mtu( struct sockaddr *ip2see )
{
  struct ifconf ifc;
  struct ifreq *ifaz;
  struct mi_ifaz mi_interfaz;
  char buffer[1024];
  int i, sock_disp;

  if ( (sock_disp = socket(AF_INET, SOCK_DGRAM, 0) ) < 0)
  {
     if ( more_verbose )
        fprintf(stderr, "When trying socket() to kernel -> %s",
                     sys_errlist[errno]);
     return MTU_DFL;
  }
  
  ifc.ifc_len = sizeof(buffer);
  ifc.ifc_buf = buffer;
  if (ioctl( sock_disp, SIOCGIFCONF, &ifc) < 0)
  {
     if ( more_verbose )
        fprintf(stderr, "SIOCGIFCONF -> %s", sys_errlist[errno]);
     return MTU_DFL;
  }
  ifaz = ifc.ifc_req;
  
  for ( i = ifc.ifc_len / sizeof(struct ifreq) ; --i >= 0 ; ifaz++ )
  {
     iniciamem( (char *)&mi_interfaz, sizeof(struct mi_ifaz));
     if ( coloca_interfaz( sock_disp, ifaz->ifr_name, &mi_interfaz ) )
        continue;
     if ( ( mi_interfaz.ifaz_flags & IFF_UP ) &&
          ( equal( (struct sockaddr_in *)&mi_interfaz.ifaz_ip, 
                 (struct sockaddr_in *)ip2see ) ) )
     {
        close(sock_disp);
        return mi_interfaz.ifaz_mtu;
     }
  }
  close(sock_disp);
  return MTU_DFL;
}



/*******************************************/
/* Copy the values from the interface      */
/* "nombre" to "mi_interfaz" struct        */
/*******************************************/
/* Arguments :                             */
/*   - sock_disp : Socket RAW descriptor   */
/*                 with the kernel.        */
/*   - nombre : Interface name.            */
/*   - mi_interfaz : Struct where to put   */
/*                   the interface values. */
/*******************************************/
/* Return : 1 on error, or 0 else other.   */
/*******************************************/
int coloca_interfaz( int sock_disp, char *nombre, struct mi_ifaz *mi_interfaz )
{
  struct ifreq if_aux; /* Estructura "auxiliar" :) */

  strcpy(mi_interfaz->ifaz_nombre, nombre);
  strcpy(if_aux.ifr_name, nombre);
  
  if (ioctl( sock_disp, SIOCGIFADDR, &if_aux) < 0)
  {
     if ( more_verbose )
        fprintf(stderr,"SIOCGIFADDR -> %s\n", sys_errlist[errno]);
     return 1;
  }

  mi_interfaz->ifaz_ip = if_aux.ifr_addr;
     
  strcpy(if_aux.ifr_name, nombre);

  if (ioctl(sock_disp, SIOCGIFMTU, &if_aux) < 0)
  {
     if ( more_verbose )
        fprintf(stderr,"SIOCGIFMTU -> %s\n", sys_errlist[errno]);
     return 1;
  }

#ifdef LINUX
  mi_interfaz->ifaz_mtu = if_aux.ifr_mtu;
#else
  mi_interfaz->ifaz_mtu = if_aux.ifr_metric;
#endif

  strcpy(if_aux.ifr_name, nombre);

  if (ioctl( sock_disp, SIOCGIFFLAGS, &if_aux ) < 0 )
  {
     if ( more_verbose )
        fprintf(stderr,"SIOCGIFFLAGS -> %s\n", sys_errlist[errno]);
     return 1;
  }
  mi_interfaz->ifaz_flags = if_aux.ifr_flags;
 
  return 0;
}


/*****************************/
/* Compares 2 sockaddr_in    */
/*****************************/
/* Returns 1 if eq           */
/* Returns 0 else other      */
/* Don't care about the port */
/*****************************/
int equal( struct sockaddr_in *if_ip, struct sockaddr_in *ip2see )
{
   if ( if_ip->sin_family != ip2see->sin_family )
      return 0;

   if ( if_ip->sin_addr.s_addr != ip2see->sin_addr.s_addr )
      return 0;  
 
   return 1;
}
