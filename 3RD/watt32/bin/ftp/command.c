#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ftp.h"

static int Help (int argc, char **argv);

#define HELP(name,str) static const char name##_hlp[] = str;

HELP (account, "send account command to remote server");
HELP (append,  "append to a remote file");
HELP (authent, "authenticates client before login");
HELP (ascii,   "set ascii transfer type");
HELP (bell,    "beep when command completed");
HELP (binary,  "set binary transfer type");
HELP (case,    "toggle mget upper/lower case id mapping");
HELP (cd,      "change remote working directory");
HELP (cdup,    "change remote working directory to parent directory");
HELP (chmod,   "change file permissions of remote file; chmod mode file");
HELP (connect, "connect to remote tftp");
HELP (cr,      "toggle carriage return stripping on ascii gets");
HELP (delete,  "delete remote file");
HELP (debug,   "toggle/set debugging mode");
HELP (dir,     "list contents of remote directory");
HELP (close,   "terminate ftp session");
HELP (domac,   "execute macro");
HELP (form,    "set file transfer format");
HELP (glob,    "toggle metacharacter expansion of local file names");
HELP (hash,    "toggle printing `#' for each buffer transferred");
HELP (help,    "print local help information");
HELP (idle,    "get (set) idle timer on remote side");
HELP (lcd,     "change local working directory");
HELP (ls,      "list contents of remote directory");
HELP (macdef,  "define a macro");
HELP (mdelete, "delete multiple files");
HELP (mdir,    "list contents of multiple remote directories");
HELP (mget,    "get multiple files");
HELP (mkdir,   "make directory on the remote machine");
HELP (mls,     "list contents of multiple remote directories");
HELP (modtime, "show last modification time of remote file");
HELP (mode,    "set file transfer mode");
HELP (mput,    "send multiple files");
HELP (newer,   "get file if remote file is newer than local file ");
HELP (nlist,   "nlist contents of remote directory");
HELP (nmap,    "set templates for default file name mapping");
HELP (ntrans,  "set translation table for default file name mapping");
HELP (passive, "enter passive transfer mode");
HELP (port,    "toggle use of PORT cmd for each data connection");
HELP (prompt,  "force interactive prompting on multiple commands");
HELP (proxy,   "issue command on alternate connection");
HELP (pwd,     "print working directory on remote machine");
HELP (quit,    "terminate ftp session and exit");
HELP (fquit,   "exit to DOS without waiting to close session");
HELP (quote,   "send arbitrary ftp command");
HELP (receive, "receive file");
HELP (reget,   "get file restarting at end of local file");
HELP (remote,  "get help from remote server");
HELP (rename,  "rename file");
HELP (restart, "restart file transfer at bytecount");
HELP (rmdir,   "remove directory on the remote machine");
HELP (rstatus, "show status of remote machine");
HELP (runique, "toggle store unique for local files");
HELP (reset,   "clear queued command replies");
HELP (send,    "send one file");
HELP (site,    "send site specific command to remote server\r\n"\
               "Try \"rhelp site\" or \"site help\" for more information");
HELP (shell,   "escape to the shell");
HELP (size,    "show size of remote file");
HELP (status,  "show current status");
HELP (struct,  "set file transfer structure");
HELP (sunique, "toggle store unique on remote machine");
HELP (system,  "show remote system type");
HELP (tenex,   "set tenex file transfer type");
HELP (tick,    "toggle printing byte counter during transfers");
HELP (trace,   "toggle packet tracing");
HELP (type,    "set file transfer type");
HELP (umask,   "get (set) umask on remote side");
HELP (user,    "send new user information");
HELP (verbose, "toggle verbose mode");
HELP (passw,   "send password information");
HELP (cls,     "clears the screen");
HELP (sock,    "dump socket data for ctrl/data connections");

#ifdef USE_SRP
HELP (level,   "set protection level for file transfer");
HELP (private, "set private protection level for file transfer");
HELP (clear,   "set clear protection level for file transfer");
HELP (safe,    "set safe protection level for file transfer");
#endif

struct CommandTab {
       const char *name;        /* command name                      */
       int         unique_len;  /* minimum length for uniqueness     */
       int       (*handler)();  /* command handler gets agc-1,argv+1 */
       int         req_argc;    /* minimum # of arguments required   */
       int         req_conn;    /* online-connection required        */
       int         proxy;       /* allowed as a proxy command        */
       int         beep;        /* beep after command completes      */
       const char *help;        /* help text for command             */
     };


