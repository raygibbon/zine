#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <time.h>
#include <io.h>
#include <sys/stat.h>

#include "ftp.h"

static jmp_buf     bail_out;
static FILE       *log;
static char       *iniBuffer;  /* must not be free'd */
static char       *tok;
static const char *iniFile;

enum Sections {
     Options = 1,
     Colours,
     AutoLogin
   };

struct FtpConfig cfg = { { LIGHTGRAY,BLACK,    /* user colour   */
                           LIGHTGRAY,BLACK,    /* data colour   */
                           LIGHTGRAY,BLACK,    /* ctrl colour   */
                           LIGHTGRAY,BLACK,    /* warn colour   */
                           BLACK,CYAN,         /* status colour */
                           LIGHTGRAY,BLACK,    /* DOS colours   */
                         },
                         0,                    /* auto-login data         */
                         0,                    /* macros definitions      */
                         NULL,                 /* log-file name           */
                         80,25,                /* screen width/height     */
                         0,                    /* status-line row (0=off) */
                         FTP_DATA_PORT,
                         FTP_CTRL_PORT,
                         DATACONN_TIME,
                         DATAWAIT_TIME,
                         KEEPALIVE_TIME,
                         BLK_SIZE,             /* send_socksize */
                         BLK_SIZE,             /* recv_socksize */
                         1,                    /* do_ping       */
                         0,                    /* bell_mode     */
                         1,                    /* auto_login    */
                         1,                    /* allow_syst    */
                         0,                    /* initial_syst  */
                         0,                    /* hash_marks    */
                         1,                    /* verbose       */
                         1,                    /* use_comspec   */
                         1,                    /* do_ansi_test  */
                         0,                    /* tracing       */
                         "none"                /* auth_type     */
                       };

STATIC void  ParseIniFile(char *buf, char *end);
STATIC char *NextToken   (void);
STATIC void  GetMacroDef (char **arg);
STATIC void  CheckMachine(char *host);
STATIC void *AllocLogin  (void);
STATIC void *AllocMacro  (void);
STATIC void  UnknownTok  (char *token);
STATIC int   BoolValue   (const char *str);

/*-----------------------------------------------------------------*/

int OpenIniFile (const char *name)
{
  static char *p = NULL;
  struct stat st;
  FILE  *fil;
  char   fname [_MAX_PATH];
  long   alloc;

  iniFile = name;
  sprintf (fname, "%s%c%s", progPath, SLASH, name);

  if (stat(fname,&st) < 0)
  {
    fprintf (stderr, "Cannot find `%s': %s\7\n", fname, strerror(errno));
    sleep (1);
    return (1);
  }

  fil = fopen (fname, "rt");
  if (!fil)
  {
    fprintf (stderr, "Cannot open `%s'\n", fname);
    return (0);
  }

  alloc = st.st_size;
  iniBuffer = calloc (alloc, 1);
  if (!iniBuffer)
  {
    fclose (fil);
    fprintf (stderr, "No memory to process `%s'\n", fname);
    return (0);
  }

  p = iniBuffer;

  while (!feof(fil) && p < iniBuffer + alloc)
  {
    const char *env;
    char       *c;

    fgets (p, alloc - (int)(p-iniBuffer), fil);
    env = _w32_expand_var_str ((char*)p);
    if ((char*)env != p)
       strcpy (p, env);

    c = strchr (p, '#');  if (c) *(p = c) = '\n';
    c = strchr (p, ';');  if (c) *(p = c) = '\n';
    c = strchr (p, '\n'); if (c) p = c+1;

    while (*p == ' ')   /* strip trailing spaces */
          --p;
  }
  fclose (fil);

  if (!setjmp(bail_out))
     ParseIniFile (iniBuffer, p);

#if 0    /* test */
  {
    struct Colour    *col = &cfg.colour;
    struct LoginData *login;
    struct MacroData *macro;

    printf ("log_file  = %s\n",    cfg.log_file);
    printf ("user_text = %d/%d\n", col->user_fg,col->user_bg);
    printf ("data_text = %d/%d\n", col->data_fg,col->data_bg);
    printf ("ctrl_text = %d/%d\n", col->ctrl_fg,col->ctrl_bg);
    printf ("warn_text = %d/%d\n", col->warn_fg,col->warn_bg);
    printf ("stat_text = %d/%d\n", col->stat_fg,col->stat_bg);
    printf ("data_port = %d\n",    cfg.data_port);
    printf ("ctrl_port = %d\n",    cfg.ctrl_port);
    printf ("dataconn_to  = %d\n", cfg.dataconn_to);
    printf ("datawait_to  = %d\n", cfg.datawait_to);
    printf ("keepalive_to = %d\n", cfg.keepalive_to);
    printf ("do_ping      = %d\n", cfg.do_ping);

    for (login = cfg.login; login; login = login->next)
    {
      printf ("default %d, machine \"%s\": (%s,%s,%s)\n",
              login->def, login->host, login->user,
              login->pass,login->acct);
    }
    for (macro = cfg.macro; macro; macro = macro->next)
    {
      int i;

      printf ("mac-name: `%s' =", macro->name);
      for (i = 0; i < MAX_MACDEF && macro->def[i]; i++)
          printf (" `%s'", macro->def[i]);
      puts ("");
    }
    exit (0);
  }
#endif

  return (1);
}

