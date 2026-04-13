/*
 * Adonis project / software utilities:
 *  Http put/get mini lib
 *  written by L. Demailly
 *  (c) 1996 Observatoire de Paris - Meudon - France
 *
 * $Id: http_lib.c,v 3.3 1996/04/25 19:07:22 dl Exp $ 
 *
 * Description : Use http protocol, connects to server to echange data
 *
 * $Log: http_lib.c,v $
 * Revision 3.3  1996/04/25  19:07:22  dl
 * using intermediate variable for htons (port) so it does not yell
 * on freebsd  (thx pp for report)
 *
 * Revision 3.2  1996/04/24  13:56:08  dl
 * added proxy support through http_proxy_server & http_proxy_port
 * some httpd *needs* cr+lf so provide them
 * simplification + cleanup
 *
 * Revision 3.1  1996/04/18  13:53:13  dl
 * http-tiny release 1.0
 *
 */

/*
 * http_lib - Http data exchanges mini library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tcp.h>

#include "http_lib.h"

#define VERBOSE
#define SERVER_DEFAULT "adonis"


char *http_server       = NULL;   /* string for server name or NULL */
char *http_proxy_server = NULL;   /* proxy server name or NULL */
int   http_port         = 5757;   /* server port number */
int   http_proxy_port   = 0;      /* proxy server port number or 0 */

/* user agent id string */
static char *http_user_agent = "adlib/3 ($Date: 1996/04/25 19:07:22 $)";

typedef enum {
        CLOSE,      /* Close the socket after the query (for put) */
        KEEP_OPEN   /* Keep it open */
      } querymode;

/*
 * read a line from file descriptor
 * returns the number of bytes read. negative if a read error occured
 * before the end of line or the max.
 * cariage returns (CR) are ignored.
 */
STATIC int http_read_line (void *fd,     /* file descriptor to read from */
                           char *buffer, /* placeholder for data */
                           int   max)    /* max number of bytes to read */
{
  int n = 0;

  while (n < max)
  {
    if (sock_read(fd,buffer,1) != 1)
    {
      n = -n;
      break;
    }
    n++;
    if (*buffer == '\r')  /* ignore CR */
       continue;
    if (*buffer == '\n')  /* LF is the separator */
       break;
    buffer++;
  }
  *buffer = 0;
  return (n);
}


/*
 * read data from file descriptor
 * retries reading until the number of bytes requested is read.
 * returns the number of bytes read. negative if a read error (EOF) occured
 * before the requested length.
 */
STATIC int http_read_buffer (fd,buffer,length)
       void *fd;       /* file descriptor to read from */
       char *buffer;   /* placeholder for data */
       int   length;   /* number of bytes to read */
{
  int n, r;

  for (n = 0; n < length; n += r)
  {
    r = sock_read (fd,buffer,length-n);
    if (r <= 0)
       return (-n);
    buffer += r;
  }
  return (n);
}


/*
 * beware that filename+type+rest of header must not exceed MAXBUF
 * so we limit filename to 256 and type to 64 chars in put & get
 */
#define MAXBUF 512

STATIC http_retcode
http_lookup_host (char *host, DWORD *ip, int verbose)
{
  if ((*ip = lookup_host(host,NULL)) == 0)
  {
    if (verbose)
       fprintf (stderr,"Could not resolve host `%s'\n",host);
    return (ERRHOST);
  }
  return (OK0);
}

/*
 * Pseudo general http query
 *
 * send a command and additional headers to the http server.
 * optionally through the proxy (if http_proxy_server and http_proxy_port are
 * set).
 *
 * Limitations: the url is truncated to first 256 chars and
 * the server name to 128 in case of proxy request.
 */
