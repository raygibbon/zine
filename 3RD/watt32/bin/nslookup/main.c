/*
 * ++Copyright++ 1985, 1989
 * -
 * Copyright (c) 1985, 1989
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *   This product includes software developed by the University of
 *   California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

/*
 *  main.c --
 *
 *  Main routine and some action routines for the name server
 *  lookup program.
 *
 *  Andrew Cherenson
 *  U.C. Berkeley Computer Science Div.
 *  CS298-26, Fall 1985
 *
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>

#include "res.h"
#include "paths.h"

/*
 * Name of a top-level name server. Can be changed with
 * the "set root" command.
 */

#ifndef ROOT_SERVER
#define ROOT_SERVER "a.root-servers.net."
#endif

char rootServerName[NAME_LEN] = ROOT_SERVER;

/*
 *  Info about the most recently queried host.
 */

HostInfo curHostInfo;
int      curHostValid = FALSE;

/*
 *  Info about the default name server.
 */

HostInfo      *defaultPtr = NULL;
char           defaultServer[NAME_LEN];
struct in_addr defaultAddr;

/*
 *  Initial name server query type is Address.
 */

int queryType  = T_A;
int queryClass = C_IN;

/*
 * Stuff for Interrupt (control-C) signal handler.
 */

FILE   *filePtr;
jmp_buf env;


/*
 * Browser command for help and view.
 */
char *pager;


/*
 * Prototype locals
 */
static void Usage       (void);
static void res_re_init (void);
static void res_dnsrch  (char *cp);
static void ShowOptions (void);
static void CvtAddrToPtr(char *host);
static void ReadRCfile  (void);


/*
 *  main --
 *
 *  Initializes the resolver library and determines the address
 *  of the initial name server. The yylex routine is used to
 *  read and perform commands.
 *
 */

