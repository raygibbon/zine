/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <copyright.h>.
 */

#include <stdio.h>
#include <string.h>

#include "pfs.h"
#include "perrno.h"
#include "archie.h"

static struct pfs_auth_info *get_pauth (void)
{
  static struct pfs_auth_info no_auth = { "UNAUTHENTICATED", "* unknown *" };
  return (&no_auth);
}

/*
 * get_vdir - Get contents of a directory given its location
 *
 *            GET_VDIR takes a directory location,  a pointer to a directory
 *            structure to be filled in, and flags.  It then queries the
 *            appropriate directory server and retrieves the desired
 *            information.
 *
 *      ARGS:   dhost       - Host on which directory resides
 *              dfile       - Directory on that host
 *              dir         - Structure to be filled in
 *              flags       - Options.  See FLAGS
 *
 *     FLAGS:   GVD_UNION   - Do not expand union links
 *              GVD_EXPAND  - Expand union links locally
 *              GVD_REMEXP  - Request remote expansion (& local if refused)
 *              GVD_LREMEXP - Request remote expansion of local union links
 *              GVD_VERIFY  - Only verify that args are for a directory
 *              GVD_ATTRIB  - Request attributes from directory server
 *              GVD_NOSORT  - Do not sort links when adding to directory
 *
 *   RETURNS:   PSUCCESS (0) or error code
 *              On some codes addition information in p_err_string
 *
 *     NOTES:   If the directory passed as an argument already has
 *              links or union links, then those lists will be freed
 *              before the new contents are filled in.
 *
 *              If a filter is passed to the procedure, and application of
 *              the filter results in additional union link, then those links
 *              will (or will not) be expanded as specified in the FLAGS field.
 *
 *      BUGS:   Doesn't process union links yet
 *              Doesn't process errors returned from server
 *              Doesn't expand union links if requested to
 */