static struct CommandTab commands[] = {
      /* name                ulen  handler   argc conn proxy beep help     */
      /* -------------------------------------------------------------------*/
       { "!",                  1,  ftp_shell,   0,  0,  0,   0,   shell_hlp   },
       { __LANG("debug"),      5,  ftp_debug,   0,  0,  0,   0,   debug_hlp   },
       { __LANG("mget"),       4,  ftp_mget,    1,  1,  1,   1,   mget_hlp    },
       { __LANG("pwd"),        3,  ftp_pwd,     0,  1,  1,   0,   pwd_hlp     },
       { __LANG("status"),     4,  ftp_status,  0,  0,  1,   0,   status_hlp  },
       { "$",                  1,  ftp_macro,   0,  0,  0,   1,   domac_hlp   },
       { __LANG("dir"),        3,  ftp_list,    0,  1,  1,   1,   dir_hlp     },
       { __LANG("mkdir"),      5,  ftp_mkdir,   1,  1,  1,   1,   mkdir_hlp   },
       { __LANG("quit"),       4,  ftp_fexit,   0,  0,  0,   0,   fquit_hlp   },
       { __LANG("struct"),     6,  ftp_struct,  1,  0,  1,   0,   struct_hlp  },
       { "?",                  1,  Help,        0,  0,  0,   0,   help_hlp    },
       { __LANG("disconnect"), 5,  ftp_close,   0,  1,  1,   0,   close_hlp   },
       { __LANG("mls"),        3,  NULL,        1,  1,  1,   1,   mls_hlp     },
       { __LANG("quote"),      5,  ftp_quote,   1,  1,  1,   1,   quote_hlp   },
       { __LANG("system"),     4,  ftp_syst,    0,  1,  1,   0,   system_hlp  },
       { __LANG("account"),    7,  ftp_acct,    1,  1,  1,   0,   account_hlp },
       { __LANG("form"),       4,  ftp_form,    1,  0,  1,   0,   form_hlp    },
       { __LANG("mode"),       4,  ftp_ftmode,  0,  0,  1,   0,   mode_hlp    },
       { __LANG("recv"),       4,  ftp_get,     1,  1,  1,   1,   receive_hlp },
       { __LANG("sunique"),    7,  ftp_sunique, 0,  0,  1,   0,   sunique_hlp },
       { __LANG("authent"),    7,  NULL,        1,  1,  0,   0,   authent_hlp },
       { __LANG("get"),        3,  ftp_get,     1,  1,  1,   1,   receive_hlp },
       { __LANG("modtime"),    7,  ftp_modtime, 1,  1,  1,   0,   modtime_hlp },
       { __LANG("reget"),      5,  ftp_reget,   1,  1,  1,   1,   reget_hlp   },
       { __LANG("tenex"),      5,  ftp_tenex,   0,  1,  1,   0,   tenex_hlp   },
       { __LANG("append"),     6,  ftp_append,  1,  1,  1,   1,   append_hlp  },
       { __LANG("tick"),       4,  NULL,        0,  0,  0,   0,   tick_hlp    },
       { __LANG("glob"),       4,  ftp_glob,    0,  1,  0,   0,   glob_hlp    },
       { __LANG("mput"),       4,  ftp_mput,    1,  1,  1,   1,   mput_hlp    },
       { __LANG("ascii"),      3,  ftp_ascii,   0,  1,  1,   0,   ascii_hlp   },
       { __LANG("rstatus"),    5,  ftp_rstat,   0,  1,  1,   0,   rstatus_hlp },
       { __LANG("binary"),     3,  ftp_binary,  0,  1,  1,   0,   binary_hlp  },
       { __LANG("trace"),      5,  ftp_trace,   0,  0,  0,   0,   trace_hlp   },
       { __LANG("hash"),       4,  ftp_hash,    0,  0,  0,   0,   hash_hlp    },
       { __LANG("bell"),       4,  ftp_bell,    0,  0,  0,   0,   bell_hlp    },
       { __LANG("newer"),      5,  ftp_newer,   1,  1,  1,   1,   newer_hlp   },
       { __LANG("rhelp"),      5,  ftp_rhelp,   0,  1,  1,   0,   remote_hlp  },
       { __LANG("bye"),        3,  ftp_exit,    0,  0,  0,   0,   quit_hlp    },
       { __LANG("case"),       4,  NULL,        0,  1,  1,   0,   case_hlp    },
       { __LANG("type"),       4,  ftp_type,    0,  1,  1,   0,   type_hlp    },
       { __LANG("help"),       4,  Help,        0,  0,  1,   0,   help_hlp    },
       { __LANG("nmap"),       4,  NULL,        1,  1,  1,   0,   nmap_hlp    },
       { __LANG("rename"),     3,  ftp_rename,  1,  1,  1,   0,   rename_hlp  },
       { __LANG("cdup"),       4,  ftp_cdup,    0,  1,  1,   0,   cdup_hlp    },
       { __LANG("user"),       4,  ftp_user,    1,  1,  1,   0,   user_hlp    },
       { __LANG("idle"),       4,  ftp_idle,    0,  1,  1,   0,   idle_hlp    },
       { __LANG("nlist"),      5,  ftp_nlist,   0,  1,  1,   1,   nlist_hlp   },
       { __LANG("reset"),      5,  ftp_reset,   0,  1,  1,   0,   reset_hlp   },
       { __LANG("cd"),         2,  ftp_cwd,     1,  1,  1,   0,   cd_hlp      },
       { __LANG("umask"),      5,  NULL,        1,  1,  1,   0,   umask_hlp   },
       { __LANG("image"),      5,  ftp_binary,  0,  1,  1,   0,   binary_hlp  },
       { __LANG("ntrans"),     6,  NULL,        1,  1,  1,   0,   ntrans_hlp  },
       { __LANG("restart"),    7,  ftp_restart, 1,  1,  1,   1,   restart_hlp },
       { __LANG("chmod"),      5,  ftp_chmod,   2,  1,  1,   0,   chmod_hlp   },
       { __LANG("verbose"),    4,  ftp_verb,    0,  0,  0,   0,   verbose_hlp },
       { __LANG("lcd"),        3,  ftp_lcd,     1,  0,  0,   0,   lcd_hlp     },
       { __LANG("open"),       4,  ftp_open,    1,  0,  1,   0,   connect_hlp },
       { __LANG("rmdir"),      5,  ftp_rmdir,   1,  1,  1,   0,   rmdir_hlp   },
       { __LANG("close"),      5,  ftp_close,   0,  1,  1,   0,   close_hlp   },
       { __LANG("ls"),         2,  ftp_nlist,   0,  1,  1,   1,   ls_hlp      },
       { __LANG("prompt"),     6,  ftp_prompt,  0,  0,  0,   0,   prompt_hlp  },
       { __LANG("runique"),    7,  ftp_runique, 0,  0,  1,   0,   runique_hlp },
       { __LANG("macdef"),     6,  NULL,        1,  1,  0,   0,   macdef_hlp  },
       { __LANG("cr"),         2,  NULL,        0,  0,  0,   0,   cr_hlp      },
       { __LANG("proxy"),      5,  ftp_proxy,   1,  0,  1,   0,   proxy_hlp   },
       { __LANG("send"),       4,  ftp_put,     1,  1,  1,   1,   send_hlp    },
       { __LANG("mdelete"),    4,  ftp_mdelete, 1,  1,  1,   1,   mdelete_hlp },
       { __LANG("sendport"),   8,  ftp_port,    0,  1,  0,   0,   port_hlp    },
       { __LANG("cls"),        3,  ftp_cls,     0,  0,  0,   0,   cls_hlp     },
       { __LANG("site"),       4,  ftp_site,    1,  1,  1,   0,   site_hlp    },
       { __LANG("sock"),       4,  sock_dump,   0,  0,  0,   0,   sock_hlp    },
       { __LANG("delete"),     3,  ftp_delete,  1,  1,  1,   0,   delete_hlp  },
       { __LANG("mdir"),       4,  ftp_mdir,    1,  1,  1,   1,   mdir_hlp    },
       { __LANG("put"),        3,  ftp_put,     1,  1,  1,   1,   send_hlp    },
       { __LANG("size"),       4,  ftp_fsize,   1,  1,  1,   1,   size_hlp    },
       { __LANG("pasv"),       5,  ftp_pasv,    0,  1,  0,   0,   passive_hlp },
       { __LANG("pass"),       4,  ftp_passw,   1,  1,  0,   0,   passw_hlp   },
       { __LANG("wait~"),      4,  wait_until,  1,  0,  0,   1,   NULL        },
       { __LANG("echo~"),      4,  echo_cmd,    1,  0,  0,   1,   NULL        },
       { __LANG("stop~"),      4,  StopScript,  0,  0,  0,   0,   NULL        },
#ifdef USE_SRP
       { __LANG("protect"),    4,  ftp_setlevel,0,  1,  1,   0,   level_hlp   },
       { __LANG("private"),    4,  ftp_setpriv, 0,  1,  1,   0,   private_hlp },
       { __LANG("clear"),      5,  ftp_setclear,0,  1,  1,   0,   clear_hlp   },
       { __LANG("safe"),       4,  ftp_setsafe, 0,  1,  1,   0,   safe_hlp    },

#endif
       { NULL, 0, NULL, 0, 0, 0, 0, NULL }
     };

