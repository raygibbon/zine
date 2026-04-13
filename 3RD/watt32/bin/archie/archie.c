/*
 * Copyright (c) 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * "copyright.h".
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pfs.h"
#include "archie.h"

int   listflag     = 0;         /* 0 = human readable or 1 = single line */
int   attrflag     = 0;         /* show Unix attribs in single line list */
int   url_flag     = 0;         /* show on URL in single line list       */
int   motd_flag    = 0;         /* request "MEssage Of The Day"          */
int   sortflag     = BY_HOST;   /* list sort option                      */
int   verbose      = 0;         /* verbose mode of operation             */
int   max_hits     = MAX_HITS;  /* Maximum number of hits for this query */
int   offset       = 0;         /* The offset for the Prospero query.    */
int   alex         = 0;         /* Display the Alex filename?            */
char *new_line     = "\r\n";    /* new-line token in server queries      */
int   rdgram_prio  = 0;         /* datagram priority                     */
int   pfs_debug    = 0;         /* debug level                           */
FILE *archie_out;               /* file to write results to              */

char  def_archie[] = "archie.funet.fi";

char *archies[] = {
      "archie.au                   Australia",
      "archie.univie.ac.at         Austria",
      "archie.belnet.be            Belgium",
      "archie.bunyip.com           Canada",
      "archie.cs.mcgill.ca         Canada",
      "archie.funet.fi             Finland",
      "archie.th-darmstadt.de      Germany",
      "archie.ac.il                Israel",
      "archie.unipi.it             Italy",
      "archie.wide.ad.jp           Japan",
      "archie.kornet.nm.kr         Korea",
      "archie.sogang.ac.kr         Korea",
      "archie.nz                   New Zealand",
      "archie.uninett.no           Norway",
      "archie.icm.edu.pl           Poland",
      "archie.rediris.es           Spain",
      "archie.luth.se              Sweden",
      "archie.switch.ch            Switzerland",
      "archie.ncu.edu.tw           Taiwan",
      "archie.hensa.ac.uk          United Kingdom",
      "archie.doc.ic.ac.uk         United Kingdom",
      "archie.ans.net              USA (NY)",
      "archie.internic.net         USA (NJ)",
      "archie.rutgers.edu          USA (NJ)",
      "archie.sura.net             USA (MD)",
      "archie.unl.edu              USA (NE)",
      NULL
    };

static void Usage       (void);
static void ListArchies (char *host);

static void Abort (char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  va_end (args);
  exit (EXIT_FAILURE);
}

int main (int argc, char **argv)
{
  Query query = EXACT;
  char *host  = def_archie;
  char *env   = getenv ("ARCHIE_HOST");
  int   ch;

  if (env)
     host = strdup (env);

  archie_out = stdout;

  while ((ch = getopt (argc,argv,"d:LN:O:ceh:alAUm:o:rstvV")) != EOF)
  {
    switch (ch)
    {
      case 'd':
           if (pfs_debug == 0)
              dbug_init();
           pfs_debug = atoi (optarg);
           if (pfs_debug == 0)
              Abort ("Illegal debug level `%c'\n", *optarg);
           break;

      case 'L':
           ListArchies (host);
           exit (0);

      case 'N':
           rdgram_prio = atoi (optarg);
           rdgram_prio = min (rdgram_prio,RDGRAM_MAX_PRI);
           rdgram_prio = max (rdgram_prio,RDGRAM_MIN_PRI);
           break;

      case 'c':    /* Substring (case-sensitive) */
           query = SUBSTRING_CASE;
           break;

      case 'e':    /* Exact match */
           query = EXACT;
           break;

      case 'h':    /* Archie host */
           if (env)
              free (host);
           host = optarg;
           break;

      case 'a':    /* List matches as Alex filenames */
           alex = 1;
           break;

      case 'l':    /* List one match per line */
           listflag = 1;
           break;

      case 'A':    /* Show Unix attributes */
           attrflag = 1;
           if (!listflag)
              Abort ("`A' option requires `l' option\n");
           break;

      case 'U':
           url_flag = 1;
           if (!listflag)
              Abort ("`U' option requires `l' option\n");
           break;

      case 'm':    /* Maximum number of hits for the query.  */
           max_hits = atoi (optarg);
           if (max_hits < 1)
              Abort ("option `-m' requires a max hits value >= 1\n");
           break;

      case 'o':    /* output file */
           if (archie_out != stdout)
              Abort ("multiple output files specified\n");
           archie_out = fopen (optarg, "w+");
           if (archie_out == NULL)
              Abort ("cannot open %s\n",optarg);
           break;

      case 'O':    /* Specify the offset */
           offset = atoi (optarg);
           break;

      case 'r':    /* Regexp search */
           query = REGEXP;
           break;

      case 's':    /* Substring (case insensitive) */
           query = SUBSTRING;
           break;

      case 't':    /* Sort inverted by date */
           sortflag = BY_DATE;
           break;

      case 'v':    /* Display version */
           fprintf (stderr,"%s\nClient version %s based upon Prospero version %s\n",
                    wattcpVersion(), CLIENT_VERSION, PFS_RELEASE);
           break;

      case 'V':   /* Verbose when search is happening */
           verbose = 1;
           break;

      default:
           Usage ();
    }
  }

  if (optind == argc)
     Usage();

  if (alex && listflag)
     Abort ("only one of `-a' or `-l' may be used\n");

  sock_init();

  for (; optind < argc; ++optind)
      ProcQuery (host, argv[optind], max_hits, offset, query);

  if (archie_out != stdout)
     fclose (archie_out);

  return (EXIT_SUCCESS);
}

static void Usage (void)
{
  printf (
    "Usage: archie [-acerslAUtLvVO] [-d level] [-m hits] \n"
    "              [-N level] [-o file] [-h host] string\n"
    "   -a         : list matches as Alex filenames\n"
    "   -c         : case sensitive substring search\n"
    "   -e         : exact string match (default)\n"
    "   -r         : regular expression search\n"
    "   -s         : case insensitive substring search\n"
    "   -l         : list one match per line\n"
    "   -A         : shows Unix attributes in `l' option\n"
    "   -U         : shows URL://format in `l' option\n"
    "   -t         : sort by accending date\n"
    "   -L         : list known servers and current default\n"
    "   -v         : show version information\n"
    "   -V         : verbose printout\n"
    "   -d level   : sets debug level (0 - 3)\n"
    "   -m hits    : specifies maximum number of hits to return (default %d)\n"
    "   -N level   : specifies query niceness level (0-%d)\n"
    "   -o filename: specifies file to store results in\n"
    "   -h host    : specifies server host\n", max_hits,RDGRAM_MAX_PRI);

  exit (EXIT_FAILURE);
}

static void ListArchies (char *def_host)
{
  int i;

  printf ("Known archie servers:\n");
  for (i=0; archies[i]; i++)
      printf ("      %s\n", archies[i]);

  printf (" * %s is the default Archie server.\n"\
          " * For the most up-to-date list, telnet to an Archie\n"
          " * server (username `archie') and give it the command `servers'.\n",
          def_host);
}
