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
 *  subr.c --
 *
 *  Miscellaneous subroutines for the name server
 *  lookup program.
 *
 *  Copyright (c) 1985
 *  Andrew Cherenson
 *  U.C. Berkeley
 *  CS298-26  Fall 1985
 *
 */

#define ALLOW_UPDATES
#include "res.h"

#ifdef __HIGHC__
#include <io.h>
#include <dos.h>
#include <pharlap.h>
#endif

/*
 *  IntrHandler --
 *
 *  This routine is called whenever a control-C is typed.
 *  It performs three main functions:
 *   - closes an open socket connection,
 *   - closes an open output file (used by LookupHost, et al.),
 *   - jumps back to the main read-eval loop.
 *
 *  If a user types a ^C in the middle of a routine that uses a socket,
 *  the routine would not be able to close the socket. To prevent an
 *  overflow of the process's open file table, the socket and output
 *  file descriptors are closed by the interrupt handler.
 *
 *  Side effects:
 *  Open file descriptors are closed.
 *  If filePtr is valid, it is closed.
 *  Flow of control returns to the main() routine.
 *
 */

void IntrHandler (void)
{
  SendRequest_close();
  ListHost_close();
  if (filePtr && filePtr != stdout)
  {
    fclose (filePtr);
    filePtr = NULL;
  }
  printf ("\n");
  yyrestart (yyin);
  longjmp (env, 1);
}


/*
 *  Malloc --
 *  Calloc --
 *
 *  Calls the malloc library routine with SIGINT blocked to prevent
 *  corruption of malloc's data structures. We need to do this because
 *  a control-C doesn't kill the program -- it causes a return to the
 *  main command loop.
 *
 *  NOTE: This method doesn't prevent the pointer returned by malloc
 *  from getting lost, so it is possible to get "core leaks".
 *
 *  If malloc fails, the program exits.
 *
 *  Results:
 *  (address)  - address of new buffer.
 *
 */

char *Malloc (int size)
{
  char *ptr = malloc (size);

  if (!ptr)
  {
    fflush(stdout);
    fprintf(stderr, "*** Can't allocate memory\n");
    fflush(stderr);
    exit (1);
  }
  return (ptr);
}

char *Calloc (int num, int size)
{
  char *ptr = Malloc(num*size);
  memset (ptr,0,num*size);
  return (ptr);
}


/*
 *  PrintHostInfo --
 *
 *  Prints out the HostInfo structure for a host.
 *
 */

void PrintHostInfo (FILE *file, char *title, HostInfo *hp)
{
  char       **cp;
  ServerInfo **sp;
  char         comma;
  int          i;

  fprintf (file, "%-7s  %s", title, hp->name ? hp->name : "<none>");

  if (hp->addrList)
  {
    if (hp->addrList[1])
         fprintf (file, "\nAddresses:");
    else fprintf (file, "\nAddress:");

    comma = ' ';
    i = 0;
    for (cp = hp->addrList; cp && *cp; cp++)
    {
      i++;
      if (i > 4)
      {
        fprintf (file, "\n\t");
        comma = ' ';
        i = 0;
      }
      fprintf (file,"%c %s", comma, inet_ntoa(*(struct in_addr*)*cp));
      comma = ',';
    }
  }

  if (hp->aliases)
  {
    fprintf (file, "\nAliases:");
    comma = ' ';
    i = 10;
    for (cp = hp->aliases; cp && *cp && **cp; cp++)
    {
      i += strlen(*cp) + 2;
      if (i > 75)
      {
        fprintf (file, "\n\t");
        comma = ' ';
        i = 10;
      }
      fprintf (file, "%c %s", comma, *cp);
      comma = ',';
    }
  }

  if (hp->servers)
  {
    fprintf(file, "\nServed by:\n");
    for (sp = hp->servers; *sp != NULL ; sp++)
    {
      fprintf (file, "- %s\n\t", (*sp)->name);
      comma = ' ';
      i = 0;
      for (cp = (*sp)->addrList; cp && *cp && **cp; cp++)
      {
        i++;
        if (i > 4)
        {
          fprintf (file, "\n\t");
          comma = ' ';
          i = 0;
        }
        fprintf (file,"%c %s", comma, inet_ntoa(*(struct in_addr *)*cp));
        comma = ',';
      }
      fprintf(file, "\n\t");

      comma = ' ';
      i = 10;
      for (cp = (*sp)->domains; cp && *cp && **cp; cp++)
      {
        i += strlen (*cp) + 2;
        if (i > 75)
        {
          fprintf (file, "\n\t");
          comma = ' ';
          i = 10;
        }
        fprintf (file, "%c %s", comma, *cp);
        comma = ',';
      }
      fprintf (file, "\n");
    }
  }

  fprintf (file, "\n\n");
}