/*
 * Commands are sorted so that they'll be printed this way:
 *
 *  !           cr            mdir         put           size
 *  $           debug         mget         pwd           sock
 *  ?           delete        mkdir        quit          status
 *  account     dir           mls          quote         struct
 *  append      disconnect    mode         recv          sunique
 *  ascii       form          modtime      reget         system
 *  authent     get           mput         rename        tenex
 *  bell        glob          newer        reset         tick
 *  binary      hash          nlist        restart       trace
 *  bye         help          nmap         rhelp         type
 *  case        idle          ntrans       rmdir         umask
 *  cd          image         open         rstatus       user
 *  cdup        lcd           pass         runique       verbose
 *  chmod       ls            pasv         send
 *  close       macdef        prompt       sendport
 *  cls         mdelete       proxy        site
 */

#ifdef __TURBOC__
#pragma warn -prot
#endif

void DoCommand (char *cmd, int argc, char **argv)
{
  struct CommandTab *c = commands;
  char  *argv0         = argv[0];
  int    rc;

  if (argc < 1 || !argv0)
  {
    xprintf (WarnText, _LANG("Empty command!!??\r\n"));
    return;
  }
  script_echo = 1;
  if (*argv0 == '@')
  {
    argv[0] = ++argv0;
    script_echo = 0;
  }
  if (strlen(argv[0]) == 0)
     return;

  if (cfg.verbose && script_file && script_echo)
  {
    ShowPrompt();
    xprintf (UserText, "%s\r\n", cmd);
  }

  while (c->name)
  {
    /* !! to-do: resolve ambigious commands ("send" vs "sendport")
     */
    if (!strnicmp(argv[0],c->name,c->unique_len))
    {
      if (c->name[c->unique_len] == '~' && !script_file)
         break;

      if (!c->handler)
      {
        WARN1 (_LANG("`%s' not implemented\r\n"), c->name);
        return;
      }
      if (c->req_conn && !ftp_isconn())
      {
        WARN (_LANG("Not connected\r\n"));
        CloseScript (1);
        return;
      }
      if (argc-1 < c->req_argc)
      {
        WARN (_LANG("Missing argument\r\n"));
        if (c->help)
           xprintf (CtrlText, _LANG("Usage: %s\r\n"), c->help);
        CloseScript (1);
        return;
      }

      switch (cmd[0])
      {
        case '!': rc = (*c->handler) (cmd+1);
                  break;
        case '$': rc = (*c->handler) (argc, argv);
                  break;
        default:  rc = (*c->handler) (argc-1, argv+1);
                  break;
      }
      if (c->beep && cfg.bell_mode)
         ftp_beep();

      if (!rc && script_file)
         CloseScript (1);
      return;
    }
    c++;
  }
  WARN (_LANG("?Invalid command\r\n"));
}