int main (int argc, char **argv)
{
  char  *wantedHost = NULL;
  int    i, result;
  struct hostent  *hp;

#ifdef DEBUG  /* copied from below */
  _res.options = (RES_DEBUG | RES_DEBUG2 | RES_INIT | RES_DEFAULT);
#endif

  if (argc > 1 && !strcmp(argv[1],"-d"))
  {
    dbug_init();
    ++argv;
    --argc;
  }
  sock_init();
  yyin = stdin;

  /*
   *  Initialize the resolver library routines.
   */

  if (res_init() == -1)
  {
    fprintf (stderr,"*** Can't initialize resolver.\n");
    return (1);
  }

  /*
   *  Allocate space for the default server's host info and
   *  find the server's address and name. If the resolver library
   *  already has some addresses for a potential name server,
   *  then use them. Otherwise, see if the current host has a server.
   *  Command line arguments may override the choice of initial server.
   */

  defaultPtr =  (HostInfo*) Calloc (1, sizeof(HostInfo));

  /*
   * Parse the arguments:
   * no args =  go into interactive mode, use default host as server
   * 1 arg   =  use as host name to be looked up, default host will be server
   *            non-interactive mode
   * 2 args  =  1st arg:
   *            if it is '-', then
   *                 ignore but go into interactive mode
   *            else
   *                 use as host name to be looked up,
   *            go into non-interactive mode
   *            2nd arg: name or inet address of server
   *
   * "Set" options are specified with a leading - and must come before
   * any arguments. For example, to find the well-known services for
   * a host, type "nslookup -query=wks host"
   */

  ReadRCfile();    /* look for options file */

  ++argv; --argc;  /* skip prog name */

  while (argc && *argv[0] == '-' && argv[0][1])
  {
    SetOption (&argv[0][1]);
    ++argv;
    --argc;
  }

  if (argc > 2)
     Usage();

  if (argc && *argv[0] != '-')
     wantedHost = *argv;  /* name of host to be looked up */

  if (argc == 2)
  {
    struct in_addr addr;

    /*
     * Use an explicit name server. If the hostname lookup fails,
     * default to the server(s) in resolv.conf.
     */

    /* If conversion from address to number successful */
    if (inet_aton(*++argv, &addr))
    {
      /* Force resolver to use specified nameserver
       * winsock doesn't support this.  might need to use pieces
       * of bind itself and talk directly to nameservers.
       */
      _res.nscount = 1;
      _res.nsaddr.sin_addr = addr;
    }
    else
    {
      /* Try to get IP Address(es) for the requested server */
      hp = gethostbyname (*argv);
      /* If we can't get the address */
      if (hp == NULL)
      {
        fprintf (stderr, "*** Can't find server address for '%s': ",*argv);
        herror (NULL);
        fputc ('\n', stderr);
      }
      else
      {
        /* Force resolver to use the address(es) found */
        for (i = 0; i < MAXNS && hp->h_addr_list[i] != NULL; i++)
           memcpy (&_res.nsaddr_list[i].sin_addr,hp->h_addr_list[i],hp->h_length);

        _res.nscount = i;
      }
    }
  }

  if (_res.nscount == 0)
  {
    fprintf (stderr, "*** Default servers are not available\n");
    return (1);
  }

  for (i = 0; i < _res.nscount; i++)
  {
    result = GetHostInfoByAddr (&_res.nsaddr_list[i].sin_addr,
                                &_res.nsaddr_list[i].sin_addr,
                                defaultPtr);
    if (result != SUCCESS)
       fprintf (stderr,"*** Can't find server name for address %s: %s\n",
                inet_ntoa(_res.nsaddr_list[i].sin_addr),
                DecodeError(result));
    else
    {
      defaultAddr = _res.nsaddr_list[i].sin_addr;
      break;
    }
  }

  if (defaultPtr->name)
     strcpy (defaultServer, defaultPtr->name);

#ifdef DEBUG
  _res.options |= RES_DEBUG;
  _res.retry = 2;
#endif

  /*
   * If we're in non-interactive mode, look up the wanted host and quit.
   * Otherwise, print the initial server's name and continue with
   * the initialization.
   */

  if (wantedHost)
  {
    LookupHost (wantedHost, 0);
    return (0);
  }

  PrintHostInfo (stdout, "Default Server:", defaultPtr);

  pager = getenv ("PAGER");
  if (pager == NULL)
      pager = _PATH_PAGERCMD;

  /*
   * Setup the environment to allow the interrupt handler to return here.
   */
  setjmp (env);

  /*
   * Return here after a longjmp.
   */

  signal (SIGINT,(void(*)(int))IntrHandler);

  /*
   * Read and evaluate commands. The commands are described in commands.l
   * Yylex returns 0 when ^D or 'exit' is typed.
   */

  printf ("> ");
  fflush (stdout);
  while (yylex())
  {
    printf ("> ");
    fflush (stdout);
  }

  return (0);
}


/*
 *  Usage --
 *
 *  Lists the proper methods to run the program and exits.
 *
 */

static void Usage (void)
{
  fprintf (stderr,
    "Usage:\n"
    "   nslookup [-opt ...]             # interactive mode using default server\n"
    "   nslookup [-opt ...] - server    # interactive mode using 'server'\n"
    "   nslookup [-opt ...] host        # just look up 'host' using default server\n"
    "   nslookup [-opt ...] host server # just look up 'host' using 'server'\n");
  exit (1);
}


/*
 * IsAddr --
 *
 *  Returns TRUE if the string looks like an Internet address.
 *  A string with a trailing dot is not an address, even if it looks
 *  like one.
 */

int IsAddr (char *host, struct in_addr *addrPtr)
{
  char *cp;

  if (isdigit(host[0]))
  {
    /* Make sure it has only digits and dots. */
    for (cp = host; *cp; ++cp)
    {
      if (!isdigit(*cp) && *cp != '.')
         return (FALSE);
    }
    /* If it has a trailing dot, don't treat it as an address. */
    if (*--cp != '.')
       return inet_aton(host, addrPtr);
  }
  return (FALSE);
}


