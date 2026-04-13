/*
** Master include file.
**
**	@(#)vrfy.h              e07@nikhef.nl (Eric Wassenaar) 961006
*/

#ifndef __VERIFY_H
#define __VERIFY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if defined(WATT32)
#include <tcp.h>
#endif

#undef NOERROR           /* in <sys/streams.h> on solaris 2.x */
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>

typedef u_char  qbuf_t;
typedef char    nbuf_t;
typedef void    ptr_t;
typedef u_int   siz_t;
typedef int     bool;

#define TRUE    1
#define FALSE	0

#define NOT_DOTTED_QUAD INADDR_NONE

#define MAXMXHOSTS      20
#define MAXHOST         255
#define MAXSPEC         255
#define MAXHOP          17

#define MAXREPLY        100
#define MAXLOOP         10
#define LOCALHOST       "127.0.0.1"
#define UUCPRELAY       LOCALHOST
#define BITNETRELAY     LOCALHOST
#define SINGLERELAY     LOCALHOST
#define CONNTIMEOUT     30
#define READTIMEOUT     30

#include "defs.h"

#ifndef WATT32
extern int h_errno;
#endif

#define newlist(a,n,t)  (t *)xalloc((ptr_t*)a, (siz_t)((n)*sizeof(t)))
#define newstr(s)       strcpy ((char*)xalloc(NULL,strlen(s)+1), s)
#define sameword(a,b)   (!stricmp(a,b))

#define is_alnum(c)     (isascii(c) && isalnum(c))
#define is_digit(c)	(isascii(c) && isdigit(c))
#define is_space(c)	(isascii(c) && isspace(c))

/* check for linear white space
 */
#define is_lwsp(c)      (c == ' ' || c == '\t')

/* sendmail V8 meta-characters
 */
#define is_meta(c)	(((c) & 0340) == 0200)

#define linebufmode(a)  setvbuf (a,NULL,_IOLBF,BUFSIZ);


/* Various exit codes
 */
#define EX_SUCCESS      0      /* successful termination */
#define EX_USAGE        64     /* command line usage error */
#define EX_DATAERR      65     /* data format error */
#define EX_NOINPUT      66     /* cannot open input */
#define EX_NOUSER       67     /* addressee unknown */
#define EX_NOHOST       68     /* host name unknown */
#define EX_UNAVAILABLE  69     /* service unavailable */
#define EX_SOFTWARE     70     /* internal software error */
#define EX_OSERR        71     /* system error (e.g., can't fork) */
#define EX_OSFILE       72     /* critical OS file missing */
#define EX_CANTCREAT    73     /* can't create (user) output file */
#define EX_IOERR        74     /* input/output error */
#define EX_TEMPFAIL     75     /* temp failure; user is invited to retry */
#define EX_PROTOCOL     76     /* remote error in protocol */
#define EX_NOPERM       77     /* permission denied */
#define EX_CONFIG       78     /* local configuration error */
#define EX_AMBUSER      79     /* ambiguous user */


/* Define constants for fixed sizes.
 */
#ifndef INT16SZ
#define INT16SZ         2       /* for systems without 16-bit ints */
#endif

#ifndef INT32SZ
#define INT32SZ         4       /* for systems without 32-bit ints */
#endif

#ifndef INADDRSZ
#define INADDRSZ        4       /* for sizeof(struct inaddr) != 4 */
#endif

#endif