/*-----------------------------------------------------------------*/

STATIC void ParseIniFile (char *buf, char *end)
{
  enum   Sections  section = Options;
  struct Colour    *col    = &cfg.colour;
  struct LoginData *login  = NULL;
  struct MacroData *macro  = NULL;

  while (buf < end)
  {
    buf = tok = strtok (buf, " \t\n");
    if (!buf)
       break;

    if (!stricmp(tok,"[options]"))
            section = Options;

    else if (!stricmp(tok,"[colours]"))
            section = Colours;

    else if (!stricmp(tok,"[autologin]"))
            section = AutoLogin;

    else if (section == Options)
    {
      if (!stricmp(tok,"log_file"))
              cfg.log_file = NextToken();

      else if (!stricmp(tok,"status_line"))
              cfg.status_line = atoi (NextToken());

      else if (!stricmp(tok,"data_port"))
              cfg.data_port = atoi (NextToken());

      else if (!stricmp(tok,"ctrl_port"))
              cfg.ctrl_port = atoi (NextToken());

      else if (!stricmp(tok,"dataconn_to"))
              cfg.dataconn_to = atoi (NextToken());

      else if (!stricmp(tok,"datawait_to"))
              cfg.datawait_to = atoi (NextToken());

      else if (!stricmp(tok,"keepalive_to"))
              cfg.keepalive_to = atoi (NextToken());

      else if (!stricmp(tok,"send_socksize"))
              cfg.send_socksize = atoi (NextToken());

      else if (!stricmp(tok,"recv_socksize"))
              cfg.recv_socksize = atoi (NextToken());

      else if (!stricmp(tok,"language"))
              ftp_lang_init (NextToken());

      else if (!stricmp(tok,"auth_type"))
              cfg.auth_type = strupr (NextToken());

      else if (!stricmp(tok,"interactive"))
              interactive = BoolValue (NextToken());

      else if (!stricmp(tok,"hash_marks"))
              cfg.hash_marks = BoolValue (NextToken());

      else if (!stricmp(tok,"verbose"))
              cfg.verbose = BoolValue (NextToken());

      else if (!stricmp(tok,"use_comspec"))
              cfg.use_comspec = BoolValue (NextToken());

      else if (!stricmp(tok,"tracing"))
              cfg.tracing = BoolValue (NextToken());

      else if (!stricmp(tok,"passive"))
              passive = BoolValue (NextToken());

      else if (!stricmp(tok,"auto_login"))
              cfg.auto_login = BoolValue (NextToken());

      else if (!stricmp(tok,"initial_syst"))
              cfg.initial_syst = BoolValue (NextToken());

      else if (!stricmp(tok,"allow_syst"))
              cfg.allow_syst = BoolValue (NextToken());

      else if (!stricmp(tok,"bell_mode"))
              cfg.bell_mode = BoolValue (NextToken());

      else if (!stricmp(tok,"do_ping"))
              cfg.do_ping = BoolValue (NextToken());

      else if (!stricmp(tok,"do_ansi_test"))
              cfg.do_ansi_test = BoolValue (NextToken());

      else if (!stricmp(tok,"use_parse"))
              use_parse = BoolValue (NextToken());
    }

    else if (section == Colours)
    {
      if (!stricmp(tok,"user_text"))
              sscanf (NextToken(), "%d,%d", &col->user_fg, &col->user_bg);

      else if (!stricmp(tok,"data_text"))
              sscanf (NextToken(), "%d,%d", &col->data_fg, &col->data_bg);

      else if (!stricmp(tok,"ctrl_text"))
              sscanf (NextToken(), "%d,%d", &col->ctrl_fg, &col->ctrl_bg);

      else if (!stricmp(tok,"warn_text"))
              sscanf (NextToken(), "%d,%d", &col->warn_fg, &col->warn_bg);

      else if (!stricmp(tok,"stat_text"))
              sscanf (NextToken(), "%d,%d", &col->stat_fg, &col->stat_bg);

      else UnknownTok (tok);
    }

    else if (section == AutoLogin)
    {
      if (!stricmp(tok,"default"))
      {
        login = (struct LoginData*) AllocLogin();
        login->def   = 1;
        login->host  = "*";
        CheckMachine ("*");
      }
      else if (!stricmp(tok,"machine"))
      {
        login = (struct LoginData*) AllocLogin();
        login->host  = NextToken();
        CheckMachine (login->host);
      }
      else if (!stricmp(tok,"login"))
              login->user = NextToken();
      else if (!stricmp(tok,"password"))
              login->pass = NextToken();
      else if (!stricmp(tok,"account"))
              login->acct = NextToken();
      else if (!stricmp(tok,"macdef"))
      {
        macro = (struct MacroData*) AllocMacro();
        macro->name = NextToken();
        GetMacroDef (macro->def);
      }
      else
        UnknownTok (tok);
    }
    buf = 1 + strchr (tok, '\0');
  }
}