/*
 *  OpenFile2 --
 *
 *  Parses a command string for a file name and opens
 *  the file.
 *
 *  Results:
 *  file pointer  - the open was successful.
 *  NULL    - there was an error opening the file or
 *        the input string was invalid.
 */

FILE *OpenFile2 (char *string, char *file)
{
  char *redirect;
  FILE *tmpPtr;

  /*
   *  Open an output file if we see '>' or >>'.
   *  Check for overwrite (">") or concatenation (">>").
   */

  redirect = strchr (string, '>');
  if (!redirect)
     return(NULL);

  if (redirect[1] == '>')
  {
    sscanf (redirect, ">> %s", file);
    tmpPtr = fopen (file, "a+");
  }
  else
  {
    sscanf (redirect, "> %s", file);
    tmpPtr = fopen (file, "w");
  }

  if (tmpPtr)
     redirect[0] = '\0';

  return (tmpPtr);
}

/*
 *  Function for looking up "struct res_sym" tables
 */

static char *sym_ntos (struct res_sym *tab, int err_code, int *success)
{
  *success = 1;  /* assume we'll find it */

  while (tab->text)
  {
    if (err_code == tab->code)
       return (tab->text);
    tab++;
  }
  *success = 0;
  return (NULL);
}

static int sym_ston (struct res_sym *tab, char *err_text, int *success)
{
  *success = 1;  /* assume we'll find it */

  while (tab->text)
  {
    if (!strcmp(err_text,tab->text))
       return (tab->code);
    tab++;
  }
  *success = 0;
  return (0);
}

/*
 *  DecodeError --
 *
 *  Converts an error code into a character string.
 */

static struct res_sym p_class_syms[] = {
           { C_IN,  "IN"  },
           { C_HS,  "HS"  },
           { C_ANY, "ANY" },
           { 0,     NULL  }
         };

static struct res_sym __p_type_syms[] = {
           { T_A,         "A"        },
           { T_NS,        "NS"       },
           { T_CNAME,     "CNAME"    },
           { T_SOA,       "SOA"      },
           { T_MB,        "MB"       },
           { T_MG,        "MG"       },
           { T_MR,        "MR"       },
           { T_NULL,      "NULL"     },
           { T_WKS,       "WKS"      },
           { T_PTR,       "PTR"      },
           { T_HINFO,     "HINFO"    },
           { T_MINFO,     "MINFO"    },
           { T_MX,        "MX"       },
           { T_TXT,       "TXT"      },
           { T_RP,        "RP"       },
           { T_AFSDB,     "AFSDB"    },
           { T_X25,       "X25"      },
           { T_ISDN,      "ISDN"     },
           { T_RT,        "RT"       },
           { T_NSAP,      "NSAP"     },
           { T_NSAP_PTR,  "NSAP_PTR" },
           { T_SIG,       "SIG"      },
           { T_KEY,       "KEY"      },
           { T_PX,        "PX"       },
           { T_GPOS,      "GPOS"     },
           { T_AAAA,      "AAAA"     },
           { T_LOC,       "LOC"      },
           { T_AXFR,      "AXFR"     },
           { T_MAILB,     "MAILB"    },
           { T_MAILA,     "MAILA"    },
           { T_ANY,       "ANY"      },
           { T_UINFO,     "UINFO"    },
           { T_UID,       "UID"      },
           { T_GID,       "GID"      },
           { T_UNSPEC,    "UNSPEC"   },
           { 0,           NULL       }
         };

static struct res_sym error_syms[] = {
           { NOERROR2,    "Success"                  },
           { FORMERR,     "Format error"             },
           { SERVFAIL,    "Server failed"            },
           { NXDOMAIN,    "Non-existent host/domain" },
           { NOTIMP,      "Not implemented"          },
           { REFUSED,     "Query refused"            },
           { NOCHANGE,    "No change"                },
           { TIME_OUT,    "Timed out"                },
           { NO_INFO,     "No information"           },
           { ERROR2,      "Unspecified error"        },
           { NONAUTH,     "Non-authoritative answer" },
           { NO_RESPONSE, "No response from server"  },
           { 0,           NULL                       }
         };

char *DecodeError (int result)
{
  int   success;
  char *string = sym_ntos (error_syms, result, &success);
  if (success)
     return (string);
  return ("BAD ERROR VALUE");
}