int get_vdir (
    char        *dhost,        /* Host on which directory resides     */
    char        *dfile,        /* Name of file on that host           */
    struct vdir *dir,          /* Structure to be filled in           */
    int          flags)        /* Flags                               */
{
  struct ptext *request;       /* Text of request to dir server       */
  struct ptext *resp;          /* Response from dir server            */

  struct vlink *cur_link = NULL;  /* Current link being filled in     */
  struct vlink *exp      = NULL;  /* The current ulink being expanded */
  struct vlink *pul      = NULL;  /* Prev union link                  */
  struct vlink *l;                /* Temp link pointer                */
  struct pfs_auth_info *authinfo; /* authentication info (user/pass)  */
  int    getattrib = 0;           /* Get attributes from server       */
  int    vl_insert_flag;          /* Flags to vl_insert               */
  int    fwdcnt = MAX_FWD_DEPTH;

  int    no_links = 0;   /* Count of number of links found */
  char   options[40];    /* LIST option                    */
  char  *opt;            /* After leading +                */
  char  *p;

  perrno     = 0;
  authinfo   = get_pauth();
  options[0] = 0;

  if (flags & GVD_ATTRIB)
  {
    getattrib++;
    flags &= ~GVD_ATTRIB;
  }

  if (flags & GVD_NOSORT)
       vl_insert_flag = VLI_NOSORT;
  else vl_insert_flag = VLI_ALLOW_CONF;
  flags &= ~GVD_NOSORT;

  if (getattrib)
  {
    strcat (options,"+ATTRIBUTES");
    flags &= ~GVD_ATTRIB;
  }

  if (flags == GVD_REMEXP)  strcat (options,"+EXPAND");
  if (flags == GVD_LREMEXP) strcat (options,"+LEXPAND");

  /*
   * If all we are doing is verifying that dfile is a directory then we
   * do not want a big response from the directory server.  A NOT-FOUND
   * is sufficient.
   */
  if (flags == GVD_VERIFY)
     strcat (options,"+VERIFY");

  if (*options)
       opt = options+1;
  else opt = "''";

startover:
  request = ptalloc();
  p  = request->start;
  p += sprintf (p,"VERSION %d %s\n", VFPROT_VNO, PFS_SW_ID);

#if (VFPROT_VNO == 1)
  if (strcmp(authinfo->authenticator,"* unknown *"))
     p += sprintf (p,"AUTHENTICATOR %s %s\n",
                   authinfo->auth_type,
                   authinfo->authenticator);
#else
  p += sprintf (p,"AUTHENTICATE '' UNAUTHENTICATED * unknown *\n");
#endif

  if (motd_flag)
     p += sprintf (p,"PARAMETER GET MOTD\n");

  p += sprintf (p,"DIRECTORY ASCII %s\nLIST %s COMPONENTS", dfile, opt);

  request->length = p - request->start;
  Adebug (2,"\nSending message to dirsrv:\n%s",request->start);

  resp = dirsend (request,dhost);
  if (!resp)
     Adebug (1,"\nfailed: %d\n",perrno);

  /* If we don't get a response, then if the requested       */
  /* directory, return error, if a ulink, mark it unexpanded */
  if (!resp)
  {
    if (exp)
         exp->expanded = FAILED;
    else return (perrno);
  }

  /* Here we must parse reponse and put in directory */
  /* While looking at each packet                    */
  while (resp)
  {
    char         *line;
    struct ptext *vtmp = resp;

    Adebug (3,"\n%s\n",resp->start);

    /* Look at each line in packet */
    for (line = resp->start; line; line = nxtline(line))
    {
      switch (line[0])
      {
        char l_linktype;
        char l_name [MAX_DIR_LINESIZE];
        char l_type [MAX_DIR_LINESIZE];
        char l_htype[MAX_DIR_LINESIZE];
        char l_host [MAX_DIR_LINESIZE];
        char l_ntype[MAX_DIR_LINESIZE];
        char l_fname[MAX_DIR_LINESIZE];
        int  l_version;
        int  l_magic;
        int  tmp;

        case 'L': /* LINK or LINK-INFO */
             if (!strncmp(line,"LINK-INFO",9))
             {
               struct pattrib *at, *last_at;
               at = parse_attr (line);
               if (!at)
                  break;

               /* Cant have link info without a link */
               if (!cur_link)
               {
                 perrno = DIRSRV_BAD_FORMAT;
                 atfree(at);
                 break;
               }

               if (cur_link->lattrib)
               {
                 last_at = cur_link->lattrib;
                 while (last_at->next)
                        last_at = last_at->next;
                 at->previous = last_at;
                 last_at->next = at;
               }
               else
               {
                 cur_link->lattrib = at;
                 at->previous = NULL;
               }
               break;
             }

             /* Not LINK-INFO, must be LINK - if not check for error */
             if (strncmp(line,"LINK",4))
                goto scanerr;

             /* If only verifying, don't want to change dir */
             if (flags == GVD_VERIFY)
                break;

             /* If first link and some links in dir, free them */
             if (!no_links++)
             {
               if (dir->links)  vllfree(dir->links);  dir->links  = NULL;
               if (dir->ulinks) vllfree(dir->ulinks); dir->ulinks = NULL;
             }

             cur_link = vlalloc();

             /* parse and insert file info */
             tmp = sscanf (line,"LINK %c %s %s %s %s %s %s %ld %ld",
                           &l_linktype, l_type, l_name, l_htype, l_host,
                           l_ntype, l_fname, &cur_link->version,
                           &cur_link->f_magic_no);

             if (tmp != 9)
             {
               perrno = DIRSRV_BAD_FORMAT;
               vlfree(cur_link);
               break;
             }

             cur_link->linktype = l_linktype;
             cur_link->type     = stcopyr (l_type,cur_link->type);
             cur_link->name     = stcopyr (unquote(l_name),cur_link->name);
             cur_link->hosttype = stcopyr (l_htype,cur_link->hosttype);
             cur_link->host     = stcopyr (l_host,cur_link->host);
             cur_link->nametype = stcopyr (l_ntype,cur_link->nametype);
             cur_link->filename = stcopyr (l_fname,cur_link->filename);

             /* If other optional info was sent back, it must */
             /* also be parsed before inserting link     ***  */

             if (cur_link->linktype == 'L')
                vl_insert (cur_link,dir,vl_insert_flag);
             else
             {
               tmp = ul_insert (cur_link,dir,pul);

               /* If inserted after pul, next one after cur_link */
               if (pul && (!tmp || tmp == UL_INSERT_SUPERSEDING))
                   pul = cur_link;
             }
             break;

        case 'F': /* FILTER, FAILURE or FORWARDED */
             /* FORWARDED */
             if (!strncmp(line,"FORWARDED",9))
             {
               if (fwdcnt-- <= 0)
               {
                 ptlfree (resp);
                 return (perrno = PFS_MAX_FWD_DEPTH);
               }
               /* parse and start over */

               tmp = sscanf (line,"FORWARDED %s %s %s %s %d %d",
                             l_htype,l_host,l_ntype,l_fname,
                             &l_version, &l_magic);

               dhost = stcopy (l_host);
               dfile = stcopy (l_fname);

               if (tmp < 4)
               {
                 perrno = DIRSRV_BAD_FORMAT;
                 break;
               }

               ptlfree (resp);
               goto startover;
             }
             if (strncmp(line,"FILTER",6))
                goto scanerr;
             break;

        case 'M': /* MULTI-PACKET (processed by dirsend) */
        case 'P': /* PACKET (processed by dirsend) */
             break;

        case 'N': /* NOT-A-DIRECTORY or NONE-FOUND */
             /* NONE-FOUND, we just have no links to insert */
             /* It is not an error, but we must clear any   */
             /* old links in the directory arg              */
             if (!strncmp(line,"NONE-FOUND",10))
             {
               /* If only verifying, don't want to change dir */
               if (flags == GVD_VERIFY)
                  break;

               /* If first link and some links in dir, free them */
               if (!no_links++)
               {
                 if (dir->links)  vllfree (dir->links);
                 if (dir->ulinks) vllfree (dir->ulinks);
                 dir->links  = NULL;
                 dir->ulinks = NULL;
               }
               break;
             }
             /* If NOT-A-DIRECTORY or anything else, scan error */
             goto scanerr;

        case 'U': /* UNRESOLVED */
             if (strncmp(line,"UNRESOLVED",10))
                goto scanerr;
             break;

        case 'V': /* VERSION-NOT-SUPPORTED */
             if (!strncmp(line,"VERSION-NOT-SUPPORTED",21))
                return (perrno = DIRSRV_BAD_VERS);
             goto scanerr;

        scanerr:
        default:
             if (*line && (tmp = scan_error(line)) != 0)
             {
               ptlfree (resp);
               return (tmp);
             }
             break;
      }
    }
    resp = resp->next;
    ptfree (vtmp);
  }

  /* If only verifying, we already know it is a directory */
  if (flags == GVD_VERIFY)
     return (PSUCCESS);

  /* Don't return if matching was delayed by the need to filter    */
  /* if FIND specified, and dir->links is non null, then we have   */
  /* found a match, and should return.                             */
  if ((flags & GVD_FIND) && dir->links)
     return (PSUCCESS);

  /* If expand specified, and ulinks must be expanded, making sure */
  /* that the order of the links is maintained properly            */

  if (flags != GVD_UNION && flags != GVD_VERIFY)
  {
    l = dir->ulinks;

    /* Find first unexpanded ulink */
    while (l && l->expanded && l->linktype == 'U')
           l = l->next;

    /* Only expand if a FILE or DIRECTORY -  Mark as  */
    /* failed otherwise                               */
    /* We must still add support for symbolic ulinks  */
    if (l)
    {
      if (!strcmp(l->type,"DIRECTORY") || !strcmp(l->type,"FILE"))
      {
        l->expanded = TRUE;
        exp   = l;
        pul   = l;
        dhost = l->host;
        dfile = l->filename;
        goto startover;   /* was get_contents; */
      }
      else
        l->expanded = FAILED;
    }
  }

  return (PSUCCESS);
}