STATIC http_retcode
http_query (
       char      *command,            /* command to send  */
       char      *url,                /* url / filename queried  */
       char      *additional_header,  /* additional header */
       querymode  mode,               /* type of query */
       char      *data,               /* Data to send after header */
       int        length,             /* size of data */
       void     **pfd)                /* returned socket descriptor */
{
  static       tcp_Socket sock;
  void        *s = &sock;
  DWORD        ip;
  char         header[MAXBUF];
  char        *host;
  int          hlg;
  http_retcode ret;
  int          proxy  = (http_proxy_server && http_proxy_port);
  int          port   = proxy ? http_proxy_port : http_port;
  int          status = 0;

  if (pfd)
     *pfd = NULL;

  host = proxy ? http_proxy_server :
         (http_server ? http_server : SERVER_DEFAULT);

  if (http_lookup_host(host,&ip,1) != OK0)
     return (ERRHOST);

  if (!tcp_open(s,0,ip,port,NULL))
     ret = ERRCONN;

  else
  {
    sock_wait_established (s,sock_delay,NULL,&status);

    if (pfd)
       *pfd = s;
    
    /* create header */
    if (proxy)
    {
      hlg = sprintf (header,
                     "%s http://%.128s:%d/%.256s HTTP/1.0\r\n"
                     "User-Agent: %s\r\n%s\r\n",
                     command, http_server, http_port, url,
                     http_user_agent, additional_header);
    }
    else
    {
      hlg = sprintf (header,
                     "%s /%.256s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
                     command, url, http_user_agent, additional_header);
    }
    
    /* send header */
    if (sock_write(s,header,hlg) != hlg)
       ret = ERRWRHD;

    /* send data */
    else if (length && data && sock_write(s,data,length) != length)
            ret = ERRWRDT;
    else
    {
      /* read result & check */
      ret = http_read_line (s,header,MAXBUF-1);
#ifdef VERBOSE
      fputs (header,stderr);
      putc ('\n',stderr);
#endif	
      if (ret <= 0)
              ret = ERRRDHD;
      else if (sscanf(header,"HTTP/1.0 %03d",(int*)&ret) != 1)
              ret = ERRPAHD;
      else if (mode == KEEP_OPEN)
              return (ret);
    }
  }

sock_err:
  if (status == 1 || status == -1)
  {
#ifdef VERBOSE
    fprintf (stderr,"%s\r\n",sockerr(s));
#endif
    return (0);
  }
  sock_close (s);
  return (ret);
}


/*
 * Put data on the server
 *
 * This function sends data to the http data server.
 * The data will be stored under the ressource name filename.
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters 
 *              and type to 64.
 */
http_retcode
http_put (filename, data, length, overwrite, type)
     char *filename;   /* name of the ressource to create */
     char *data;       /* pointer to the data to send   */
     int   length;     /* length of the data to send  */
     int   overwrite;  /* flag to request to overwrite the ressource if it
                          was already existing */
     char *type;       /* type of the data, if NULL default type is used */
{
  char header[MAXBUF];

  if (type)
       sprintf (header,"Content-length: %d\r\nContent-type: %.64s\r\n%s",
                length, type, overwrite ? "Control: overwrite=1\r\n" : "");
  else sprintf (header,"Content-length: %d\r\n%s",length,
                overwrite ? "Control: overwrite=1\r\n" : "");
  return http_query ("PUT",filename,header,CLOSE, data, length, NULL);
}


/*
 * Get data from the server
 *
 * This function gets data from the http data server.
 * The data is read from the ressource named filename.
 * Address of new new allocated memory block is filled in pdata
 * whose length is returned via plength.
 * 
 * returns a negative error code or a positive code from the server
 * 
 *
 * limitations: filename is truncated to first 256 characters
 */
http_retcode
http_get (filename, pdata, plength, typebuf)
     char  *filename;  /* name of the ressource to read */
     char **pdata;     /* address of a pointer variable which will be set
                          to point toward allocated memory containing read data.*/
     int   *plength;   /* address of integer variable which will be set to
                          length of the read data */
     char  *typebuf;   /* allocated buffer where the read data type is returned.
                          If NULL, the type is not returned */
     
{
  http_retcode ret;

  char  header[MAXBUF];
  char *pc;
  void *fd = NULL;
  int   n, length = -1;

  if (!pdata)
       return (ERRNULL);
  else *pdata = NULL;

  if (plength) *plength = 0;
  if (typebuf) *typebuf = '\0';

  ret = http_query ("GET",filename,"",KEEP_OPEN,NULL,0,&fd);
  if (ret == 200)
  {
    while (1)
    {
      n = http_read_line (fd,header,MAXBUF-1);
#ifdef VERBOSE
      fputs (header,stderr);
      putc ('\n',stderr);
#endif	
      if (n <= 0)
      {
        if( fd)
            sock_close (fd);
        return (ERRRDHD);
      }
      /* empty line ? (=> end of header) */
      if (n > 0 && *header =='\0')
         break;

      /* try to parse some keywords : */
      /* convert to lower case 'till a : is found or end of string */
      for (pc = header; *pc != ':' && *pc; pc++)
          *pc = tolower (*pc);
      sscanf (header,"content-length: %d",&length);
      if (typebuf)
         sscanf (header,"content-type: %s",typebuf);
    }
    if (length <= 0)
    {
      sock_close (fd);
      return (ERRNOLG);
    }
    if (plength)
       *plength = length;
    if ((*pdata = malloc(length)) == NULL)
    {
      sock_close (fd);
      return (ERRMEM);
    }
    n = http_read_buffer (fd,*pdata,length);
    sock_close (fd);
    if (n != length)
       ret = ERRRDDT;
  }
  else if (ret >= 0 && fd)
          sock_close (fd);

  return (ret);
}


