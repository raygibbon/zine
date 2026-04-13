/**********************************************/
/* This is only the help part of the program. */
/* See the man page for further details.      */
/**********************************************/
#include <stdio.h>

extern char *prog;

void help(void)
{
#ifdef SPANISH
  fprintf(stdout,"            Uso:  %s  tipo [opciones]  host\n",prog);
  fprintf(stdout,"Tipo:\n");
  fprintf(stdout,"  -du      Destination Unreach         -echo  Echo Request\n");
  fprintf(stdout,"  -info    Information Request         -mask  Address Mask Request\n");
  fprintf(stdout,"  -rta     Router Advertisement        -rts   Router Solicitation\n");
  fprintf(stdout,"  -red     Redirect                    -sq    Source Quench\n"); 
  fprintf(stdout,"  -tstamp  Timestamp                   -tx    Time Exceeded\n");
  fprintf(stdout,"  -param   Parameter Problem\n");
  fprintf(stdout,"  -v       Modo informativo            -vv    Modo debug\n");  
  fprintf(stdout,"  -h       Pantalla de ayuda           -V     Versión del programa\n\n");
  fprintf(stdout,"Opciones:\n");
  fprintf(stdout,"  -sp  address     Host a usar como spoof.\n");
  fprintf(stdout,"  -to  secs        Timeout en segundos para leer las respuestas.\n");
  fprintf(stdout,"  -n               No usar resolución DNS de hosts.\n");
  fprintf(stdout,"  -lt  secs        Lifetime en segundos de Router Advertisement.\n");
  fprintf(stdout,"  -gw  address     Host gateway de ruta en un ICMP Redirect.\n");
  fprintf(stdout,"  -dest  address   Host destino de ruta en un ICMP Redirect.\n");
  fprintf(stdout,"  -orig  address   Host original en un ICMP error.\n");
  fprintf(stdout,"  -psrc  port      Puerto origen (tcp/udp) a usar en datos de ICMP error.\n");
  fprintf(stdout,"  -pdst  port      Puerto destino (tcp/udp) a usar en datos de ICMP error.\n");
  fprintf(stdout,"  -prot            Protocolo a usar en datos de ICMP error (tcp|udp|icmp).\n");
  fprintf(stdout,"  -id  ident       Identificador en ICMPs de información.\n");
  fprintf(stdout,"  -seq  seq#       Número de secuencia en ICMPs de información.\n");
  fprintf(stdout,"  -pat  patron     Patrón de datos a enviar en un ICMP Echo Request.\n");
  fprintf(stdout,"  -gbg  bytes|max  Cantidad de bytes basura a enviar o máximo(max).\n");
  fprintf(stdout,"  -ptr  byte       Puntero a byte erróneo en ICMP Parameter Problem.\n"); 
  fprintf(stdout,"  -c  cod|num|max  Código de ICMP. Mirar página man para más detalles.\n\n");  
#else
  fprintf(stdout,"           Usage:  %s  type [options]  host\n",prog);
  fprintf(stdout,"Type:\n");
  fprintf(stdout,"  -du      Destination Unreach         -echo  Echo Request\n");
  fprintf(stdout,"  -info    Information Request         -mask  Address Mask Request\n");
  fprintf(stdout,"  -rta     Router Advertisement        -rts   Router Solicitation\n");
  fprintf(stdout,"  -red     Redirect                    -sq    Source Quench\n"); 
  fprintf(stdout,"  -tstamp  Timestamp                   -tx    Time Exceeded\n");
  fprintf(stdout,"  -param   Parameter Problem\n");  
  fprintf(stdout,"  -v       Verbose mode on             -vv    Debug mode on\n");    
  fprintf(stdout,"  -h       This help screen            -V     Program version\n\n");
  fprintf(stdout,"Options:\n");
  fprintf(stdout,"  -sp  address     Spoof host.\n");
  fprintf(stdout,"  -to  secs        Timeout secs to read the replies.\n");
  fprintf(stdout,"  -n               Don't use name resolution.\n");
  fprintf(stdout,"  -lt  secs        Lifetime secs for an ICMP Router Advertisement.\n");
  fprintf(stdout,"  -gw  address     Route gateway host for an ICMP Redirect.\n");
  fprintf(stdout,"  -dest  address   Route destination host for an ICMP Redirect.\n");
  fprintf(stdout,"  -orig  address   Original host for an ICMP error.\n");
  fprintf(stdout,"  -psrc  port      Source port (tcp/udp) of ICMP error data.\n");
  fprintf(stdout,"  -pdst  port      Destination port (tcp/udp) of ICMP error data.\n");
  fprintf(stdout,"  -prot            ICMP error data protocol ( tcp | udp | icmp ).\n");
  fprintf(stdout,"  -id  ident       Identification for an ICMP information message.\n");
  fprintf(stdout,"  -seq  seq#       Sequence number for an ICMP information message.\n");
  fprintf(stdout,"  -pat  pattern    Data pattern to send within an ICMP Echo Request.\n");
  fprintf(stdout,"  -gbg  bytes|max  Number of garbage data bytes to send or maximum(max).\n");
  fprintf(stdout,"  -ptr  byte       Incorrect byte on an ICMP Parameter Problem.\n"); 
  fprintf(stdout,"  -c  code|num|max ICMP code. See the man page for details.\n\n");  
#endif
  exit(0);
} 