/*
 * Fix the commands[] table to suit current language.
 */
void TranslateCmdTab (void)
{
  struct CommandTab *c = commands;

  while (c->name)
  {
    const char *name = _LANG (c->name);

    if (name != c->name && c->name[c->unique_len] != '~')
    {
      c->name       = name;
      c->unique_len = strlen (name);
    }
    if (c->help)
        c->help = _LANG (c->help);
    c++;
  }
}

/*
 * Show help for proxy commands
 */
void ProxyHelp (void)
{
  const struct CommandTab *c = commands;

  xprintf (CtrlText, _LANG("legal proxy commands:\r\n"));
  while (c->name)
  {
    if (c->proxy)
       xprintf (CtrlText, "%-16s", c->name);
    c++;
  }
  xputs (CtrlText, "\r\n");
}

/*
 * Show help for specific command or list all commands.
 */
static int Help (int argc, char **argv)
{
  const struct CommandTab *cmd = commands;

  if (argc == 0)       /* show all commands */
  {
    int num_unimpl = 0;
    int column = 0;

    while (cmd->name)
    {
      if (cmd->name[cmd->unique_len] != '~')  /* don't show script commands */
      {
        if (cmd->handler)
           xprintf (CtrlText, "%-14s", cmd->name);
        else
        {
          xprintf (CtrlText, "%s *%-*s", cmd->name, 12-(int)strlen(cmd->name), "");
          num_unimpl++;
        }
      }
      cmd++;
      if (++column == 5)
      {
        xputs (CtrlText, "\r\n");
        column = 0;
      }
    }
    if (num_unimpl)
       xprintf (CtrlText, _LANG("(* = not implemented)\r\n"));
    return (1);
  }

  while (cmd->name)
  {
    if (!strnicmp(argv[0],cmd->name,cmd->unique_len))
    {
      xprintf (CtrlText, "%s %s\r\n",
               cmd->help    ? cmd->help : "<no help>",
               cmd->handler ? ""        : _LANG("(Not implemented)"));
      return (1);
    }
    cmd++;
  }
  WARN (_LANG("No such command\r\n"));
  return (0);
}

