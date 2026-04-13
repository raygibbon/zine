/*
 * query.c : Routines for processing results from Archie
 *
 * Originally part of the Prospero Archie client by Cliff Neuman (bcn@isi.edu).
 * Modified by Brendan Kehoe (brendan@cygnus.com).
 * Re-modified by George Ferguson (ferguson@cs.rochester.edu).
 *
 * Copyright (c) 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <copyright.h>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include "pfs.h"
#include "perrno.h"
#include "archie.h"


/*
 * Functions defined here
 */
static void alex_name      (FILE *outfile, char *host, char *file, int dirp);
static void translate_resp (struct vlink *l);
static int  hostnamecmp    (char *a, char *b);
static int  host_compare   (struct vlink *p, struct vlink *q);
static int  date_compare   (struct vlink *p, struct vlink *q);


/*
 * Data defined here
 */

static struct tm *presenttime;
static char       lastpath [MAX_VPATH] = "\001";
static char       lasthost [MAX_VPATH] = "\001";

/* Print the contents of the given virtual link.  */

static void display_link (struct vlink *l)
{
  struct pattrib *ap;
  char   linkpath [MAX_VPATH];
  char   archie_date [40];
  int    dirflag = 0;
  DWORD  size    = 0L;
  char  *modes   = "";
  char  *gt_date = "";
  int    gt_year = 0;
  int    gt_mon  = 0;
  int    gt_day  = 0;
  int    gt_hour = 0;
  int    gt_min  = 0;
  int    special = listflag || alex;

  /* Initialize local buffers */
  *archie_date = '\0';

  /* Remember if we're looking at a directory */
  if (strstr(l->type,"DIRECTORY"))
       dirflag = 1;
  else dirflag = 0;

  /* Extract the linkpath from the filename */
  strncpy (linkpath,l->filename,sizeof(linkpath)-1);
  linkpath [strlen(linkpath) - strlen(l->name) - 1] = '\0';

  /* Is this a new host? */
  if (strcmp(l->host,lasthost))
  {
    if (!special)
       fprintf (archie_out, "\nHost %s\n\n",l->host);
    strcpy (lasthost,l->host);
    *lastpath = '\001';
  }

  /* Is this a new linkpath (location)? */
  if (strcmp(linkpath,lastpath))
  {
    if (!special)
        fprintf (archie_out,
                 "    Location: %s\n",(*linkpath ? linkpath : "/"));
    strcpy (lastpath,linkpath);
  }

  /* Parse the attibutes of this link */
  for (ap = l->lattrib; ap; ap = ap->next)
  {
    if (!strcmp(ap->aname,"SIZE"))
       sscanf (ap->value.ascii,"%lu",&size);

    else if (!strcmp(ap->aname,"UNIX-MODES"))
            modes = ap->value.ascii;

    else if (!strcmp(ap->aname,"LAST-MODIFIED"))
    {
      gt_date = ap->value.ascii;
      sscanf (gt_date,"%4d%2d%2d%2d%2d",
              &gt_year, &gt_mon, &gt_day, &gt_hour, &gt_min);

      if (listflag)
           sprintf (archie_date,"%02d/%02d/%04d %02d:%02d",
                    gt_day, gt_mon, gt_year, gt_hour, gt_min);
      else if ((12 * (presenttime->tm_year + 1900 - gt_year) +
               presenttime->tm_mon - gt_mon) > 6)
           sprintf (archie_date,"%s %2d %4d",month_sname(gt_mon),
                    gt_day, gt_year);
      else sprintf (archie_date,"%s %2d %02d:%02d",month_sname(gt_mon),
                    gt_day, gt_hour, gt_min);
    }
  }

  /* Print this link's information */

  if (listflag)   /* one line per link */
  {
    char *isdir = dirflag ? "/" : "";

    fprintf (archie_out, "%.9s %s %8lu  ",
             attrflag ? modes : "", archie_date, size);

    if (url_flag)
         fprintf (archie_out, "ftp://%s%s%s\n", l->host, l->filename, isdir);
    else fprintf (archie_out, "%s : %s%s\n",     l->host, l->filename, isdir);
  }
  else if (alex)
       alex_name (archie_out,l->host, l->filename, dirflag);
  else fprintf (archie_out,
                "      %9s %s %10lu  %s  %s\n", dirflag ? "DIRECTORY" : "FILE",
                modes, size, archie_date, l->name);
             
  /* Free the attibutes */
  atlfree (l->lattrib);
  l->lattrib = NULL;
}

/*
 * archie_query : Sends a request to _host_, telling it to search for
 *                _string_ using _query_ as the search method.
 *                No more than _max_hits_ matches are to be returned
 *                skipping over _offset_ matches.
 *
 *                archie_query returns a linked list of virtual links. 
 *                The Archie responses will be translated and sorted
 *                using _cmp_proc_ to compare pairs of links.
 *
 *                archie_query returns NULL and sets perrno if the query
 *                failed. Note that it can return NULL with perrno == PSUCCESS
 *                if the query didn't fail but there were simply no matches.
 *
 *        query:  S  Substring search ignoring case   
 *                C  Substring search with case significant
 *                R  Regular expression search
 *                =  Exact String Match
 *            s,c,e  Tries exact match first and falls back to S, C, or R 
 *                   if not found.
 *
 *     cmp_proc:  vlink sorting routine
 *
 */
