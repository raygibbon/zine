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
 *  @(#)res.h  5.10 (Berkeley) 6/1/90
 *  $Id: res.h,v 8.3 1996/12/02 09:17:24 vixie Exp $
 */

/*
 *  res.h --
 *
 *  Definitions used by modules of the name server lookup program.
 *
 *  Copyright (c) 1985
 *  Andrew Cherenson
 *  U.C. Berkeley
 *  CS298-26  Fall 1985
 *
 */

#ifndef __RES_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <netdb.h>
#include <tcp.h>

#define TRUE   1
#define FALSE  0

#define MAXALIASES  35
#define MAXADDRS    35
#define MAXDOMAINS  35
#define MAXSERVERS  10

typedef int Boolean;

/*
 *  Define return statuses in addtion to the ones defined in namserv.h
 *   let SUCCESS be a synonym for NOERROR
 *
 *  TIME_OUT    - a socket connection timed out.
 *  NO_INFO     - the server didn't find any info about the host.
 *  ERROR       - one of the following types of errors:
 *                dn_expand, res_mkquery failed
 *                bad command line, socket operation failed, etc.
 *  NONAUTH     - the server didn't have the desired info but
 *                returned the name(s) of some servers who should.
 *  NO_RESPONSE - the server didn't respond.
 *
 */

#define  SUCCESS      0
#define  TIME_OUT    -1
#define  NO_INFO     -2
#define  ERROR2      -3
#define  NONAUTH     -4
#define  NO_RESPONSE -5

#define  ERROR       -6
#define  NOERROR2     0

/*
 *  Define additional options for the resolver state structure.
 *
 *   RES_DEBUG2    more verbose debug level
 */

#define RES_DEBUG2  0x80000000

/*
 *  Maximum length of server, host and file names.
 */

#define NAME_LEN 256

/*
 * We need to know the IPv6 address family number even on IPv4-only systems.
 * Note that this is NOT a protocol constant, and that if the system has its
 * own AF_INET6, different from ours below, all of BIND's libraries and
 * executables will need to be recompiled after the system <sys/socket.h>
 * has had this type added.  The type number below is correct on most BSD-
 * derived systems for which AF_INET6 is defined.
 */
#ifndef AF_INET6
#define AF_INET6  24
#endif


/*
 * Modified struct hostent from <netdb.h>
 *
 * "Structures returned by network data base library.  All addresses
 * are supplied in host order, and returned in network order (suitable
 * for use in system calls)."
 */

typedef struct  {
        char   *name;          /* official name of host */
        char  **domains;       /* domains it serves */
        char  **addrList;      /* list of addresses from name server */
      } ServerInfo;

typedef struct  {
        char   *name;          /* official name of host */
        char  **aliases;       /* alias list */
        char  **addrList;      /* list of addresses from name server */
        int     addrType;      /* host address type */
        int     addrLen;       /* length of address */
        ServerInfo **servers;
      } HostInfo;

struct res_sym {
       int    code;
       char  *text;
     };

/*
 *  FilePtr is used for directing listings to a file.
 *  It is global so the Control-C handler can close it.
 */

extern jmp_buf   env;
extern FILE     *filePtr;
extern char      rootServerName[];

extern HostInfo *defaultPtr;
extern HostInfo  curHostInfo;
extern int       curHostValid;
extern int       queryType;
extern int       queryClass;


/*
 * TCP/UDP port of server.
 */
extern unsigned short nsport;

/*
 *  Imported from the resolver library.
 */
#ifndef WIN32
#define W32_DATA extern
#endif

W32_DATA char *_res_resultcodes[];
W32_DATA char *_res_opcodes[];
W32_DATA struct __res_state _res;


/*
 *  External routines:
 */

/* commands.l */

extern FILE *yyin;              /* scanner input file */
extern int  yylex     (void);   /* main scanner routine */
extern void yyrestart (FILE*);  /* restart scanner after interrupt */

/* debug.c */

void    Print_query  (char *msg, char *eom, int printHeader);
void    Fprint_query (u_char *msg,u_char *eom, int header,  FILE *file);
u_char *Print_cdname (u_char *cp, u_char *msg, u_char *eom, FILE *file);
u_char *Print_cdname2(u_char *cp, u_char *msg, u_char *eom, FILE *file);
u_char *Print_rr     (u_char *ocp,u_char *msg, u_char *eom, FILE *file);

/* getinfo.c */

int  GetHostInfoByName (struct in_addr *nsAddrPtr, int queryClass, int queryType, char *name, HostInfo *hostPtr, Boolean isServer);
int  GetHostInfoByAddr (struct in_addr *nsAddrPtr, struct in_addr *address, HostInfo *hostPtr);
void FreeHostInfoPtr   (HostInfo *hostPtr);

/* list.c */

void ListHostsByType (char *string, int putToFile);
void ListHosts       (char *string, int putToFile);
void ViewList        (char *string);
int  Finger          (char *string, int putToFile);
void ListHost_close  (void);

/* main.c */

int  IsAddr               (char *host, struct in_addr *addrPtr);
int  SetDefaultServer     (char *string, Boolean local);
int  LookupHost           (char *string, Boolean putToFile);
int  LookupHostWithServer (char *string, Boolean putToFile);
int  SetOption            (char *option);
void PrintHelp            (void);

/* send.c */

int  SendRequest      (struct in_addr *nsAddrPtr, char *buf, int buflen, char *answer, u_int anslen, int *trueLenPtr);
void SendRequest_close(void);

/* skip.c */

char *res_skip (char *msg, int numFieldsToSkip, char *eom);


/* subr.c */

void  IntrHandler   (void);
char *Malloc        (int size);
char *Calloc        (int num, int size);
void  PrintHostInfo (FILE *file, char *title, HostInfo *hp);
FILE *OpenFile2     (char *string, char *file);
int   StringToClass (char *class, int dflt, FILE *errorfile);
int   StringToType  (char *type, int dflt, FILE *errorfile);
char *DecodeError   (int result);
char *DecodeType    (int type);

#ifdef __HIGHC__
int System (char *cmd);
#else
#define System(s) system(s)
#endif

#endif