STATIC char *NextToken (void)
{
  tok = strtok (NULL, "= \t\n");
  if (!tok)
     longjmp (bail_out, 1);
  return (tok);
}

STATIC void UnknownTok (char *token)
{
  fprintf (stderr, "%s: Unknown keyword `%s'\n", iniFile, token);
  sleep (2);
}

STATIC int BoolValue (const char *str)
{
  const char *p = str;

  while (*p == ' ' || *p == '\t')
        p++;

  if ((*p >= '1' && *p <= '9') ||
      !stricmp(p,"on") || !stricmp(p,"yes") || !stricmp(p,"true"))
     return (1);
  return (0);
}

STATIC char *NextMacro (void)
{
  char  *str;
  int    quote   = 0;
  static int end = 0;

  str = tok;

  while (1)
  {
    switch (*str)
    {
      case '\n':
           if (!quote)
           {
             if (end)
             {
               end  = 0;
               *str = 0;
               return (NULL);
             }
             end = (*(str+1) == '\n');
             goto got_it;
           }
           break;

      case '"':
           if ((quote ^= 1) == 1)
           {
             tok++;
             break;
           }
           goto got_it;

      case ' ' :
      case '\t':
           if (!quote)
              goto got_it;
    }
    str++;
  }

got_it:
  *str = 0;
  return (tok);
}

STATIC void GetMacroDef (char **arg)
{
  int  i;
  char *p;

  for (i = 0; i < MAX_MACDEF; i++)
  {
    tok = 1 + strchr (tok, 0);
    p = NextMacro();
    if (!p)
       break;
    if (*p)
       arg[i] = p;
  }
  for ( ; i < MAX_MACDEF-1; i++)
      arg[i] = NULL;
}

STATIC void CheckMachine (char *host)
{
  struct LoginData *login;
  int    num = 0;

  for (login = cfg.login; login; login = login->next)
      if (!stricmp(login->host,host))
         num++;

  if (num > 1)
  {
    if (host[0] == '*')
         fprintf (stderr, "\7%s: default login multiple defined\n", iniFile);
    else fprintf (stderr, "\7%s: machine `%s' multiple defined\n", iniFile, host);
    sleep (1);
  }
}

STATIC void *AllocLogin (void)
{
  struct LoginData *login = calloc (sizeof(*login),1);

  if (!login)
  {
    fprintf (stderr, "%s: No memory for login data\n", iniFile);
    longjmp (bail_out, 1);
  }
  login->next = cfg.login;
  cfg.login   = login;
  return ((void*)login);
}

STATIC void *AllocMacro (void)
{
  struct MacroData *macro = calloc (sizeof(*macro), 1);

  if (!macro)
  {
    fprintf (stderr, "%s: No memory for macro data\n", iniFile);
    longjmp (bail_out, 1);
  }
  macro->next = cfg.macro;
  cfg.macro   = macro;
  return ((void*)macro);
}