static struct vlink *archie_query (
                     char *host,   char *string,
                     int max_hits, int offset,
                     Query query,
                     int (*cmp_proc)(struct vlink*,struct vlink*))
{
  char           qstring[MAX_VPATH];    /* For construting the query  */
  struct vdir    dir;                   /* Filled in by get_vdir      */
  struct vlink  *links;                 /* Matches returned by server */
  struct vlink  *p, *q, *r;
  struct vlink  *lowest, *nextp;
  struct vlink  *pnext,  *pprev;
  int            tmp;

  /* Make the query string */
  sprintf (qstring, "ARCHIE/MATCH(%d,%d,%c)/%s",
           max_hits, offset, (char)query, string);

  /* Initialize Prospero structures */
  perrno = PSUCCESS; *p_err_string  = '\0';
  pwarn  = PNOWARN;  *p_warn_string = '\0';
  memset (&dir,0,sizeof(dir));

  /* Retrieve the list of matches, return error if there was one */

  tmp = get_vdir (host,qstring,&dir,GVD_ATTRIB|GVD_NOSORT);
  if (tmp)
  {
    perrno = tmp;
    return (NULL);
  }

  /* Save the links, and clear in dir in case it's used again   */
  links = dir.links;
  dir.links = NULL;

  /* As returned, list is sorted by suffix, and conflicting     */
  /* suffixes appear on a list of "replicas".  We want to       */
  /* create a one-dimensional list sorted by host then filename */
  /* and maybe by some other parameter                          */

  for (p = links; p; p = nextp) /* flatten the doubly-linked list */
  {
    nextp = p->next;
    if (p->replicas)
    {
      p->next = p->replicas;
      p->next->previous = p;
      for (r = p->replicas; r->next; r = r->next)
          ;
      r->next = nextp;
      nextp->previous = r;
      p->replicas = NULL;
    }
  }

  /* Translate the filenames */
  for (p = links; p; p = p->next)
      translate_resp (p);

  /* sort it using a selection sort and the given cmp_proc */
  for (p = links; p; p = nextp)
  {
    nextp  = p->next;
    lowest = p;
    for (q = p->next; q; q = q->next)
        if ((*cmp_proc)(q,lowest) < 0)
           lowest = q;
    if (p != lowest)
    {
      /* swap the two links */
      pnext = p->next;
      pprev = p->previous;
      if (lowest->next)
          lowest->next->previous = p;
      p->next = lowest->next;
      if (nextp == lowest)
         p->previous = lowest;
      else
      {
        lowest->previous->next = p;
        p->previous = lowest->previous;
      }
      if (nextp == lowest)
         lowest->next = p;
      else
      {
        pnext->previous = lowest;
        lowest->next = pnext;
      }
      if (pprev)
          pprev->next = lowest;
      lowest->previous = pprev;

      if (links == p)   /* keep the head of the list in the right place */
          links = lowest;
    }
  }

  /* Return the links */
  perrno = PSUCCESS;
  return (links);
}

/*
 * procquery : Process the given query and display the results.
 *      Sort by option given in sortflag. If listflag is non-zero
 *      then each entry is printed on a separate, complete line.
 */
void ProcQuery (char *host, char *str, int max_hits, int offset, Query query)
{
  struct vlink *l;
  time_t now;

  /* initialize data structures for this query */
  time (&now);
  presenttime = localtime (&now);

  /* Do the query */
  switch (sortflag)
  {
    case BY_HOST:
         l = archie_query (host,str,max_hits,offset,query,host_compare);
         break;

    case BY_DATE:
         l = archie_query (host,str,max_hits,offset,query,date_compare);
         break;
#if 0
    case BY_CLOSEST:
         l = archie_query (host,str,max_hits,offset,query,dist_compare);
         break;

    case BY_SIZE:
         l = archie_query (host,str,max_hits,offset,query,size_compare);
         break;
#endif

    default:
         l = archie_query (host,str,max_hits,offset,query,host_compare);
  }

  if (pfs_debug >= 1)
     fprintf (stderr,"\n");

  /* Error? */
  if (perrno != PSUCCESS)
  {
    if (p_err_text[perrno])
    {
      if (*p_err_string)
           fprintf (stderr,"failed: %s - %s\n", p_err_text[perrno], p_err_string);
      else fprintf (stderr,"failed: %s\n",  p_err_text[perrno]);
    }
    else
      fprintf (stderr, "failed: Undefined error %d (prospero)\n", perrno);
  }

  /* Warning? */
  if (pwarn != PNOWARN)
  {
    if (*p_warn_string)
         fprintf (stderr,"Warning! %s - %s\n", p_warn_text[pwarn], p_warn_string);
    else fprintf (stderr,"Warning! %s\n", p_warn_text[pwarn]);
  }


  /* Display the results */

  if (!l && pwarn == PNOWARN && perrno == PSUCCESS)
  {
    if (!listflag)
       puts ("\nNo matches.");
    exit (1);
  }

  lasthost[0] = 0;
  lastpath[0] = 0;
  while (l)
  {
    display_link (l);
    l = l->next;
  }
}