int StringToClass (char *class, int dflt, FILE *errorfile)
{
  int success;
  int result = sym_ston (p_class_syms, class, &success);

  if (success)
     return (result);

  if (errorfile)
     fprintf (errorfile, "unknown query class: %s\n", class);
  return (dflt);
}


/*
 *  StringToType --
 *
 *  Converts a string form of a query type name to its
 *  corresponding integer value.
 */

int StringToType (char *type, int dflt, FILE *errorfile)
{
  int success;
  int result = sym_ston (__p_type_syms, type, &success);

  if (success)
     return (result);

  if (errorfile)
     fprintf (errorfile, "unknown query type: %s\n", type);
  return (dflt);
}

/*
 *  DecodeType --
 *
 *  Converts a query type to a descriptive name.
 *  (A more verbose form of p_type.)
 *
 */

char *DecodeType (int type)
{
  switch (type)
  {
    case T_A       : return "host address";
    case T_NS      : return "authoritative server";
    case T_MD      : return "mail destination";
    case T_MF      : return "mail forwarder";
    case T_CNAME   : return "canonical name";
    case T_SOA     : return "start of authority zone";
    case T_MB      : return "mailbox domain name";
    case T_MG      : return "mail group member";
    case T_MR      : return "mail rename name";
    case T_NULL    : return "null resource record";
    case T_WKS     : return "well known service";
    case T_PTR     : return "domain name pointer";
    case T_HINFO   : return "host information";
    case T_MINFO   : return "mailbox information";
    case T_MX      : return "mail routing information";
    case T_TXT     : return "text strings";
    case T_RP      : return "responsible person";
    case T_AFSDB   : return "AFS cell database";
    case T_X25     : return "X_25 calling address";
    case T_ISDN    : return "ISDN calling address";
    case T_RT      : return "router";
    case T_NSAP    : return "NSAP address";
    case T_NSAP_PTR: return "reverse NSAP lookup";
    case T_SIG     : return "security signature";
    case T_KEY     : return "security key";
    case T_PX      : return "X.400 mail mapping";
    case T_GPOS    : return "geo. position (withdrawn)";
    case T_AAAA    : return "IP6 Address";
    case T_LOC     : return "Location Information";
    case T_NXT     : return "Next Valid Name in Zone";
    case T_EID     : return "Endpoint identifier";
    case T_NIMLOC  : return "Nimrod locator";
    case T_SRV     : return "Server selection";
    case T_ATMA    : return "ATM Address";
    case T_NAPTR   : return "Naming Authority PoinTeR";
    case T_UINFO   : return "user (finger) information";
    case T_UID     : return "user ID";
    case T_GID     : return "group ID";
    case T_UNSPEC  : return "Unspecified format (binary data)";
    case T_IXFR    : return "incremental zone transfer";
    case T_AXFR    : return "transfer zone of authority";
    case T_MAILB   : return "transfer mailbox records";
    case T_MAILA   : return "transfer mail agent records";
    case T_ANY     : return "wildcard match";
    default        : return "Unknown";
  }
}

/*
 *  Metaware HighC's system() is flaky.. make our own
 */
#ifdef __HIGHC__
static int Execute (char *path, char *args)
{
  union  REGS reg;
  EXEC_BLK    blk;
  char        buf[150];
  int         len;
  ULONG       minPages,maxPages;

  if ((len = strlen(args)) > sizeof(buf)-2)
     return (0);

  _dx_cmem_limit (4,&minPages,&maxPages);

  strcpy (buf+1,args);
  buf [0]     = len;
  buf [++len] = '\r';

  blk.ex_eseg = 0;              /* inherit environment  */
  blk.ex_coff = (UCHAR*)&buf;   /* command line address */
  blk.ex_cseg = SS_DATA;
  reg.x.ax    = 0x4B00;
  reg.x.bx    = (UINT)&blk;
  reg.x.dx    = (UINT)path;
  intdos (&reg,&reg);
  return (reg.x.cflag & 1 != 1);
}

int System (char *cmd)
{
  char buf[150];
  char *env = getenv ("COMSPEC");

  if (!env || access(env,0))
  {
    fprintf (stderr,"Bad COMSPEC variable\n");
    return (0);
  }

  if (cmd && *cmd)
       _bprintf (buf, sizeof(buf), " /c %s", cmd);
  else _bprintf (buf, sizeof(buf), "%s", cmd);
  return Execute (env,buf);
}
#endif

