/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@cebaf.gov>
 *  Some changes Copyright (C) 1996 Jon Nelson <nels0988@tc.umn.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "boa.h"

static char *get_file_size   (DWORD size);
static FILE *get_dir_file    (request * req, struct stat *statbuf);
static int   index_directory (request *req, char *dest_filename);

/*
 * Name: init_get
 * Description: Initializes a non-script GET or HEAD request. 
 * 
 * Return values:
 *   0: finished or error, request will be freed
 *   1: successfully initialized, added to ready queue
 */      
int init_get (request *req)
{
  struct stat statbuf;
  int    was_dir = 0;
  char  *p;

  clean_pathname (req->path_translated);  /* remove double slashes etc. */
  dosify_path (req->path_translated);
  stat (req->path_translated, &statbuf);

  req->file    = NULL;
  req->filepos = 0;

  if (S_ISDIR(statbuf.st_mode))             /* directory */
  {
    was_dir = 1;
    p = &req->path_translated [strlen(req->path_translated)-1];
    if (*p != SLASH)
    {
      char buffer [3*MAX_PATH_LENGTH+128];

      *(++p) = SLASH;
      *p     = '\0';
      sprintf (buffer, "http://%s:%d%s/", server_name, server_port,
               escape_uri(req->request_uri));
      req->ret_content_type = strdup ("text/html");
      send_redirect (req, buffer);
      return (0);
    }
    req->file = get_dir_file (req,&statbuf);   /* updates statbuf */
    if (!req->file)                            /* couldn't do it  */
       return (0);                /* errors reported by get_dir_file */
  }
  if (!was_dir)
  {
    req->file = fopen (req->path_translated, "rb");
    if (!req->file)
    {
      log_error_doc (req);
      perror ("document open");
      if (errno == ENOENT)
           send_r_not_found (req);
      else if (errno == EACCES)
           send_r_forbidden (req);
      else send_r_bad_request (req);
      return (0);
    }
    fstat (fileno(req->file),&statbuf);
  }

  if (req->if_modified_since &&
      !modified_since(&statbuf.st_mtime,req->if_modified_since))
  {
    send_r_not_modified (req);
    if (req->file)
       fclose (req->file);
    req->file = NULL;
    return (0);
  }

  req->filesize      = statbuf.st_size;
  req->last_modified = statbuf.st_mtime;

  if (req->method == M_HEAD)
  {
    send_r_request_ok (req);
    fclose (req->file);
    return (0);
  }

  send_r_request_ok (req);  /* All's well */

  /* We lose statbuf here, so make sure response has been sent */
  return (1);
}

/*
 * Name: process_get
 * Description: Writes a chunk of data to the socket.
 * 
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */                                
int process_get (request *req)
{
  int   written;
  int   size = min (sockbufsize, req->filesize - req->filepos);
  FILE *file = req->file;
  char *buf  = malloc (size);

  if (!buf)
  {
    log_printf ("process_get(): malloc\n");
    return (0);
  }
  if (!file)
  {
    log_printf ("process_get(): file not open\n");
    return (0);
  }

  fseek (file,req->filepos,SEEK_SET);
  if (fread(buf,size,1,file) != size)
  {
    log_printf ("process_get(): read\n");
    return (0);
  }

  written = write_s (req->fd,buf,size);
  if (written < 0)
  {
    if (errno == EAGAIN ||    /* no room in pipe */
        errno == EWOULDBLOCK)
       return (-1);

      /* something wrong, includes EPIPE possibility */
#if 1
    log_error_doc (req);    /* Can generate lots of log entries,      */
    perror ("write_s");     /* OK to disable if your logs get too big */
#endif
    SQUASH_KA (req);        /* force close fd */
    return (0);

  }
  req->filepos  += written;
  req->time_last = time (NULL);

  if (req->filepos == req->filesize)      /* EOF */
       return (0);                        /* req->file will be closed */
  else return (1);                        /* more to do */
}

/*
 * Name: get_dir_file
 * Description: Called from init_get if the request is a directory.
 * statbuf must describe directory on input, since we may need its
 *   device, inode, and mtime.
 * statbuf is updated, since we may need to check mtimes of a cache.
 * returns file handle, or NULL on error.
 * Sequence of places to look:
 *  1. User generated index.html
 *  2. Previously generated cachedir file
 *  3. cachedir file we generate now
 * Cases 2 and 3 only work if cachedir is enabled, otherwise
 * issue a 403 Forbidden.
 */                         