/* Given a dotted hostname, return its Alex root.  */
static char *alex_reverse (char *string, int len)
{
  int   i   = 0;
  char *buf = malloc (len);
  char *p   = buf;
  char *q   = string + len - 1;

  while (q > string)
  {
    for (i = 0; q > string; q--, i++)
        if (*q == '.')
        {
          q++;
          break;
        }
    if (q == string)
       i++;
    strncpy (p,q,i);
    p += i;
    *p++ = '/';
    i    = 0;
    q   -= 2;
  }
  *--p = '\0';
  return (buf);
}

/*
 * Emit a string that's the Alex filename for the given host and file.
 */
static void alex_name (FILE *out_file, char *host, char *file, int dirp)
{
  int   hostlen = strlen (host);
  int   len     = strlen (file) + hostlen + dirp + 8;
  char *buf     = malloc (len);
  char *rev;

  if (!buf)
     exit (99);

  sprintf (buf,"/alex/%s%s\n", rev = alex_reverse(host,hostlen), file);
  if (dirp)
  {
    len--;
    buf[len-1] = '/';
    buf[len]   = '\0';
  }

  fputs (buf,out_file);
  free (buf);
  free (rev);
}


/*
 * translate_resp:
 *
 *   If the given link is for an archie-pseudo directory, fix it. 
 *   This is called unless AQ_NOTRANS was given to archie_query().
 */
static void translate_resp (struct vlink *l)
{
  char *slash;

  if (!strcmp(l->type,"DIRECTORY"))
  {
    if (!strncmp(l->filename,"ARCHIE/HOST",11))
    {
      l->type = stcopyr ("EXTERNAL(AFTP,DIRECTORY)",l->type);
      l->host = stcopyr (l->filename+12,l->host);
      slash   = strchr (l->host,'/');
      if (slash)
      {
        l->filename = stcopyr (slash,l->filename);
        *slash++ = '\0';
      }
      else
        l->filename = stcopyr ("",l->filename);
    }
  }
}


/* hostnamecmp: Compare two hostnames based on domain,
 *              right-to-left.  Returns <0 if a belongs before b, >0
 *              if b belongs before a, and 0 if they are identical.
 *              Contributed by asami@cs.berkeley.edu (Satoshi ASAMI).
 */
static int hostnamecmp (char *a, char *b)
{
  char *pa, *pb;
  int   result;

  for (pa = a ; *pa ; pa++)
      ;
  for (pb = b ; *pb ; pb++)
      ;

  while (pa > a && pb > b)
  {
    for (; pa > a ; pa--)
        if (*pa == '.')
        {
          pa++;
          break;
        }
    for (; pb > b ; pb--)
        if (*pb == '.')
        {
          pb++;
          break;
        }
    if ((result = strcmp(pa,pb)) != 0)
        return (result);
    pa -= 2;
    pb -= 2;
  }
  if (pa <= a)
  {
    if (pb <= b)
         return (0);
    else return (-1);
  }
  else
    return (1);
}

/*
 * host_compare: The default link comparison function for sorting. Compares
 *               links p and q first by host then by filename. Returns < 0 if p
 *               belongs before q, > 0 if p belongs after q, and == 0 if their
 *               host and filename fields are identical.
 */
static int host_compare (struct vlink *p, struct vlink *q)
{
  int result = hostnamecmp (p->host,q->host);

  if (result)
       return (result);
  else return (strcmp(p->filename,q->filename));
}

/*
 * date_compare: An alternative comparison function for sorting that
 *               compares links p and q first by LAST-MODIFIED date,
 *               if they both have that attribute. If both links
 *               don't have that attribute or the dates are the
 *               same, it then calls host_compare() and returns its value.
 */
static int date_compare (struct vlink *p, struct vlink *q)
{
  struct pattrib *pat,  *qat;
  char           *pdate,*qdate;
  int             result;

  pdate = qdate = NULL;
  for (pat = p->lattrib; pat; pat = pat->next)
      if (!strcmp(pat->aname,"LAST-MODIFIED"))
         pdate = pat->value.ascii;

  for (qat = q->lattrib; qat; qat = qat->next)
      if (!strcmp(qat->aname,"LAST-MODIFIED"))
         qdate = qat->value.ascii;

  if (!pdate && !qdate)
     return (host_compare(p,q));

  if (!pdate)
     return (1);

  if (!qdate)
     return (-1);

  if ((result = strcmp(qdate,pdate)) == 0)
       return (host_compare(p,q));
  else return (result);
}


