/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "vrfy.h"

extern int verbose;
extern int debug;

char *CurHostName = NULL;         /* remote host we are connecting to */
char *MyHostName  = NULL;         /* my own fully qualified host name */

int   ConnTimeout = CONNTIMEOUT;  /* timeout in secs for connect */
int   ReadTimeout = READTIMEOUT;  /* timeout in secs for read reply */

/*
** SFGETS -- Read an input line, using timeout
** -------------------------------------------
**
**  Returns:
**    Pointer to start of input line.
**    NULL on error (including timeout).
*/

char *sfgets (char *buf,        /* input buffer */
              int   size,       /* size of input buffer */
              void *s)          /* input socket */
{
  char *p  = buf;
  DWORD to = set_timeout (1000*ReadTimeout);

  *p = 0;

  while (!chk_timeout(to) && size)
  {
    int c;

    tcp_tick (NULL);
    if (sock_dataready(s))
    {
      c = sock_getc (s);
      if (c == '\r' || c == '\n')
      {
        if (p > buf)
           return (buf);
      }
      else
      {
        *p++ = c;
        *p = 0;
        size--;
      }
    }
    if (sockerr(s))
    {
      errno = ECONNRESET;
      return (NULL);
    }
  }
  if (!size)
     return (buf);

  errno = ETIMEDOUT;
  return (NULL);
}


/*
** MAKECONNECTION -- Establish SMTP connection to remote host
** ----------------------------------------------------------
**
**  Returns:
**    Status code indicating success or failure.
**
**  Outputs:
**    Sets outfile and infile to the output and input channel.
**    Sets CurHostName to the name of the remote host.
*/

int makeconnection (char  *host,      /* host name to connect to */
                    void **outfile,   /* smtp output channel */
                    void **infile)    /* smtp input channel */
{
  static tcp_Socket sock;
  int    status = 0;
  WORD   port   = 25;
  DWORD  addr   = 0L;
  struct servent *sp;
  struct in_addr  inaddr;
  static char     hostname [MAXHOST+1];

  if ((sp = getservbyname("smtp","tcp")) != NULL)
     port = ntohs (sp->s_port);

  if (!host || host[0] == '\0')
     host = "localhost";

/*
 * Check for dotted quad, potentially within brackets.
 */
  addr = numeric_addr (host);
  if (addr == NOT_DOTTED_QUAD)
      addr = inet_addr (host);

/*
 * Fetch the ip addresses of the given host.
 */
  if (addr != NOT_DOTTED_QUAD)
  {
    struct hostent *hp = gethostbyaddr ((char*)&addr, INADDRSZ, AF_INET);
    if (hp)
       host = hp->h_name;
  }
  else if ((addr = htonl(resolve(host))) == 0L)
  {
    /* no address found by nameserver */
    return (EX_NOHOST);
  }

  strncpy (hostname, host, MAXHOST);
  hostname[MAXHOST] = '\0';
  CurHostName = hostname;
  inaddr.s_addr = addr;

  if (verbose >= 2 || debug)
     printf ("connecting to %s (%s) port %d\n",
             CurHostName, inet_ntoa(inaddr), port);

  if (!tcp_open(&sock,0,ntohl(addr),port,NULL))
     goto sock_err;

  sock_wait_established (&sock,sock_delay,NULL,&status);
  *outfile = &sock;
  *infile  = &sock;

  if (debug)
     printf ("connected to %s\n", CurHostName);

  errno   = 0;
  h_errno = 0;
  return (EX_SUCCESS);

sock_err:
  if ((verbose >= 1 || debug) && sockerr(&sock))
     printf ("\nError: %s\n",sockerr(&sock));
  return (EX_TEMPFAIL);
}



/*
** SETMYHOSTNAME -- Determine own fqdn hostname
** --------------------------------------------
**
**  Returns:
**    None.
**
**  Outputs:
**    Sets MyHostName to the local hostname.
*/

void setmyhostname (void)
{
  static char hostname[MAXHOST+1];
  int status;

  if (!MyHostName)
  {
    status = getmyhostname (hostname);
    if (status != EX_SUCCESS)
    {
      giveresponse (status);
      exit (status);
    }
    MyHostName = hostname;
  }
}

/*
** GETMYHOSTNAME -- Determine own fqdn hostname
** --------------------------------------------
**
**  Returns:
**    Status code indicating success or failure.
**
**  Outputs:
**    Stores hostname in given buffer.
*/

int getmyhostname (char *hostname)  /* buffer to store host name */
{
  struct hostent *hp;

  errno   = 0;
  h_errno = 0;

  if (gethostname(hostname, MAXHOST) < 0)
  {
    perror ("gethostname");
    return (EX_OSERR);
  }
  hostname[MAXHOST] = '\0';

  hp = gethostbyname (hostname);
  if (!hp)
  {
    /* cannot contact nameserver, force retry */
    if (errno == ETIMEDOUT || errno == ECONNREFUSED)
       h_errno = TRY_AGAIN;

    /* nameserver could not resolve name properly */
    if (h_errno == TRY_AGAIN)
       return (EX_TEMPFAIL);

    /* no address found by nameserver */
    return (EX_NOHOST);
  }

  strncpy (hostname, hp->h_name, MAXHOST);
  hostname[MAXHOST] = '\0';
  return (EX_SUCCESS);
}

/*
** INTERNET -- Check whether given name has an internet address
** ------------------------------------------------------------
**
**  Returns:
**    TRUE if an internet address exists.
**    FALSE otherwise.
**
**  The given name can be a dotted quad, perhaps between
**  square brackets. If not, an A resource record must exist.
**
**  Note that we do not check the status after a negative return
**  from gethostbyname. Failure can be due to nameserver timeout,
**  in which case the result is still undecided.
**  Currently we consider this an error, so that we won't retry
**  such host during recursive lookups.
*/

bool internet (char *host)       /* host name to check */
{
  DWORD  addr;
  struct hostent *hp;

  /* check dotted quad between brackets */
  addr = numeric_addr (host);
  if (addr != NOT_DOTTED_QUAD)
     return (TRUE);

  /* check plain dotted quad */
  addr = inet_addr (host);
  if (addr != NOT_DOTTED_QUAD)
     return (TRUE);

  /* check if nameserver can resolve it */
  hp = gethostbyname (host);
  if (hp)
     return (TRUE);

  /* probably not, but could be nameserver timeout */
  return (FALSE);
}

/*
** NUMERIC_ADDR -- Check if we have a dotted quad between brackets
** ---------------------------------------------------------------
**
**  Returns:
**    The numeric address if yes.
**    NOT_DOTTED_QUAD if not.
*/

u_long numeric_addr (char *host)   /* host name to check */
{
  u_long addr;
  char  *p;

  if (host[0] != '[')
     return (NOT_DOTTED_QUAD);

  p = strchr (host+1, ']');
  if (!p || p[1] != '\0')
     return (NOT_DOTTED_QUAD);

  *p   = '\0';
  addr = inet_addr (host+1);
  *p   = ']';

  return (addr);
}
