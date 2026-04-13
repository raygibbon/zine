#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h>

#include "ftp.h"

int     interactive   = 1;
int     passive       = 0;
int     quit          = 0;
int     debug_mode    = 0;
WORD    initial_port  = 0;
char   *initial_url   = NULL;
FILE   *script_file   = NULL;
char   *auth_type     = NULL;
int     script_echo   = 1;
int     fast_exit     = 0;
int     use_parse     = 0;
int     curr_drive    = 2;
int     home_drive    = 2;
int     editing       = 0;
int     is_init       = 0;
int     c_flag        = 0;
int     proxy_flag    = 0;
int     prot_level    = 0;
int     sigint_active = 0;
int     stdin_mode    = O_TEXT;
int     in_a_shell    = 0;

jmp_buf cmdline;
int   (*background)(void);
void   *ctrl_socket;
void   *data_socket;

char    progPath   [_MAX_PATH] = ".";
char    progName   [_MAX_PATH] = "ftp";
char    local_dir  [_MAX_PATH];
char    local_home [_MAX_PATH];

static char *argv0_copy;

STATIC int  GetCommand (char *cmd, int size);
STATIC int  GetCmdLine (int argc, char **argv);
STATIC void InitMain   (const char *argv0);
STATIC int  OpenURL    (void);