/*
 * Request the header
 *
 * This function outputs the header of thehttp data server.
 * The header is from the ressource named filename.
 * The length and type of data is eventually returned (like for http_get(3))
 *
 * returns a negative error code or a positive code from the server
 * 
 * limitations: filename is truncated to first 256 characters
 */
http_retcode
http_head (filename, plength, typebuf)
     char *filename; /* name of the resource to read */
     int  *plength;  /* address of integer variable which will be set to
                        length of the data */
     char *typebuf;  /* allocated buffer where the data type is returned.
                        If NULL, the type is not returned */
{
  http_retcode ret;
  
  char  header[MAXBUF];
  char *pc;
  void *fd;
  int   n, length =- 1;

  if (plength) *plength = 0;
  if (typebuf) *typebuf = '\0';

  ret = http_query ("HEAD",filename,"",KEEP_OPEN,NULL,0,&fd);
  if (ret == 200)
  {
    while (1)
    {
      n = http_read_line (&fd,header,MAXBUF-1);
#ifdef VERBOSE
      fputs (header,stderr);
      putc ('\n',stderr);
#endif	
      if (n <= 0)
      {
        sock_close (fd);
        return (ERRRDHD);
      }
      /* empty line ? (=> end of header) */
      if (n > 0 && *header == '\0')
         break;

      /* try to parse some keywords :
       * convert to lower case 'till a : is found or end of string
       */
      for (pc = header; *pc != ':' && *pc; pc++)
          *pc = tolower (*pc);

      sscanf (header,"content-length: %d",&length);
      if (typebuf)
         sscanf (header,"content-type: %s",typebuf);
    }
    if (plength)
       *plength = length;
    sock_close (fd);
  }
  else if (ret >= 0 && fd)
          sock_close (fd);

  return (ret);
}



/*
 * Delete data on the server
 *
 * This function request a DELETE on the http data server.
 *
 * returns a negative error code or a positive code from the server
 *
 * limitations: filename is truncated to first 256 characters 
 */

http_retcode http_delete (char *filename)
{
  return http_query ("DELETE",filename,"",CLOSE,NULL,0,NULL);
}



/* parses an url : setting the http_server and http_port global variables
 * and returning the filename to pass to http_get/put/...
 * returns a negative error code or 0 if sucessfully parsed.
 */
http_retcode
http_parse_url (char *url, char **pfilename)
{
  char *pc, c;
  
  http_port = 80;
  if (http_server)
  {
    free (http_server);
    http_server = NULL;
  }
  if (*pfilename)
  {
    free (*pfilename);
    *pfilename = NULL;
  }
  
  if (strnicmp("http://",url,7))
  {
#ifdef VERBOSE
    fprintf (stderr,"invalid url (must start with 'http://')\n");
#endif
    return (ERRURLH);
  }
  url += 7;
  for (pc = url, c = *pc; c && c != ':' && c != '/'; )
      c = *pc++;

  *(pc-1) = 0;
  if (c == ':')
  {
    if (sscanf(pc,"%d",&http_port)!=1)
    {
#ifdef VERBOSE
      fprintf (stderr,"invalid port in url\n");
#endif
      return (ERRURLP);
    }
    for (pc++; *pc && *pc != '/'; pc++) ;
    if (*pc)
       ++pc;
  }

  http_server = strdup (url);
  *pfilename  = strdup (c ? pc : "");

#ifdef VERBOSE
  fprintf (stderr,"host=(%s), port=%d, filename=(%s)\n",
           http_server,http_port,*pfilename);
#endif
  return (OK0);
}