static FILE *get_dir_file (request *req, struct stat *statbuf)
{
  char   pathname_with_index[MAX_PATH_LENGTH];
  time_t real_dir_mtime;
  FILE  *file;

  sprintf (pathname_with_index,"%s%s",req->path_translated,directory_index);

  file = fopen (pathname_with_index,"rb");
  if (file)                                      /* user's index file */
  {
    strcpy (req->request_uri, directory_index);  /* for mimetype */
    fstat (fileno(file), statbuf);
    return (file);
  }
  if (cachedir == NULL)
  {
    send_r_forbidden (req);
    return (NULL);
  }

  real_dir_mtime = statbuf->st_mtime;
  sprintf (pathname_with_index, "%s%c%d_%d.dir",
           cachedir, SLASH, statbuf->st_dev, statbuf->st_ino);

  file = fopen (pathname_with_index, READ_MODE);  /* index cache */
  if (file)                 
  {
    fstat (fileno(file),statbuf);
    if (statbuf->st_mtime > real_dir_mtime)
    {
      statbuf->st_mtime = real_dir_mtime;          /* lie */
      strcpy (req->request_uri, directory_index);  /* for mimetype */
      return (file);
    }
    fclose (file);
    unlink (pathname_with_index);    /* cache is stale, delete it */
  }
  if (index_directory(req,pathname_with_index) == -1)
     return (NULL);

  file = fopen (pathname_with_index,"rb");         /* Last chance */
  if (file)
  {
    strcpy (req->request_uri, directory_index);    /* for mimetype */
    fstat (fileno(file), statbuf);
    statbuf->st_mtime = real_dir_mtime;            /* lie */
    return (file);
  }

  boa_perror (req,errno,"re-opening dircache");
  return (NULL);    /* Nothing worked. */
}

/*
 * Name: index_directory
 * Description: Called from get_dir_file if a directory html
 * has to be generated on the fly
 * returns -1 for problem, else 0
 */
static int index_directory (request *req, char *dest_filename)
{
  DIR   *request_dir;
  FILE  *fdstream;
  struct stat    statbuf;
  struct dirent *dirbuf;
  int    bytes = 0;

  if (chdir(req->path_translated) == -1)
  {
    if (errno == EACCES || errno == EPERM)
       send_r_forbidden (req);
    else
    {
      log_error_doc (req);
      perror ("chdir");
      send_r_bad_request (req);
    }
    return (-1);
  }

  request_dir = opendir (".");
  if (!request_dir)
  {
    send_r_error (req);
    log_printf ("directory \"%s\": opendir: %s\n",  /* debug */
                req->path_translated, strerror(errno));
    return (-1);
  }

  fdstream = fopen (dest_filename, WRITE_MODE);
  if (!fdstream)
  {
    boa_perror (req,errno,"dircache fopen");
    return (-1);
  }

  bytes += fprintf (fdstream,
                    "<HTML><HEAD>\n<TITLE>Index of %s</TITLE>\n</HEAD>\n\n",
                    req->request_uri);
  bytes += fprintf (fdstream, "<BODY>\n\n<H2>Index of %s</H2>\n\n<PRE>\n",
                    req->request_uri);

  while ((dirbuf = readdir(request_dir)) != NULL)
  {
    if (!strcmp(dirbuf->d_name, "."))
       continue;

    if (!strcmp(dirbuf->d_name, ".."))
    {
      bytes += fprintf (fdstream,
                        " [DIR] <A HREF=\"../\">Parent Directory</A>\n");
      continue;
    }

    if (stat(dirbuf->d_name, &statbuf) == -1)
       continue;

    if (S_ISDIR(statbuf.st_mode))
    {
      bytes += fprintf (fdstream, " [DIR] <A HREF=\"%s/\">%s</A>\n",
                        escape_uri(dirbuf->d_name), dirbuf->d_name);
    }
    else
    {
      bytes += fprintf (fdstream,
                        "[FILE] <A HREF=\"%s\">%s</A> (%s)\n",
                        escape_uri(dirbuf->d_name), dirbuf->d_name,
                        get_file_size(statbuf.st_size));
    }
  }

  closedir (request_dir);
  bytes += fprintf (fdstream, "</PRE>\n\n</BODY>\n</HTML>\n");
  fclose (fdstream);
  chdir (server_root);

  req->filesize = bytes;              /* for logging transfer size */
  return (0);                         /* success */
}


/*
 * Name: get_filesize
 * Description: Return filesize string formatted as bytes, kB or MB
 * with 1 decimal. Field is 6 characters, right adjusted
 */
static char *get_file_size (DWORD size)
{
  static char buf[30];
#define _1MB_ (1024*1024)

  if (size < 1024)
       sprintf (buf, "%6lu bytes",(unsigned long)size);
  else if (size <= _1MB_)
       sprintf (buf,"%4.1f kB", (double)size/1024);
  else sprintf (buf,"%4.1f MB", (double)size/_1MB_);
  return (buf);
}
