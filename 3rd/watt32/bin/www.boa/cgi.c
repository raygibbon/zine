/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *  Some changes Copyright (C) 1996 Larry Doolittle <ldoolitt@cebaf.gov>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@dmacc.cc.ia.us>
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

static char **common_cgi_env;

/*
 * Name: create_common_env
 *
 * Description: Set up the environment variables that are common to
 * all CGI scripts
 */
void create_common_env (void)
{
  char buffer [MAX_HEADER_LENGTH+1];
  int  index = 0;

  common_cgi_env = malloc (sizeof(char*) * COMMON_CGI_VARS);

  sprintf (buffer, "%s=%s", "PATH", DEFAULT_PATH);
  common_cgi_env[index++] = strdup (buffer);

  sprintf (buffer, "%s=%s", "SERVER_SOFTWARE", SERVER_VERSION);
  common_cgi_env[index++] = strdup (buffer);

  sprintf (buffer, "%s=%s", "SERVER_NAME", server_name);
  common_cgi_env[index++] = strdup (buffer);

  sprintf (buffer, "%s=%s", "GATEWAY_INTERFACE", CGI_VERSION);
  common_cgi_env[index++] = strdup (buffer);

  sprintf (buffer, "%s=%d", "SERVER_PORT", server_port);
  common_cgi_env[index++] = strdup (buffer);

  /* NCSA and APACHE added -- not in CGI spec */
  sprintf (buffer, "%s=%s", "DOCUMENT_ROOT", document_root);
  common_cgi_env[index++] = strdup (buffer);

  /* NCSA added */
  sprintf (buffer, "%s=%s", "SERVER_ROOT", server_root);
  common_cgi_env[index++] = strdup (buffer);

  /* APACHE added */
  sprintf (buffer, "%s=%s", "SERVER_ADMIN", server_admin);
  common_cgi_env[index++] = strdup (buffer);
}

/*
 * Name: create_env
 * 
 * Description: Allocates memory for environment before execing a CGI 
 * script.  I like spelling creat with an extra e, don't you?
 */
void create_env (request * req)
{
  char buffer [MAX_HEADER_LENGTH+1];
  int  i;

  req->cgi_env = malloc (sizeof(char*) * MAX_CGI_VARS);

  for (i = 0; i < COMMON_CGI_VARS; i++)
      req->cgi_env[i] = common_cgi_env[i];

  req->cgi_env_index = COMMON_CGI_VARS;

  sprintf (buffer, "%s=%s", "REQUEST_METHOD",
           ((req->method == M_POST) ? "POST" : "GET"));
  req->cgi_env[req->cgi_env_index++] = strdup (buffer);

  if (req->simple)
       sprintf (buffer, "%s=HTTP/0.9", "SERVER_PROTOCOL");
  else sprintf (buffer, "%s=HTTP/%s", "SERVER_PROTOCOL", req->http_version);

  req->cgi_env[req->cgi_env_index++] = strdup (buffer);

  if (req->path_info)
  {
    sprintf (buffer, "%s=%s", "PATH_INFO", req->path_info);
    req->cgi_env[req->cgi_env_index++] = strdup (buffer);
  }

  if (req->path_info)
       sprintf (buffer, "PATH_TRANSLATED=%s%s",document_root,req->path_info);
  else sprintf (buffer, "PATH_TRANSLATED=%s", "");

  req->cgi_env[req->cgi_env_index++] = strdup (buffer);

  sprintf (buffer, "%s=%s", "SCRIPT_NAME", req->script_name);
  req->cgi_env[req->cgi_env_index++] = strdup (buffer);

  if (req->query_string)
  {
    sprintf (buffer, "%s=%s", "QUERY_STRING", req->query_string);
    req->cgi_env[req->cgi_env_index++] = strdup (buffer);
  }

  sprintf (buffer, "%s=%s", "REMOTE_ADDR", req->remote_ip_addr);
  req->cgi_env[req->cgi_env_index++] = strdup (buffer);

  sprintf (buffer, "%s=%d", "REMOTE_PORT", req->remote_port);
  req->cgi_env[req->cgi_env_index++] = strdup (buffer);
}

/* 
 * Name: add_cgi_env
 *
 * Description: adds a variable to CGI's environment
 * Used for HTTP_ headers
 */
