/*
 * archie.h : Definitions for the programmatic Prospero interface to Archie
 *
 *     Written by Brendan Kehoe (brendan@cygnus.com),
 *                George Ferguson (ferguson@cs.rochester.edu), and
 *                Clifford Neuman (bcn@isi.edu).
 */

#ifndef __ARCHIE_H
#define __ARCHIE_H

extern FILE *archie_out;
extern int   listflag;
extern int   sortflag;
extern int   attrflag;
extern int   url_flag;
extern int   motd_flag;
extern int   alex;
extern int   pfs_debug;
extern int   verbose;
extern int   rdgram_prio;

/*
 * Default value for max hits.  Note that this is normally different
 * for different client implementations.  Doing so makes it easier to
 * collect statistics on the use of the various clients.
 */

#define MAX_HITS 95

/*
 * CLIENT_VERSION may be used to identify the version of the client if 
 * distributed separately from the Prospero distribution.  The version
 * command should then identify both the client version and the Prospero
 * version identifiers.   
 */
#define CLIENT_VERSION "1.4.1"

/* The different kinds of queries we can make.  */
typedef enum {
        NONE           = '\0',
        EXACT          = '=',
        REGEXP         = 'R',
        SUBSTRING      = 'S',
        SUBSTRING_CASE = 'C'
      } Query;

typedef enum {
        BY_HOST    = 0,
        BY_CLOSEST = 1,
        BY_DATE    = 2,
        BY_SIZE    = 3
      } Sorting;

void ProcQuery (char *host, char *str, int max_hits, int offset, Query query);

#endif /* __ARCHIE_H */