/*
 *  SetDefaultServer --
 *
 *  Changes the default name server to the one specified by
 *  the first argument. The command "server name" uses the current
 *  default server to lookup the info for "name". The command
 *  "lserver name" uses the original server to lookup "name".
 *
 *  Side effects:
 *  This routine will cause a core dump if the allocation requests fail.
 *
 *  Results:
 *  SUCCESS    The default server was changed successfully.
 *  NONAUTH    The server was changed but addresses of
 *      other servers who know about the requested server
 *      were returned.
 *  Errors    No info about the new server was found or
 *      requests to the current server timed-out.
 *
 */

int SetDefaultServer (char *string, Boolean local)
{
  HostInfo *newDefPtr;
  struct in_addr  *servAddrPtr;
  struct in_addr  addr;
  char   newServer[NAME_LEN];
  int    i, result;

  /*
   *  Parse the command line. It maybe of the form "server name",
   *  "lserver name" or just "name".
   */

  if (local)
       i = sscanf (string, " lserver %s", newServer);
  else i = sscanf (string, " server %s", newServer);

  if (i != 1)
  {
    i = sscanf (string, " %s", newServer);
    if (i != 1)
    {
      fprintf (stderr,"SetDefaultServer: invalid name: %s\n",  string);
      return (ERROR);
    }
  }

  /*
   * Allocate space for a HostInfo variable for the new server. Don't
   * overwrite the old HostInfo struct because info about the new server
   * might not be found and we need to have valid default server info.
   */

  newDefPtr = (HostInfo *) Calloc (1,sizeof(HostInfo));

  /*
   *  A 'local' lookup uses the original server that the program was
   *  initialized with.
   *
   *  Check to see if we have the address of the server or the
   *  address of a server who knows about this domain.
   *  XXX For now, just use the first address in the list.
   */

  if (local)
       servAddrPtr = &defaultAddr;
  else if (defaultPtr->addrList)
       servAddrPtr = (struct in_addr *) defaultPtr->addrList[0];
  else servAddrPtr = (struct in_addr *) defaultPtr->servers[0]->addrList[0];

  result = ERROR;
  if (IsAddr(newServer, &addr))
  {
    result = GetHostInfoByAddr (servAddrPtr, &addr, newDefPtr);
    /* If we can't get the name, fall through... */
  }
  if (result != SUCCESS && result != NONAUTH)
      result = GetHostInfoByName (servAddrPtr, C_IN, T_A,
                                  newServer, newDefPtr, 1);


  /* If we ask for an A record and get none back, but get an NS
   * record for the NS server, this is the NONAUTH case.
   * We must check whether we got an IP address for the NS
   * server or not.
   */
  if ((result == SUCCESS || result == NONAUTH) &&
      ((newDefPtr->addrList && newDefPtr->addrList[0]) ||
       (newDefPtr->servers && newDefPtr->servers[0] &&
        newDefPtr->servers[0]->addrList[0])))
  {
    /*
     *  Found info about the new server. Free the resources for
     *  the old server.
     */

    FreeHostInfoPtr (defaultPtr);
    free (defaultPtr);
    defaultPtr = newDefPtr;
    strcpy (defaultServer, defaultPtr->name);
    PrintHostInfo (stdout, "Default Server:", defaultPtr);
    return (SUCCESS);
  }
  else
  {
    fprintf (stderr, "*** Can't find address for server %s: %s\n",
             newServer, DecodeError(result));
    free (newDefPtr);
    return (result);
  }
}

/*
 * DoLoookup --
 *
 *  Common subroutine for LookupHost and LookupHostWithServer.
 *
 *  Results:
 *  SUCCESS    - the lookup was successful.
 *  Misc. Errors  - an error message is printed if the lookup failed.
 *
 */

