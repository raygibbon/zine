/***************************************************************************/
/*                               COMPAT.H v1.8                             */
/***************************************************************************/
#ifndef __MY_COMPAT_H__
#define __MY_COMPAT_H__

                     /* IPv4 packet header */
struct ip2 {
#ifdef _BIT_FIELDS_LTOH
             u_char ip_hl:4,   /* Longitud cabecera en grupos de 32 bits */
                     ip_v:4;   /* Version de IP                          */
#else
              u_char ip_v:4,   /* Version de IP                          */ 
                    ip_hl:4;   /* Longitud cabecera en grupos de 32 bits */
#endif  
             u_char  ip_tos;   /* Tipo de servicio                       */
              short  ip_len;   /* Longitud total incluyendo datos        */
            u_short   ip_id;   /* Identificador paquete IP               */
              short  ip_off;   /* Offset si fragmentacion                */
             u_char  ip_ttl;   /* Time To Live                           */
             u_char    ip_p;   /* Protocolo del campo de datos           */
            u_short  ip_sum;   /* Checksum                               */
             struct  in_addr ip_src, /* Origen       */
                     ip_dst;         /* Destino      */
          };


                       /* ICMP packet header */
struct icmp2 {
              struct icmp_hdr
              {
                 u_char  icmp_type;
                 u_char  icmp_code;
                 u_short icmp_cksum;
              } icmp_hdr;
              union {
                       u_char ih_pptr;
                       struct in_addr ih_gwaddr;
                       struct ih_idseq {
                                         n_short  icd_id;
                                         n_short  icd_seq;
                                       } ih_idseq;
                       u_long ih_reserved;
                       struct ih_rdiscovery {
                                              u_char  num_addr;    
                                              u_char  addr_entry_size;
                                              u_short lifetime;
                                            } ih_rdiscovery;
                    } icmp_hun;
              union {
                      struct id_ts {
                                      n_time its_otime;
                                      n_time its_rtime;
                                      n_time its_ttime;
                                   } id_ts;
                      struct id_ip {
                                      struct ip2 idi_ip;
                                   } id_ip;
                      u_long  id_mask;
                      char  id_data[1];
                      struct id_rdiscovery {
                                             struct in_addr router_addr;
                                             struct in_addr pref_level;
                                           } id_rdiscovery;
                    } icmp_dun;
            }; 

#define icmp_type       icmp_hdr.icmp_type
#define icmp_code       icmp_hdr.icmp_code
#define icmp_cksum      icmp_hdr.icmp_cksum
#define	icmp_pptr       icmp_hun.ih_pptr
#define	icmp_gwaddr     icmp_hun.ih_gwaddr
#define	icmp_id	        icmp_hun.ih_idseq.icd_id
#define	icmp_seq        icmp_hun.ih_idseq.icd_seq
#define	icmp_reserved        icmp_hun.ih_reserved
#define icmp_num_addr        icmp_hun.ih_rdiscovery.num_addr
#define icmp_addr_entry_size icmp_hun.ih_rdiscovery.addr_entry_size
#define icmp_lifetime        icmp_hun.ih_rdiscovery.lifetime
#define	icmp_otime      icmp_dun.id_ts.its_otime
#define	icmp_rtime      icmp_dun.id_ts.its_rtime
#define	icmp_ttime      icmp_dun.id_ts.its_ttime
#define	icmp_ip	        icmp_dun.id_ip.idi_ip
#define	icmp_mask       icmp_dun.id_mask
#define icmp_data       icmp_dun.id_data
#define icmp_rdiscovery icmp_dun.id_rdiscovery
#define icmp_rdisc_router icmp_dun.id_rdiscovery.router_addr
#define icmp_rdisc_pref   icmp_dun.id_rdiscovery.pref_level


                   /* UDP packet header */
struct udp_hdr {
                 u_short   source;  /* Puerto origen  */
                 u_short     dest;  /* Puerto destino */
                   short  uh_ulen;  /* Longitud       */
                 u_short   uh_sum;  /* Checksum       */
               };


                   /* TCP packet header */
struct tcp_hdr {
                 u_short source;   /* Puerto origen       */
                 u_short   dest;   /* Puerto destino      */
                 u_long     seq;   /* Numero de secuencia */
               };


                   /* ICMP ECHO packet header */
struct echo_hdr {
                  u_char  type;
                  u_char  code;
                  u_short cksum;
                  u_short id;
                  u_short seq;
               };

                                             
               /* ICMP error data field */
union data_hdr {
                  struct tcp_hdr  cab_tcp;
                  struct udp_hdr  cab_udp;
                  struct echo_hdr cab_echo;
               };


#define TCAB_IP         sizeof(struct ip2)
#define TCAB_ICMP       sizeof(struct icmp2)
#define TCAB_UDP        sizeof(struct udp_hdr)
#define TCAB_TCP        sizeof(struct tcp_hdr)
#define TCAB_ECHO       sizeof(struct echo_hdr)
#define TCAB_ICMP_MSG   TCAB_ECHO
#define TCAB_64DATA     sizeof(union data_hdr)
#define TDATA_TIMESTAMP sizeof(struct id_ts)
#define TDATA_ADDRESS   sizeof(u_long)
#define TCAB_RDISC   sizeof(struct ih_rdiscovery)
#define TDATA_RDISC  sizeof(struct id_rdiscovery)

/* ICMP errors ... */
#define ICMP_DEST_UNREACH    3
#define ICMP_SOURCE_QUENCH   4
#define ICMP_REDIRECT        5
#define ICMP_TIME_EXCEEDED  11
#define ICMP_PARAM_PROB     12

/* ICMP information messages ... */
#define ICMP_ECHO_REPLY        0
#define ICMP_ECHO_REQUEST      8 
#define ICMP_ROUTER_ADVERT     9
#define ICMP_ROUTER_SOLICIT   10
#define ICMP_TIMESTAMP        13 
#define ICMP_TIMESTAMP_REPLY  14 
#define ICMP_INFO_REQUEST     15
#define ICMP_INFO_REPLY       16
#define ICMP_ADDRESS          17
#define ICMP_ADDRESS_REPLY    18

#define TTL_DFL 254        /* Time To Live default */

#define TIMEOUT_DFL 5      /* Timeout default, secs */

#ifdef WATT32              /* Maximum packet size */
  #define SIZE_BIG  1480   /* no fragment Tx support yet */
#elif defined (SOLARIS)
  #define SIZE_BIG 65535
#else 
  #define SIZE_BIG 75000
#endif

#define LIFETIME_DFL   1800  /* Lifetime default of a Router Advertisement */
#define PREFERENCE_DFL 0     /* Preference default of a Router Advertisement*/
#define MAX_ROUTERS ( ( SIZE_BIG - TCAB_IP - sizeof(struct icmp_hdr) \
                        - TCAB_RDISC ) / TDATA_RDISC )
#define ICMP_ERROR  1
#define ICMP_INFO   0

#define ICMP_ERR_DFL_TOS 0

#define NO_REPLY 255   /* ICMP type that doesn't have a reply function */

#define IP_MF 0x2000   /* IP More Fragments flag */

#endif
