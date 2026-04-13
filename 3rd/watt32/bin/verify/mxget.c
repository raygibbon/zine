/*
 * mxget.c -- fetch MX records for given DNS name
 *
 * Copyright 1997 by Eric S. Raymond
 * For license terms, see the file COPYING in this directory.
 */

#include "vrfy.h"

/*
 * These are defined in RFC833. Some bind interface headers don't declare them.
 * Ghod help us if they're ever actually incompatible with what's in 
 * the arpa/nameser.h header.
 */
#ifndef PACKETSZ
#define PACKETSZ	512		/* maximum packet size */
#endif
#ifndef HFIXEDSZ
#define	HFIXEDSZ	12		/* #/bytes of fixed data in header */
#endif

struct mxentry {
       char *name;
       int   pref;
     };

/* minimum possible size of MX record in packet */
#define MIN_MX_SIZE	8	/* corresp to "a.com 0" w/ terminating space */

/*
 * get MX records for given host
 */
struct mxentry *getmxrecords (const char *name)
{
  char   answer[PACKETSZ], *eom, *cp, *bp;
  int    n, ancount, qdcount, buflen, type, pref, ind;
  static struct mxentry pmx [(PACKETSZ - HFIXEDSZ) / MIN_MX_SIZE];
  static char MXHostBuf[PACKETSZ - HFIXEDSZ];
  HEADER *hp;

  pmx->name = NULL;
  pmx->pref = -1;
  n = res_search (name,C_IN,T_MX,(u_char*)&answer,sizeof(answer));
  if (n == -1)
     return (NULL);

  hp = (HEADER*) &answer;
  cp = answer + HFIXEDSZ;
  eom = answer + n;
  h_errno = 0;
  for (qdcount = ntohs(hp->qdcount); qdcount--; cp += n + QFIXEDSZ)
      if ((n = dn_skipname(cp, eom)) < 0)
         return (NULL);

  buflen = sizeof(MXHostBuf) - 1;
  bp  = MXHostBuf;
  ind = 0;
  ancount = ntohs(hp->ancount);
  while (--ancount >= 0 && cp < eom)
  {
    if ((n = dn_expand(answer, eom, cp, bp, buflen)) < 0)
       break;
    cp += n;
    GETSHORT (type, cp);
    cp += INT16SZ + INT32SZ;
    GETSHORT (n, cp);
    if (type != T_MX)
    {
      cp += n;
      continue;
    }
    GETSHORT (pref, cp);
    if ((n = dn_expand(answer, eom, cp, bp, buflen)) < 0)
       break;
    cp += n;

    pmx[ind].name = bp;
    pmx[ind].pref = pref;
    ++ind;

    n = strlen (bp);
    bp += n;
    *bp++ = '\0';
    buflen -= n + 1;
  }

  pmx[ind].name = NULL;
  pmx[ind].pref = -1;
  return (pmx);
}

int main (int argc, char **argv)
{
  struct mxentry *responses;

  if (argc < 2)
  {
    printf ("Usage: %s host\n", argv[0]);
    return (1);
  }

  dbug_init();
  sock_init();

  responses = getmxrecords (argv[1]);
  if (!responses)
  {
    puts ("No MX records found");
    return (1);
  }
  do
  {
    printf ("%s %d\n", responses->name, responses->pref);
  }
  while ((++responses)->name);
  return (0);
}
