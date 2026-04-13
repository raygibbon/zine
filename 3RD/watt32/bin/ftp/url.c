/*
 * lftp and utils
 *
 * Parses url and returns 1 if successfully, otherwise 0
 *
 * Based on code from Linux lftp and util
 * Copyright (c) 1996-1997 by Alexander V. Lukyanov (lav@yars.free.net)
 *
 * 26-June-1997 GV
 * Added support for user, password, file and type components
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <arpa/ftp.h>

#include "ftp.h"

/*
 * check if password is specified
 */
static int CheckPassw (struct URL *url)
{
  char *s = strdup (url->user);

  if (!s)
     return (0);

  /* user:passwd
   */
  if (sscanf(url->user,"%[^:]:%s",s,url->pass) == 2)
  {
    strcpy (url->user, s);
    free (s);
    return (1);
  }
  return (0);
}

/*
 * check if user name is specified
 */
static int CheckUser (struct URL *url)
{
  int  n;
  char *s = strdup (url->host);

  if (!s)
     return (0);

  /* user@host:port
   */
  n = sscanf (url->host,"%[^@]@%[^:]:%d",url->user,s,&url->port);
  if (n == 3 || n == 2)
  {
    strcpy (url->host, s);
    free (s);
    return CheckPassw (url);
  }

  /* user@host
   */
  if (sscanf(url->host,"%[^@]@%[^:]",url->user,s) == 2)
  {
    strcpy (url->host, s);
    free (s);
    return CheckPassw (url);
  }

  url->user[0] = '\0';       /* no user name found */

  /* host:port
   */
  if (sscanf(url->host,"%[^:]:%d",s,&url->port) == 2)
     strcpy (url->host, s);
  free (s);
  return (1);
}

/*
 *  ftp://user:passwd@foo.com:12345/pub/pc/files/readme.html;type=<typecode>
 *  ^     ^    ^      ^       ^    ^             ^           ^
 *  proto user passwd serv    port path          file        transfer-type
 *                                                          <a> or <i>
 *  NB: Doesn't handle buffer overruns!
 */
int ParseURL (const char *str, struct URL *url)
{
  int   n;
  char *p, *q;

  url->path[0] = '\0';   /* set defaults */
  url->file[0] = '\0';
  url->user[0] = '\0';
  url->pass[0] = '\0';

  /* first try: protocol://host/path{/}
   */
  n = sscanf (str, "%[a-zA-Z]://%[^/]%[^\n]",
              url->proto, url->host, url->path);

  if (!strstr(str, "://"))
       strcpy (url->proto, "ftp://");
  else strlwr (url->proto);

  q = url->path;
  p = strrchr (q, '/');

  if (p && *q && (strlen(q) > (size_t)(p-q)))
  {
    _strncpy (url->file, ++p, sizeof(url->file)-1);
    *p = '\0';
  }

  q = strchr (q, ';');
  if (q > p)
  {
    *q = '\0';
    switch (*(q+6))
    {
      case 'A':
      case 'a': url->type = TYPE_A;
                break;
      case 'i':
      case 'I': url->type = TYPE_I;
                break;
    }
  }

  if (n == 3 || n == 2)
     return CheckUser (url);

  if (n == EOF &&
      sscanf(str, "%[a-zA-Z]://%[^/]", url->proto, url->host) == 2)
  {
    strlwr (url->proto);
    return CheckUser (url);
  }

  _strncpy (url->host, str, sizeof(url->host)-1);
  return (1);
}

