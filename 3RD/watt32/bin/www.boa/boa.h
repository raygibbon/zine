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
 *  Adapted / rewritten for Waterloo tcp/ip and DOS
 *  G. Vanem 1998 <gvanem@yahoo.no>
 */

#ifndef _BOA_H
#define _BOA_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <conio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <tcp.h>

#include "defines.h"
#include "compat.h"            /* oh what fun is porting */

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

typedef struct request {       /* pending requests structure */
        int    fd;                   /* client's socket fd */
        int    status;               /* see #defines.h */
        int    simple;               /* simple request? */
        int    keepalive;            /* 0-disabled 2-close 3-continue */
        int    kacount;              /* kacount */

        u_long filesize;             /* filesize */
        FILE  *file;                 /* FILE for current file */

        int    time_last;            /* time of last succ. op. */
        int    method;               /* M_GET, M_POST, etc. */
        int    response_len;         /* Length of response[] */

        char   request_uri[MAX_HEADER_LENGTH+1]; /* uri */
        char   logline    [MAX_LOG_LENGTH+1];    /* line to log file */
        char   response   [RESPONSEBUF_SIZE+1];  /* data to send */
        char   accept     [MAX_HEADER_LENGTH+1]; /* Accept: fields */
        char   header_line[MAX_HEADER_LENGTH+1];
        int    headerpos;

        char   http_version[8];      /* HTTP/?.? of req */
        int    header_offset;        /* offset into headers */
        long   filepos;              /* position in file */
        int    response_status;      /* R_NOT_FOUND etc. */

        char  *ret_content_type;     /* content type to return */
        char  *if_modified_since;    /* If-Modified-Since */
        char  *user_agent;           /* User-Agent */
        char  *referer;              /* Referer */
        char  *remote_ip_addr;       /* after inet_ntoa */
        int    remote_port;          /* could be used for ident */
        time_t last_modified;        /* Last-modified: */

        /* CGI needed vars */

        int    is_cgi;               /* true if CGI/NPH */
        char **cgi_env;              /* CGI environment */
        int    cgi_env_index;        /* index into array */
        char **cgi_argv;             /* CGI argv */
        int   post_data_fd;          /* fd for post data tmpfile */

        char *path_info;             /* env variable */
        char *path_translated;       /* env variable */
        char *script_name;           /* env variable */
        char *query_string;          /* env variable */
        char *content_type;          /* env variable */
        char *content_length;        /* env variable */
        int   cgi_argc;              /* CGI argc */

        struct request *next;        /* next */
        struct request *prev;        /* previous */
        char  *post_file_name;       /* only used processing POST */
      } request;


typedef struct alias {
        char  *fakename;             /* URI path to file */
        char  *realname;             /* Actual path to file */
        int    type;                 /* ALIAS, SCRIPTALIAS, REDIRECT */
        int    fake_len;             /* strlen of fakename */
        int    real_len;             /* strlen of realname */
        struct alias *next;
      } alias;

extern int   server_s;               /* boa socket */
extern int   true;
extern char *optarg;                 /* For getopt */
extern FILE *yyin;                   /* yacc input */
extern int   sockbufsize;

extern request *request_ready;       /* first in ready list */
extern request *request_block;       /* first in blocked list */
extern request *request_free;        /* first in free list */

extern fd_set block_read_fdset;      /* fds blocked on read */
extern fd_set block_write_fdset;     /* fds blocked on write */

#define SQUASH_KA(req) ((req)->keepalive &= (~1))

/* global server variables */

extern char  *access_log_name;
extern char  *aux_log_name;
extern char  *error_log_name;

extern int   server_port;
extern char *server_admin;
extern char *server_root;
extern char *server_name;

extern char *document_root;
extern char *user_dir;
extern char *directory_index;
extern char *default_type;
extern char *cachedir;

extern int ka_timeout;
extern int ka_max;
extern int lame_duck_mode;


/* alias */

void add_alias (char *fakename, char *realname, int script);
int  translate_uri (request * req);
int  init_script_alias (request * req, alias * current);

/* boa */

void die (int exit_code);
void fdset_update (void);

/* config */

void  read_config_files (void);
void  add_user_dir (char *user, char *home_dir);
char *get_home_dir (char *name);

/* debug */

void print_set_fds (void);
void print_current_fds (void);

/* get */

int init_get (request * req);
int process_get (request * req);

/* hash */

void  add_mime_type (char *extension, char *type);
int   get_mime_hash_value (char *extension);
char *get_mime_type (char *filename);

/* lexer */

int yylex   (void);
int yyparse (void);
int yyerror (char *msg);

/* log */

void open_logs (void);
void close_access_log (void);
void close_error_log (void);
void log_access (request * req);
void log_error_doc (request * req);
void log_printf (const char *fmt, ...);
void boa_perror (request *req, int err, char *message);

/* queue */

void block_request (request * req);
void ready_request (request * req);
void dequeue (request ** head, request * req);
void enqueue (request ** head, request * req);

/* read */

int read_header (request * req);
int read_body (request * req);

/* request */

request *new_request (void);
void get_request (void);
void free_request (request ** list_head_addr, request * req);
void process_requests (void);
int  process_logline (request * req);
void process_header_line (request * req);
void add_accept_header (request * req, char * mime_type);

/* response */

void print_content_type (request * req);
void print_content_length (request * req);
void print_last_modified (request * req);
void print_http_headers (request * req);

void send_r_request_ok (request * req);                  /* 200 */
void send_redirect (request * req, char * url);          /* 302 */
void send_r_not_modified (request * req);                /* 304 */
void send_r_bad_request (request * req);                 /* 400 */
void send_r_forbidden (request * req);                   /* 403 */
void send_r_not_found (request * req);                   /* 404 */
void send_r_error (request * req);                       /* 500 */
void send_r_not_implemented (request * req);             /* 501 */

/* cgi */

void create_common_env (void);
void create_env (request * req);
void add_cgi_env (request * req, char * key, char * value);
void complete_env (request * req);
void call_cgi (request * req);

/* signals */

void sigint (int dummy);
void lame_duck_mode_run (void);

/* util */

void req_write (request * req, char * msg);
void flush_response (request * req);
void dosify_path (char *path);

void  clean_pathname (char * pathname);
char *get_commonlog_time (void);
char *get_rfc822_time (time_t t);
int   modified_since (time_t * mtime, char * if_modified_since);
int   month2int (char * month);
char *to_upper (char * str);
int   unescape_uri (char * uri);
char *escape_uri (char * uri);
void  create_tmp_dir (void);

#endif

