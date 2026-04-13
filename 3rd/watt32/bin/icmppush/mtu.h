#ifndef __MTU_H__
#define __MTU_H__

struct mi_ifaz {  char   ifaz_nombre[IFNAMSIZ];
                  short  ifaz_flags;
                  int    ifaz_mtu;
                  struct sockaddr ifaz_ip;
               };

#define MTU_DFL 1500

#endif