/*static*/ int DoLookup (char *host, HostInfo *servPtr, char *serverName)
{
  int    result;
  struct in_addr *servAddrPtr;
  struct in_addr  addr;

  /* Skip escape character */
  if (host[0] == '\\')
     host++;

 /*
  *  If the user gives us an address for an address query,
  *  silently treat it as a PTR query. If the query type is already
  *  PTR, then convert the address into the in-addr.arpa format.
  *
  *  Use the address of the server if it exists, otherwise use the
  *  address of a server who knows about this domain.
  *  XXX For now, just use the first address in the list.
  */

  if (servPtr->addrList)
       servAddrPtr = (struct in_addr *) servPtr->addrList[0];
  else servAddrPtr = (struct in_addr *) servPtr->servers[0]->addrList[0];

  /*
   * RFC1123 says we "SHOULD check the string syntactically for a
   * dotted-decimal number before looking it up [...]" (p. 13).
   */
  if (queryType == T_A && IsAddr(host, &addr))
     result = GetHostInfoByAddr (servAddrPtr, &addr, &curHostInfo);
  else
  {
    if (queryType == T_PTR)
       CvtAddrToPtr (host);

    result = GetHostInfoByName (servAddrPtr, queryClass, queryType,
                                host, &curHostInfo, 0);
  }

  switch (result)
  {
    case SUCCESS:
      /*
       *  If the query was for an address, then the &curHostInfo
       *  variable can be used by Finger.
       *  There's no need to print anything for other query types
       *  because the info has already been printed.
       */
         if (queryType == T_A)
         {
           curHostValid = TRUE;
           PrintHostInfo (filePtr, "Name:", &curHostInfo);
         }
         break;

      /*
       * No Authoritative answer was available but we got names
       * of servers who know about the host.
       */
    case NONAUTH:
         PrintHostInfo (filePtr, "Name:", &curHostInfo);
         break;

    case NO_INFO:
         fprintf (stderr, "*** No %s (%s) records available for %s\n",
                  DecodeType(queryType), p_type(queryType), host);
         break;

    case TIME_OUT:
         fprintf (stderr, "*** Request to %s timed-out\n", serverName);
         break;

    default:
         fprintf (stderr, "*** %s can't find %s: %s\n", serverName, host,
                  DecodeError(result));
  }
  return (result);
}

/*
 *  LookupHost --
 *
 *  Asks the default name server for information about the
 *  specified host or domain. The information is printed
 *  if the lookup was successful.
 *
 *  Results:
 *  ERROR    - the output file could not be opened.
 *  + results of DoLookup
 *
 */

int LookupHost (char *string, Boolean putToFile)
{
  char host[NAME_LEN];
  char file[_MAX_PATH];
  int  result;

  /*
   *  Invalidate the current host information to prevent Finger
   *  from using bogus info.
   */

  curHostValid = FALSE;

  /*
   *  Parse the command string into the host and
   *  optional output file name.
   *
   */

  sscanf (string, " %s", host);  /* removes white space */
  if (!putToFile)
      filePtr = stdout;
  else
  {
    filePtr = OpenFile2 (string, file);
    if (!filePtr)
    {
      fprintf (stderr, "*** Can't open %s for writing\n", file);
      return (ERROR);
    }
    fprintf (filePtr,"> %s\n", string);
  }

  PrintHostInfo (filePtr, "Server:", defaultPtr);

  result = DoLookup (host, defaultPtr, defaultServer);

  if (putToFile)
  {
    fclose (filePtr);
    filePtr = NULL;
  }
  return (result);
}


/*
 *  LookupHostWithServer --
 *
 *  Asks the name server specified in the second argument for
 *  information about the host or domain specified in the first
 *  argument. The information is printed if the lookup was successful.
 *
 *  Address info about the requested name server is obtained
 *  from the default name server. This routine will return an
 *  error if the default server doesn't have info about the
 *  requested server. Thus an error return status might not
 *  mean the requested name server doesn't have info about the
 *  requested host.
 *
 *  Comments from LookupHost apply here, too.
 *
 *  Results:
 *  ERROR    - the output file could not be opened.
 *  + results of DoLookup
 *
 */