/*-----------------------------------------------------------------*/

int GetUserPass (struct URL *url)
{
  struct LoginData *login = NULL;
  struct LoginData *def   = NULL;

  for (login = cfg.login; login; login = login->next)
  {
    if (login->def)
    {
      def = login;
      continue;
    }
    if (!stricmp(login->host,url->host))
       break;
  }

  if (def && !login)
     login = def;
  if (login)
  {
    if (login->user)
         _strncpy (url->user, login->user, sizeof(url->user));
    else url->user[0] = 0;

    if (login->pass)
         _strncpy (url->pass, login->pass, sizeof(url->pass));
    else url->pass[0] = 0;

    if (login->acct)
         _strncpy (url->acct, login->acct, sizeof(url->acct));
    else url->acct[0] = 0;
  }
  return (login != NULL);
}


/*
 * Slice a string up into argc/argv.
 */
char **MakeArgv (char *str, int *argc)
{
  static char *argv [MAX_ARGC];
  char  *tok  = strtok (str, "\t ");
  int    i, numarg = 0;

  for (i = 0; i < MAX_ARGC; i++)
  {
    if (tok)
         argv[i] = tok, numarg++;
    else argv[i] = NULL;
    tok = strtok (NULL, "\t ");
  }
  *argc = numarg;
  return (argv);
}

/*
 * First check if log-file starts with "~/". If so set log-file
 * name to path of program. Open log-file and return 'FILE*'
 */
STATIC FILE *OpenLogFile (void)
{
  if (cfg.log_file[0] == '~' &&    /* "~/" -> home-directory */
      cfg.log_file[1] == '/')
  {
    static char fname [_MAX_PATH];
    sprintf (fname, "%s%c%s", progPath, SLASH, cfg.log_file+2);
    cfg.log_file = fname;
  }
  return fopen (cfg.log_file, "at");
}

/*-----------------------------------------------------------------------*/

static char *version (void)
{
#if defined(__DJGPP__)
  return ("djgpp");
#elif defined(__HIGHC__)
  return ("Metaware HighC");
#elif defined(__BORLANDC__)
  return ("Borland");
#elif defined(__WATCOMC__)
  return ("Watcom");
#elif defined(_MSC_VER)
  return ("MSVC");
#elif defined(__MINGW32__)
  return ("MingW");
#elif defined(__CYGWIN__)
  return ("CygWin");
#else
  return ("??");
#endif
}

int ftp_log_init (void)
{
  if (!cfg.log_file || log)
     return (1);

  log = OpenLogFile();
  if (log && fputs("\n",log) != EOF)
  {
    ftp_log ("%s%c%s (%s) started", progPath, SLASH, progName, version());
    atexit (ftp_log_exit);
    return (1);
  }
  printf ("Cannot open `%s'\n", cfg.log_file);
  return (0);
}

int ftp_log (const char *fmt, ...)
{
  if (log)
  {
    va_list args;
    char    buf[512];
    time_t  now = time (NULL);

    va_start (args, fmt);
    VSPRINTF (buf, _LANG(fmt), args);
    va_end (args);
    return fprintf (log, "%.24s:  %s\n", ctime(&now), buf);
  }
  return (0);
}

void ftp_log_raw (const char *str)
{
  fputs (str, log ? log : stderr);
}

void ftp_log_flush (void)
{
  if (log)
     fflush (log);
}

void ftp_log_pause (void)
{
  if (log)
  {
    fclose (log);
    log = NULL;
  }
}

void ftp_log_continue (void)
{
  if (cfg.log_file)
     log = OpenLogFile();
}

void ftp_log_exit (void)
{
  struct LoginData *login, *next_l;
  struct MacroData *macro, *next_m;

  if (log)
  {
    ftp_log ("%s%c%s stopped", progPath, SLASH, progName);
    fclose (log);
    log = NULL;
  }

  for (login = cfg.login; login; login = next_l)
  {
    next_l = login->next;
    free (login);
  }

  for (macro = cfg.macro; macro; macro = next_m)
  {
    next_m = macro->next;
    free (macro);
  }

  if (iniBuffer)
     free (iniBuffer);
  cfg.login = NULL;
  cfg.macro = NULL;
  iniBuffer = NULL;
}