STATIC void Exit (const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;

/*-----------------------------------------------------------------------*/

STATIC void Usage (int details)
{
#ifdef USE_SRP
  char *srp_cmd = "\n           [-c cipher] [-h hash]";
#else
  char *srp_cmd = "";
#endif

  if (details)
     printf (_LANG("%s\ntcp/ip kernel: %s\n\n"), FTP_VERSION, wattcpVersion());

  printf (_LANG("Usage: ftp [-] [-?idvntcCpP] [-s script] [-$ macro] %s"
                " [host [port] | url]\n"), srp_cmd);

  if (details)
  {
    puts (_LANG("options: -   run commands from stdin"));
    puts (_LANG("         -i  non-interactive prompts for mget/mput"));
    puts (_LANG("         -d  enable debugging"));
    puts (_LANG("         -v  turn off verbose information"));
    puts (_LANG("         -n  don't use autologin information"));
    puts (_LANG("         -t  enable packet tracing"));
    puts (_LANG("         -S  inhibit the SYST command"));
    puts (_LANG("         -C  send SYST after connect"));
    puts (_LANG("         -p  sets passive mode"));
    puts (_LANG("         -P  use EPLF dir-list parsing"));
    puts (_LANG("         -s  run the commands from script-file (- = stdin)"));
    puts (_LANG("         -$  run macro after login (default $init)"));
#ifdef USE_SRP
    puts (_LANG("         -c  set initial cipher for SRP"));
    puts (_LANG("         -h  set initial hash for SRP"));
#endif
  }
  exit (0);
}

/*-----------------------------------------------------------------------*/

STATIC void Exit (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vfprintf (stdout, fmt, args);
  va_end (args);
  exit (1);
}

/*-----------------------------------------------------------------------*/

int main (int argc, char **argv)
{
  InitMain (argv[0]);

  if (!OpenIniFile("ftp.ini"))
     return (1);

  if (!GetCmdLine(argc,argv))
     return (2);

  if (!ftp_log_init())
     return (3);

  if (!ftp_init())
     return (4);

  if (!ScreenInit())
     return (5);

  if (!OpenURL())
     return (6);

  setjmp (cmdline);

  while (!quit)
  {
    char cmd[512];

    if (GetCommand(cmd,sizeof(cmd)))
    {
      char   raw [sizeof(cmd)];
      int    argc;
      char **argv;

      StatusLine (XferStatistics, NULL);
      strcpy (raw, cmd);
      argv = MakeArgv (cmd, &argc);
      DoCommand (raw, argc, argv);
    }
  }
  return (0);
}

/*--------------------------------------------------------------------*/

STATIC int GetCmdLine (int argc, char **argv)
{
  static char opts[30] = "?idvntSCpPs:$:";
  int    ch;

#ifdef USE_SRP
  strcat (opts, "c:h:");
#endif

  while ((ch = getopt(argc, argv, opts)) != EOF)
     switch (ch)
     {
       case 'i': interactive = 0;
                 break;

       case 'd': debug_mode = 1;
                 break;

       case 'v': cfg.verbose = 0;
                 break;

       case 'n': cfg.auto_login = 0;
                 break;

       case 't': cfg.tracing = 1;
                 break;

       case 'S': cfg.allow_syst = 0;
                 break;

       case 'C': cfg.initial_syst = 1;
                 break;

       case 'p': passive = 1;
                 break;

       case 'P': use_parse = 1;
                 break;

       case 's': if (!strcmp(optarg,"-"))
                 {
                   script_file = stdin;
                   /* stdin_mode = setmode (FILENO(stdin), BINARY_MODE); */
                 }
                 else
                 {
                   if (*optarg == ':')
                      optarg++;
                   script_file = fopen (optarg, "rt");
                 }
                 if (!script_file)
                    Exit (_LANG("Cannot open scriptfile `%s'\n"), optarg);
                 break;

       case '$': init_macro = optarg;
                 if (!FindMacro(init_macro))
                    Exit (_LANG("`%s' macro not found\n"), init_macro);
                 break;
#ifdef USE_SRP
       case 'c': if (srp_selcipher(optarg) < 0)
                    Exit (_LANG("Setting SRP cipher `%s' failed\n"), optarg);
                 break;

       case 'h': if (srp_selhash(optarg) < 0)
                    Exit (_LANG("Setting SRP hash `%s' failed\n"), optarg);
                 break;
#endif
       case '?': Usage (1);
       default : Usage (0);
     }

  argv += optind;
  argc -= optind;

  if (argc-- > 0)
     initial_url  = *argv++;

  if (argc-- > 0)
     initial_port = atoi (*argv++);

  return (1);
}

/*--------------------------------------------------------------------*/

STATIC int GetCommand (char *cmd, int size)
{
  memset (cmd, 0, size);

  if (is_init)
  {
    tcp_tick (ctrl_socket);
    DoKeepAlive();
  }

  c_flag = 0;

  if (script_file)
  {
    if (!fgets(cmd,size-1,script_file))
    {
      CloseScript (0);
      c_flag = 1;
      return (0);
    }
    while (*cmd == ' ' || *cmd == '\t')
          ++cmd;
    if (*cmd == ';' || *cmd == '#')
    {
      c_flag = 1;
      return (0);
    }
  }
  else
  {
    editing = 1;
    ShowPrompt();
    cgets_ed (cmd, size-1, 1, NULL, background);
    xputs (UserText, "\r\n");
    editing = 0;
    while (*cmd == ' ' || *cmd == '\t')
          ++cmd;
  }
  rip (cmd);
  c_flag = 1;
  return (*cmd != 0);
}

/*--------------------------------------------------------------------*/

STATIC void ExitMain (void)
{
  if (argv0_copy)
    free (argv0_copy);
#ifdef FORTIFY
  Fortify_ListAllMemory();
  Fortify_OutputStatistics();
#endif
}

STATIC void InitMain (const char *argv0)
{
#ifdef __HIGHC__
  int    _386debug = 0;  /* 386|Debug doesn't give us correct argv[] */
  FARPTR iv, ex;

  _dx_pmiv_get (13, &iv);
  _dx_excep_get(13, &ex);
  if (FP_SEL(iv) != FP_SEL(ex)) /* separate exc-13 gates ? */
     _386debug = 1;             /* yes, debugger active    */

  InstallExcHandler (NULL);
  if (!_386debug)
#endif
  {
    char *s, *p = strdup (argv0);

    if (p && (s = strrchr (p, SLASH)) != NULL)
    {
      *s = '\0';
      strcpy (progPath, p);
      strcpy (progName, s+1);
    }
    else
    {
      strcpy (progPath, ".");
      strcpy (progName, argv0);
    }
    strlwr (progPath);
    strlwr (progName);
    argv0_copy = p;
  }

#ifdef FORTIFY
  Fortify_EnterScope();
  Fortify_SetOutputFunc (ftp_log_raw);
#endif

  atexit (ExitMain);
}

/*--------------------------------------------------------------------*/

STATIC int OpenURL (void)
{
  if (!initial_url || ftp_open_url(initial_url,initial_port))
     return (1);

  sound (2000);
  delay (20);
  nosound();
  sleep (4);
  CleanUp();
  return (0);
}