int LookupHostWithServer (char *string, Boolean putToFile)
{
  char   file[_MAX_PATH];
  char   host[NAME_LEN];
  char   server[NAME_LEN];
  int    result;
  static HostInfo serverInfo;

  curHostValid = FALSE;

  sscanf (string, " %s %s", host, server);
  if (!putToFile)
     filePtr = stdout;
  else
  {
    filePtr = OpenFile2 (string, file);
    if (!filePtr)
    {
      fprintf (stderr, "*** Can't open %s for writing\n", file);
      return (ERROR);
    }
    fprintf (filePtr,"> %s\n", string);
  }

  result = GetHostInfoByName (defaultPtr->addrList ?
                  (struct in_addr*)defaultPtr->addrList[0] :
                  (struct in_addr*)defaultPtr->servers[0]->addrList[0],
                  C_IN, T_A, server, &serverInfo, 1);

  if (result != SUCCESS)
     fprintf (stderr,"*** Can't find address for server %s: %s\n",
              server, DecodeError(result));
  else
  {
    PrintHostInfo (filePtr, "Server:", &serverInfo);
    result = DoLookup (host, &serverInfo, server);
  }
  if (putToFile)
  {
    fclose (filePtr);
    filePtr = NULL;
  }
  return (result);
}

/*
 *  SetOption --
 *
 *  This routine is used to change the state information
 *  that affect the lookups. The command format is
 *     set keyword[=value]
 *  Most keywords can be abbreviated. Parsing is very simplistic--
 *  A value must not be separated from its keyword by white space.
 *
 *  Valid keywords:    Meaning:
 *  all      lists current values of options.
 *  ALL      lists current values of options, including
 *          hidden options.
 *  [no]d2      turn on/off extra debugging mode.
 *  [no]debug    turn on/off debugging mode.
 *  [no]defname    use/don't use default domain name.
 *  [no]search    use/don't use domain search list.
 *  domain=NAME    set default domain name to NAME.
 *  [no]ignore    ignore/don't ignore trunc. errors.
 *  query=value    set default query type to value,
 *        value is one of the query types in RFC883
 *        without the leading T_.  (e.g., A, HINFO)
 *  [no]recurse    use/don't use recursive lookup.
 *  retry=#      set number of retries to #.
 *  root=NAME    change root server to NAME.
 *  time=#      set timeout length to #.
 *  [no]vc      use/don't use virtual circuit.
 *  port      TCP/UDP port to server.
 *
 *   Deprecated:
 *  [no]primary    use/don't use primary server.
 *
 *  Results:
 *  SUCCESS    the command was parsed correctly.
 *  ERROR    the command was not parsed correctly.
 *
 */

