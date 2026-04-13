/*
 * Adonis project / software utilities:
 *  Http put/get/.. standalone program using Http put mini lib
 *  written by L. Demailly
 *  (c) 1996 Observatoire de Paris - Meudon - France
 *
 * $Id: http.c,v 1.2 1996/04/18 13:52:14 dl Exp dl $
 *
 * $Log: http.c,v $
 * Revision 1.2  1996/04/18  13:52:14  dl
 * strings.h->string.h
 *
 * Revision 1.1  1996/04/18  12:17:25  dl
 * Initial revision
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <tcp.h>

#include "http_lib.h"

void Usage (void)
{
  puts ("usage: http <cmd> <url>\n"
        "       by <dl@hplyot.obspm.fr>\n"
        "       <cmd> = {get | put | delete | head}");
  exit (1);
}

int main (int argc, char **argv)
{
  int   ret,lg,blocksize,r,i;
  int   debug = 0;
  char  typebuf[70];
  char *data     = NULL;
  char *filename = NULL;
  char *proxy    = NULL;
  enum Actions {
       ERR,
       DOPUT,
       DOGET,
       DODEL,
       DOHEA
     } todo = ERR;

  i = 1;
  if (argc >= 2 && !strcmp(argv[i],"-d"))
  {
    debug = 1;
    argc--;
    i++;
  }
  if (argc != 3)
     Usage();
  
  if (!stricmp(argv[i],"put"))
          todo = DOPUT;
  else if (!stricmp(argv[i],"get"))
          todo = DOGET;
  else if (!stricmp(argv[i],"delete"))
          todo = DODEL;
  else if (!stricmp(argv[i],"head"))
          todo = DOHEA;
  else Usage();

  i++;
  
  if ((proxy = getenv("HTTP_PROXY")) != NULL)
  {
    ret = http_parse_url (proxy,&filename);
    if (ret < 0)
       return (ret);
    http_proxy_server = http_server;
    http_server       = NULL;
    http_proxy_port   = http_port;
  }

  ret = http_parse_url (argv[i],&filename);
  if (ret < 0)
  {
    if (proxy)
       free (http_proxy_server);
    return (ret);
  }

  if (debug)
  {
    tcp_set_debug_state (2);
    dbug_init();
  }
  sock_init();

  switch (todo)
  {
    case DOPUT:
      fprintf (stderr,"reading stdin...\n");
      /* read stdin into memory */
      blocksize = 16384;
      lg = 0;
      if ((data = malloc(blocksize)) == NULL)
         return (3);

      while (1)
      {
        r = read (STDIN_FILENO,data+lg,blocksize-lg);
        if (r <= 0)
           break;
        lg += r;
        if ((3*lg/2) > blocksize)
        {
	  blocksize *= 4;
          fprintf (stderr,
                   "read to date: %9d bytes, reallocating buffer to %9d\n",
                   lg,blocksize);
          if ((data = realloc(data,blocksize)) == NULL)
             return (4);
        }
      }
      fprintf (stderr,"read %d bytes\n",lg);
      ret = http_put (filename,data,lg,0,NULL);
      fprintf (stderr,"res = %d\n",ret);
      break;

    case DOGET:
      ret = http_get (filename,&data,&lg,typebuf);
      fprintf (stderr,"res = %d,type = '%s',lg = %d\n",ret,typebuf,lg);
      fwrite (data,lg,1,stdout);
      break;

    case DOHEA:
      ret = http_head (filename,&lg,typebuf);
      fprintf (stderr,"res = %d,type = '%s',lg = %d\n",ret,typebuf,lg);
      break;

    case DODEL:
      ret = http_delete (filename);
      fprintf (stderr,"res = %d\n",ret);
      break;

    default:
      fprintf (stderr,"impossible todo value = %d\n",todo);
      return (5);
  }
  if (data)
     free (data);
  free (filename);
  free (http_server);
  if (proxy)
     free (http_proxy_server);
  
  return (ret == 201 || ret == 200) ? 0 : ret;
}
