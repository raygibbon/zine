/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <copyright.h>.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "pfs.h"

static struct ptext *pfree   = NULL;
static int    ptext_count    = 0;
static int    ptext_max      = 0;

static struct vlink *vfree   = NULL;
static int    vlink_count    = 0;
static int    vlink_max      = 0;

static struct pattrib *afree = NULL;
static int    pattrib_count  = 0;
static int    pattrib_max    = 0;

/*
 * ptalloc - allocate and initialize ptext structure
 *
 *    PTALLOC returns a pointer to an initialized structure of type
 *    PTEXT.  If it is unable to allocate such a structure, it
 *    returns NULL.
 */
struct ptext *ptalloc (void)
{
  struct ptext *vt;

  if (pfree)
  {
    vt = pfree;
    pfree = pfree->next;
  }
  else
  {
    vt = (struct ptext*) calloc (sizeof(*vt),1);
    if (!vt)
       return (NULL);
    ptext_max++;
  }
  ptext_count++;

  /* The offset is to leave room for additional headers */
  vt->start = vt->dat + MAX_PTXT_HDR;

  return (vt);
}

/*
 * ptfree - free a VTEXT structure
 *
 *    VTFREE takes a pointer to a VTEXT structure and adds it to
 *    the free list for later reuse.
 */
void ptfree (struct ptext *vt)
{
  vt->next     = pfree;
  vt->previous = NULL;
  pfree        = vt;
  ptext_count--;
}

/*
 * ptlfree - free a VTEXT structure
 *
 *    VTLFREE takes a pointer to a VTEXT structure frees it and any linked
 *    VTEXT structures.  It is used to free an entrie list of VTEXT
 *    structures.
 */
void ptlfree (struct ptext *vt)
{
  while (vt)
  {
    struct ptext *nxt = vt->next;
    ptfree (vt);
    vt = nxt;
  }
}

/*
 * vlalloc - allocate and initialize vlink structure
 *
 *    VLALLOC returns a pointer to an initialized structure of type
 *    VLINK.  If it is unable to allocate such a structure, it
 *    returns NULL.
 */
struct vlink *vlalloc (void)
{
  struct vlink *vl;

  if (vfree)
  {
    vl    = vfree;
    vfree = vfree->next;
  }
  else
  {
    vl = (struct vlink*) malloc (sizeof(*vl));
    if (!vl)
       return (NULL);
    vlink_max++;
  }

  vlink_count++;

  /* Initialize and fill in default values */
  memset (vl,0,sizeof(*vl));

  vl->linktype = 'L';
  vl->type     = stcopy ("FILE");
  vl->hosttype = stcopy ("INTERNET-D");
  vl->nametype = stcopy ("ASCII");

  return (vl);
}

/*
 * vlfree - free a VLINK structure
 *
 *    VLFREE takes a pointer to a VLINK structure and adds it to
 *    the free list for later reuse.
 */
void vlfree (struct vlink *vl)
{
  extern int string_count;

  if (vl->dontfree)
     return;

  /* many of these don't need to call stfree(); since a check
   * for pointer validity's already done before even calling
   * it, we can just call free() here then do one big decrement
   * of string_count at the end.
   */

  if (vl->name)
     free (vl->name);
  stfree (vl->type);
  if (vl->replicas)
     vllfree (vl->replicas);
  stfree (vl->hosttype);
  if (vl->host)
     free (vl->host);
  stfree (vl->nametype);
  if (vl->filename)
     free (vl->filename);
  if (vl->args)
     free (vl->args);
  if (vl->lattrib)
     atlfree (vl->lattrib);

  /* No allocation routines for f_info yet */
  vl->f_info   = NULL;
  vl->next     = vfree;
  vl->previous = NULL;
  vfree = vl;
  vlink_count--;
  string_count -= 4; /* freed name, host, filename, and args */
}

/*
 * vllfree - free a VLINK structure
 *
 *    VLLFREE takes a pointer to a VLINK structure frees it and any linked
 *    VLINK structures.  It is used to free an entrie list of VLINK
 *    structures.
 */
void vllfree (struct vlink *vl)
{
  struct vlink *nxt;

  while (vl && !vl->dontfree)
  {
    nxt = vl->next;
    vlfree (vl);
    vl = nxt;
  }
}


/*
 * atalloc - allocate and initialize vlink structure
 *
 *    ATALLOC returns a pointer to an initialized structure of type
 *    PATTRIB.  If it is unable to allocate such a structure, it
 *    returns NULL.
 */
struct pattrib *atalloc (void)
{
  struct pattrib *at;

  if (afree)
  {
    at = afree;
    afree = afree->next;
  }
  else
  {
    at = (struct pattrib*) malloc (sizeof(*at));
    if (!at)
       return (NULL);
    pattrib_max++;
  }

  pattrib_count++;

  memset (at,0,sizeof(*at));
  at->precedence = ATR_PREC_OBJECT;

  return (at);
}

/*
 * atfree - free a PATTRIB structure
 *
 *    ATFREE takes a pointer to a PATTRRIB structure and adds it to
 *    the free list for later reuse.
 */
void atfree (struct pattrib *at)
{
  if (at->aname)
     stfree (at->aname);

  if (!strcmp(at->avtype,"ASCII") && at->value.ascii)
     stfree (at->value.ascii);

  if (!strcmp(at->avtype,"LINK") && at->value.link)
     vlfree (at->value.link);

  if (at->avtype)
     stfree (at->avtype);

  at->next     = afree;
  at->previous = NULL;
  afree        = at;
  pattrib_count--;
}

/*
 * atlfree - free a PATTRIB structure
 *
 *    ATLFREE takes a pointer to a PATTRIB structure frees it and any linked
 *    PATTRIB structures.  It is used to free an entrie list of PATTRIB
 *    structures.
 */
void atlfree (struct pattrib *at)
{
  while (at)
  {
    struct pattrib *nxt = at->next;
    atfree (at);
    at = nxt;
  }
}