int SetOption (char *option)
{
  char  type[NAME_LEN];
  char  *ptr;
  int    tmp;

  while (isspace(*option))
        ++option;
  if (!strncmp(option,"set ",4))
     option += 4;

  while (isspace(*option))
        ++option;

  if (*option == 0)
  {
    fprintf (stderr, "*** Invalid set command\n");
    return (ERROR);
  }
  else
  {
    if (!strnicmp(option,"all",3))
         ShowOptions();
    else if (!strncmp(option,"d2",2))    /* d2 (more debug) */
         _res.options |= (RES_DEBUG | RES_DEBUG2);
    else if (!strncmp(option,"nod2",4))
    {
      _res.options &= ~RES_DEBUG2;
      printf ("d2 mode disabled; still in debug mode\n");
    }
    else if (!strncmp(option,"def",3))   /* defname */
         _res.options |= RES_DEFNAMES;
    else if (!strncmp(option,"nodef",5))
         _res.options &= ~RES_DEFNAMES;
    else if (!strncmp(option,"do",2))    /* domain */
    {
      ptr = strchr (option,'=');
      if (ptr)
      {
        sscanf (++ptr, "%s", _res.defdname);
        res_re_init();
      }
    }
    else if (!strncmp(option,"deb",1))   /* debug */
         _res.options |= RES_DEBUG;
    else if (!strncmp(option,"nodeb",5))
         _res.options &= ~(RES_DEBUG | RES_DEBUG2);
    else if (!strncmp(option,"ig",2))   /* ignore */
         _res.options |= RES_IGNTC;
    else if (!strncmp(option,"noig",4))
         _res.options &= ~RES_IGNTC;
    else if (strncmp(option,"po",2))    /* port */
    {
      ptr = strchr (option,'=');
      if (ptr)
         sscanf (++ptr, "%hu", &nsport);
    }
#ifdef deprecated
    else if (!strncmp(option,"pri",3))  /* primary */
         _res.options |= RES_PRIMARY;
    else if (!strncmp(option,"nopri",5))
         _res.options &= ~RES_PRIMARY;
#endif
    else if (!strncmp(option,"q", 1) ||  /* querytype */
             !strncmp(option,"ty",2))     /* type */
    {
      ptr = strchr (option, '=');
      if (ptr)
      {
        sscanf (++ptr, "%s", type);
        queryType = StringToType (type,queryType,stderr);
      }
    }
    else if (!strncmp(option,"cl",2))   /* query class */
    {
      ptr = strchr (option, '=');
      if (ptr)
      {
        sscanf (++ptr, "%s", type);
        queryClass = StringToClass (type, queryClass, stderr);
      }
    }
    else if (!strncmp(option,"rec",3))   /* recurse */
         _res.options |= RES_RECURSE;
    else if (!strncmp(option,"norec",5))
         _res.options &= ~RES_RECURSE;
    else if (!strncmp(option,"ret",3))   /* retry */
    {
      ptr = strchr (option, '=');
      if (ptr)
      {
        sscanf (++ptr, "%d", &tmp);
        if (tmp >= 0)
           _res.retry = tmp;
      }
    }
    else if (!strncmp(option,"ro",2))   /* root */
    {
      ptr = strchr (option, '=');
      if (ptr)
         sscanf (++ptr, "%s", rootServerName);
    }
    else if (!strncmp(option,"sea",3))   /* search list */
         _res.options |= RES_DNSRCH;
    else if (!strncmp(option,"nosea",5))
         _res.options &= ~RES_DNSRCH;
    else if (!strncmp(option,"srchl",5)) /* domain search list */
    {
      ptr = strchr (option, '=');
      if (ptr)
         res_dnsrch (++ptr);
    }
    else if (!strncmp(option,"ti",2))    /* timeout */
    {
      ptr = strchr (option, '=');
      if (ptr)
      {
        sscanf (++ptr, "%d", &tmp);
        if (tmp >= 0)
           _res.retrans = tmp;
      }
    }
    else if (!strncmp(option,"v",1))     /* vc */
         _res.options |= RES_USEVC;
    else if (!strncmp(option,"nov",3))
         _res.options &= ~RES_USEVC;
    else
    {
      fprintf (stderr, "*** Invalid option: %s\n",  option);
      return (ERROR);
    }
  }
  return (SUCCESS);
}

/*
 * Fake a reinitialization when the domain is changed.
 */
/*static*/ void res_re_init (void)
{
  char *cp, **pp;
  int   n;

  /* find components of local domain that might be searched */
  pp = _res.dnsrch;
  *pp++ = _res.defdname;
  for (cp = _res.defdname, n = 0; *cp; cp++)
      if (*cp == '.')
         n++;
  cp = _res.defdname;
  for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch + MAXDFLSRCH; n--)
  {
    cp = strchr(cp, '.');
    *pp++ = ++cp;
  }
  *pp = 0;
  _res.options |= RES_INIT;
}

#define SRCHLIST_SEP '/'

/*static*/ void res_dnsrch (char *cp)
{
  char **pp;
  int n;

  strncpy (_res.defdname, cp, sizeof(_res.defdname) - 1);
  if ((cp = strchr(_res.defdname, '\n')) != NULL)
      *cp = '\0';
  /*
   * Set search list to be blank-separated strings
   * on rest of line.
   */
  cp = _res.defdname;
  pp = _res.dnsrch;
  *pp++ = cp;
  for (n = 0; *cp && pp < _res.dnsrch + MAXDNSRCH; cp++)
  {
    if (*cp == SRCHLIST_SEP)
    {
      *cp = '\0';
      n = 1;
    }
    else if (n)
    {
      *pp++ = cp;
      n = 0;
    }
  }
  if ((cp = strchr(pp[-1], SRCHLIST_SEP)) != NULL)
      *cp = '\0';
  *pp = NULL;
}