void add_cgi_env (request *req, char *key, char *value)
{
  char buffer[MAX_HEADER_LENGTH + 7];

  if (req->cgi_env_index < MAX_CGI_VARS - 6)   /* 5 in complete_env */
  {
    sprintf (buffer, "HTTP_%s=%s", key, value);
    req->cgi_env[req->cgi_env_index++] = strdup (buffer);
  }
}

/* 
 * Name: complete_env
 * 
 * Description: adds the known client header env variables
 * and terminates the environment array
 */
void complete_env (request * req)
{
  char buffer[MAX_HEADER_LENGTH + 1];

  if (req->method == M_POST)
  {
    if (req->content_type)
    {
      sprintf (buffer, "%s=%s", "CONTENT_TYPE", req->content_type);
      req->cgi_env[req->cgi_env_index++] = strdup (buffer);
    }
    if (req->content_length)
    {
      sprintf (buffer, "%s=%s", "CONTENT_LENGTH", req->content_length);
      req->cgi_env[req->cgi_env_index++] = strdup (buffer);
    }
  }

  if (req->user_agent)
  {
    sprintf (buffer, "%s=%s", "HTTP_USER_AGENT", req->user_agent);
    req->cgi_env[req->cgi_env_index++] = strdup (buffer);
  }

  if (req->referer)
  {
    sprintf (buffer, "%s=%s", "HTTP_REFERER", req->referer);
    req->cgi_env[req->cgi_env_index++] = strdup (buffer);
  }

  if (req->accept[0] != '\0')
  {
    sprintf (buffer, "%s=%s", "HTTP_ACCEPT", req->accept);
    req->cgi_env[req->cgi_env_index++] = strdup (buffer);
  }

  req->cgi_env[req->cgi_env_index] = NULL;            /* terminate */
}

/*
 * Name: make_args_cgi
 *
 * Build argv list for a CGI script according to spec
 *
 */
#define ARGC_MAX 128

static void create_argv (request *req, char **argv)
{
  char *p, *q, *r;
  int  argc;

  q = req->query_string;
  argv[0] = req->path_translated;

  if (q && !strchr(q,'='))
  {
    log_printf ("Parsing string %s\n",q); /* !! test */
    q = strdup (q);
    for (argc = 1; q && argc < ARGC_MAX; )
    {
      r = q;
      p = strchr (q,'+');
      if (p)
      {
        *p = '\0';
        q = p + 1;
      }
      else
        q = NULL;

      if (unescape_uri(r))
      {
        /* printf ("parameter %d: %s\n",argc,r); */
        argv[argc++] = r;
      }
    }
    argv[argc] = NULL;
  }
  else
    argv[1] = NULL;
}

/*
 * Name: fork_cgi_prog
 * 
 * Description: tie stdout to socket,
 * stdin to data if POST, and execute CGI script
 */
static void call_cgi_prog (request *req)
{
  char *argv [ARGC_MAX+1];

  /* Switch socket flags back to blocking
   */
  char on = 1;
  ioctlsocket (req->fd, FIONBIO, &on); /* cannot fail */

  /* fsext'ensions functions added in djgpp 2.02
   */
#if defined(__DJGPP__) && (DJGPP_MINOR >= 2)
  if (dup2(req->fd, STDOUT_FILENO) == -1)   /* tie stdout to socket */
  {
    perror ("dup2");
    return;
  }
#else
  /* pipe stdout to a temp file. After CGI finishes, print this
   * file to socket (man, what a mess!)
   */
#endif

  if (req->method == M_POST)              /* tie stdin to file */
  {
    lseek (req->post_data_fd, SEEK_SET, 0);
    dup2 (req->post_data_fd, STDIN_FILENO);
    close (req->post_data_fd);
  }

  create_argv (req,argv);
  if (execve(req->path_translated, argv, req->cgi_env) < 0)
  {
    log_printf ("path %s: ", req->path_translated);
    perror ("execve");
  }

  if (req->method == M_POST)
  {
    close (req->post_data_fd);
    unlink (req->post_file_name);
  }
}

/*
 * Name: call_cgi  (previously init_cgi)
 * 
 * Description: Called for GET/POST requests that refer to ScriptAlias 
 * directories or application/x-httpd-cgi files.  Ties stdout to socket,
 * stdin to data if POST, and execs CGI.
 */      
void call_cgi (request *req)
{
  SQUASH_KA (req);
  complete_env (req);

  if (req->is_cgi != NPH)          /* have to precede execve() */
     send_r_request_ok (req);

  log_printf ("Spawning: \"%s\"\n", req->path_translated);
  call_cgi_prog (req);
}

