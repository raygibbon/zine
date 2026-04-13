/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@dmacc.cc.ia.us>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@cebaf.gov>
 *  Some changes Copyright (C) 1996 Jon Nelson <nels0988@tc.umn.edu>
 *  Some changes Copyright (C) 1998 Gisle Vanem <gvanem@yahoo.no>
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

int    server_s;                        /* boa socket */
struct sockaddr_in server_sockaddr;	/* boa socket address */
struct timeval     req_timeout;         /* timeval for select */

fd_set block_read_fdset;
fd_set block_write_fdset;

int lame_duck_mode = 0;
int true           = 1;
int debug          = 0;

int main (int argc, char **argv)
{
  int    c;
  char  *dirbuf;
  size_t dirbuf_size;

  while ((c = getopt(argc,argv,"c:dt?h")) != EOF)
        switch(c)
        {
          case 'c':
               server_root = strdup (optarg);
               break;

          case 'd':
               dbug_init();
               break;

          case '?':
          case 'h':
          default:
               fprintf (stderr, "Usage: %s [-d] [-c serverroot]\n",argv[0]);
               return (1);
        }

  if (!server_root)
  {
#ifdef SERVER_ROOT
    server_root = SERVER_ROOT;
#else
    fprintf (stderr,"Boa: don't know where server root is.  "
             "Please #define SERVER_ROOT in boa.h\n"
             "and recompile, or use the -c command line option to specify it.\n");
    return (1);
#endif
  }

  if (chdir(server_root) == -1)
  {
    fprintf (stderr, "Could not chdir to %s: aborting\n", server_root);
    return (1);
  }

  if (server_root[0] != SLASH)
  {
    /* server_root (as specified on the command line) is
     * a relative path name. CGI programs require SERVER_ROOT
     * to be absolute.
     */
    dirbuf_size = MAX_PATH_LENGTH*2+1;
    if ((dirbuf = malloc(dirbuf_size)) == NULL)
    {
      fprintf (stderr, "boa: Cannot malloc %ld bytes of memory. Aborting.\n",
               dirbuf_size);
      return (1);
    }
    while (getcwd(dirbuf,dirbuf_size) == NULL)
    {
      if (errno == ERANGE)
      {
        dirbuf_size += MAX_PATH_LENGTH;
        if ((dirbuf = realloc(dirbuf,dirbuf_size)) == NULL)
        {
          fprintf (stderr,"boa: Cannot realloc %ld bytes of memory. Aborting.\n",
                   dirbuf_size);
          return (1);
        }
      }
      else if (errno == EACCES)
      {
        fprintf (stderr,"boa: getcwd() failed. "
                 "No read access in current directory. Aborting.\n");
        return (1);
      }
      else
      {
        perror ("getcwd");
        return (1);
      }
    }
    fprintf (stderr,
             "boa: Warning, the server_root directory specified"
             " on the command line,\n"
             "%s, is relative. CGI programs expect the environment\n"
             "variable SERVER_ROOT to be an absolute path.\n"
             "Setting SERVER_ROOT to %s to conform.\n", server_root, dirbuf);
    free (server_root);
    server_root = dirbuf;
  }

  sock_init();
  read_config_files();
  open_logs();
  create_common_env();

  log_printf ("boa: starting server\n");

//_fmode = O_BINARY;
  signal (SIGINT, sigint);
  create_tmp_dir();

  if ((server_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
     die (NO_CREATE_SOCKET);

  if ((setsockopt(server_s,SOL_SOCKET,SO_REUSEADDR,(void*)&true,sizeof(true))) == -1)
     die (NO_SETSOCKOPT);

  server_sockaddr.sin_family      = AF_INET;
  server_sockaddr.sin_port        = htons (server_port);
  server_sockaddr.sin_addr.s_addr = htonl (INADDR_ANY);

  if (bind(server_s,(struct sockaddr*)&server_sockaddr,sizeof(server_sockaddr)) == -1)
     die (NO_BIND);

  /* listen: large number just in case your kernel is nicely tweaked */

  if (listen(server_s,3) == -1)
     die (NO_LISTEN);

  fdset_update();

  /* main loop */

  while (1)
  {
    kbhit();   /* give our sigint handler a chance too */
    if (lame_duck_mode == 1)
       lame_duck_mode_run();

    if (!request_ready && !request_block && lame_duck_mode)
    {
      log_printf ("completing shutdown\n");
      return (15);
    }

    if (!request_ready)
    {
      if (select_s(FOPEN_MAX, &block_read_fdset, &block_write_fdset, NULL,
                   (request_block ? &req_timeout : NULL)) == -1)
         continue;

      if (FD_ISSET(server_s, &block_read_fdset))
         get_request();
      fdset_update();
      if (debug)
      {
        print_set_fds();
        print_current_fds();
      }
      /* move selected req's from request_block to request_ready */
    }

    process_requests();
    /* any blocked req's move from request_ready to request_block */
  }
}

/*
 * Name: fdset_update
 *
 * Description: iterate through the blocked requests, checking whether
 * that file descriptor has been set by select.  Update the fd_set to
 * reflect current status.
 */
void fdset_update (void)
{
  struct request *current, *next;

  current = request_block;

  while (current)
  {
    next = current->next;

    if (lame_duck_mode)
       SQUASH_KA (current);

    if (FD_ISSET(current->fd, &block_write_fdset) ||
        FD_ISSET(current->fd, &block_read_fdset))
    {
      ready_request (current);
    }
    else if (current->kacount && ((time(NULL) - current->time_last) > ka_timeout))
    {
      SQUASH_KA (current);
      free_request (&request_block, current);
    }
    else if ((time(NULL) - current->time_last) > REQUEST_TIMEOUT)
    {
      log_error_doc (current);
      log_printf ("connection timed out\n");
      SQUASH_KA (current);
      free_request (&request_block, current);
    }
    else
    {
      if (current->status == WRITE)
           FD_SET (current->fd, &block_write_fdset);
      else FD_SET (current->fd, &block_read_fdset);
    }
    current = next;
  }

  if (lame_duck_mode)
       FD_CLR (server_s, &block_read_fdset);
  else FD_SET (server_s, &block_read_fdset);    /* server always set */
  req_timeout.tv_sec  = (ka_timeout ? ka_timeout : REQUEST_TIMEOUT);
  req_timeout.tv_usec = 0l;                     /* reset timeout */
}

/*
 * Name: die
 * Description: die with fatal error
 */
void die (int exit_code)
{
  switch (exit_code)
  {
    case SERVER_ERROR:
         log_printf ("fatal error: exiting\n");
         break;
    case OUT_OF_MEMORY:
         perror ("malloc");
         break;
    case NO_CREATE_SOCKET:
         perror ("socket create");
         break;
    case NO_SETSOCKOPT:
         perror ("setsockopt");
         break;
    case NO_BIND:
         log_printf ("port %d: ", server_port);
         perror ("bind");
         break;
    case NO_LISTEN:
         perror ("listen");
         break;
    case NO_SETGID:
         perror ("setgid/initgroups");
         break;
    case NO_SETUID:
         perror ("setuid");
         break;
    case NO_OPEN_LOG:
         perror ("logfile fopen");    /* ??? */
         break;
    case NO_CREATE_TMP:
         log_printf ("unable to create cachedir \"%s\": \n", cachedir);
         perror ("mkdir");
         break;
    case WRONG_TMP_STAT:
         log_printf ("cachedir \"%s\" unusable\n", cachedir);
         perror ("chmod");
         break;
    default:
         break;
  }
  exit (exit_code);
}