/*
 *  ShowOptions --
 *
 *  Prints out the state information used by the resolver
 *  library and other options set by the user.
 *
 */

/*static*/ void ShowOptions (void)
{
  char **cp;

  PrintHostInfo (stdout, "Default Server:", defaultPtr);
  if (curHostValid)
     PrintHostInfo (stdout, "Host:", &curHostInfo);

  printf ("Set options:\n");
  printf ("  %sdebug  \t",   (_res.options & RES_DEBUG)    ? "" : "no");
  printf ("  %sdefname\t",   (_res.options & RES_DEFNAMES) ? "" : "no");
  printf ("  %ssearch\t",    (_res.options & RES_DNSRCH)   ? "" : "no");
  printf ("  %srecurse\n",   (_res.options & RES_RECURSE)  ? "" : "no");

  printf ("  %sd2\t\t",      (_res.options & RES_DEBUG2)   ? "" : "no");
  printf ("  %svc\t\t",      (_res.options & RES_USEVC)    ? "" : "no");
  printf ("  %signoretc\t",  (_res.options & RES_IGNTC)    ? "" : "no");
  printf ("  port=%u\n",     nsport);

  printf ("  querytype=%s\t",p_type(queryType));
  printf ("  class=%s\t",    p_class(queryClass));
  printf ("  timeout=%d\t",  _res.retrans);
  printf ("  retry=%d\n",    _res.retry);
  printf ("  root=%s\n",     rootServerName);
  printf ("  domain=%s\n",   _res.defdname);

  if ((cp = _res.dnsrch) != 0)
  {
    printf ("  srchlist=%s", *cp);
    for (cp++; *cp; cp++)
        printf ("%c%s", SRCHLIST_SEP, *cp);
    putchar ('\n');
  }
  putchar ('\n');
}
#undef SRCHLIST_SEP

/*
 *  PrintHelp --
 *
 *  Displays the help file.
 *
 */

void PrintHelp (void)
{
  char cmd[_MAX_PATH];

  if (!stricmp(pager,"more.exe"))
       sprintf (cmd, "more < %s", _PATH_HELPFILE);
  else sprintf (cmd, "%s %s", pager, _PATH_HELPFILE);
  if (_res.options & RES_DEBUG2)
     printf ("Running: \"%s\"\n", cmd);
  System (cmd);
}

/*
 * CvtAddrToPtr --
 *
 *  Convert a dotted-decimal Internet address into the standard
 *  PTR format (reversed address with .in-arpa. suffix).
 *
 *  Assumes the argument buffer is large enougth to hold the result.
 *
 */

/*static*/ void CvtAddrToPtr (char *name)
{
  struct in_addr addr;

  if (IsAddr(name, &addr))
  {
    int  ip[4];
    char *p = inet_ntoa (addr);
    if (sscanf(p, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
        sprintf (name, "%d.%d.%d.%d.in-addr.arpa.",
                 ip[3], ip[2], ip[1], ip[0]);
  }
}

/*
 * ReadRCfile --
 *
 *  Use the contents of ~/.nslookuprc as options.
 *
 */

/*static*/ void ReadRCfile (void)
{
  FILE *fp;
  char *env = getenv ("HOME");
  char buf[_MAX_PATH];

  if (env && (strlen(env) + strlen(_PATH_NSLOOKUPRC)) < sizeof(buf))
  {
    if (!strrchr(env,'\\'))
         sprintf (buf,"%s%s",   env,_PATH_NSLOOKUPRC);
    else sprintf (buf,"%s\\%s", env,_PATH_NSLOOKUPRC);

    if ((fp = fopen(buf,"r")) != NULL)
    {
      while (fgets(buf,sizeof(buf),fp))
      {
        char *cp = strchr (buf,'\n');
        if (cp)
           *cp = '\0';
        SetOption (buf);
      }
      fclose (fp);
    }
  }
}


