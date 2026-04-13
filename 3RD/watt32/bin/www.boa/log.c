/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@cebaf.gov>
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

char *error_log_name;
char *access_log_name;
char *aux_log_name;

static FILE *access_log = stdout;
static FILE *error_log  = stderr;

/*
 * Name: open_logs
 * 
 * Description: Opens up the error log, line buffers it (and
 * ties it to stderr)
 */
void open_logs (void)
{
  if ((access_log = fopen(access_log_name,APPEND_MODE)) == NULL)
  {
    fprintf (stderr, "Cannot open %s for logging: ", access_log_name);
    perror ("logfile open");
    exit (1);
  }
  setvbuf (access_log,NULL,_IOLBF,0);

  if (!error_log_name)
  {
    fprintf (stderr,"No ErrorLog directive specified in %s.\n",CONFIG_NAME);
    exit (1);
  }

  if ((error_log = fopen(error_log_name,APPEND_MODE)) == NULL)
     die (NO_OPEN_LOG);

#if REDIR_STDERR
  /* redirect stderr to error_log */
  dup2 (fileno(error_log),fileno(stderr));
  fclose (error_log);
  error_log = stderr;
#else
  atexit (close_error_log);
#endif
}

/*
 * Name: close_access_log
 * 
 * Description: closes access_log file
 */
void close_access_log (void)
{
  fclose (access_log);
}

/*
 * Name: close_error_log
 * 
 * Description: closes error_log file
 */
void close_error_log (void)
{
  fclose (error_log);
}

/*
 * Name: log_printf
 * 
 * Description: formatted print to error_log file (or stderr)
 */
void log_printf (const char *fmt, ...)
{
  int errno_save = errno;

  fprintf (error_log,get_commonlog_time());
  vfprintf (error_log,fmt,(&fmt)+1);
  errno = errno_save;
}

/*
 * Name: log_access
 * 
 * Description: Writes log data to access_log.
 */
void log_access (request *req)
{
  fprintf (access_log, "%s - - %s\"%s\" %d %ld\n",
           req->remote_ip_addr,
           get_commonlog_time(),
           req->logline,
           req->response_status,
           req->filepos);
}

/*
 * Name: log_error_doc
 *
 * Description: Logs the current time and transaction identification
 * to the stderr (the error log): 
 * should always be followed by another printf to error_log
 */
void log_error_doc (request *req)
{
  log_printf ("request from %s \"%s\": ",
              req->remote_ip_addr, req->logline);
}

/*
 * Name: boa_perror
 *
 * Description: logs an error to user and error file both
 */
void boa_perror (request *req, int err, char *message)
{
  send_r_error (req);
  log_error_doc (req);
  fprintf (error_log, "%s: %s\n", message, strerror(err));
  SQUASH_KA (req);   /* force close fd */
}
