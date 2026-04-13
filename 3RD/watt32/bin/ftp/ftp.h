#ifndef __FTP_H
#define __FTP_H

#define FTP_VERSION      "DOS ftp 1.23"
#define STATIC

#define FTP_DEBUG        0
#define FTP_DATA_PORT    20
#define FTP_CTRL_PORT    21

#define HASH_MARK_SIZE   1024
#define BLK_SIZE         4096
#define LIN_SIZE         8192
#define FTP_BUFSIZ       4096
#define NUM_DATA_SOCKS   4
#define DATACONN_TIME    10
#define DATAWAIT_TIME    20
#define KEEPALIVE_TIME   30
#define MAX_ARGC         20    /* max # of strings in **argv */
#define MAX_HOSTNAME     80
#define MAX_PORTS        400   /* # of listen ports to check */
#define FTP_LOW_PORT     1025  /* range of empherical ports  */
#define FTP_HIGH_PORT    5000  /* normally accepted by FTPD  */

#define RANDPORT(lo,hi) ((lo) + (WORD)(rand() % ((hi)-(lo)+1)))

#if defined(__DJGPP__)
  #include <unistd.h>
  #include <go32.h>
  #include <dpmi.h>
  #include <sys/exceptn.h>
  #include <libc/dosio.h>

  #define CONIO_INIT()  gppconio_init()
  #define SLASH         '/'
  #define _MAX_PATH     128

  #if (DJGPP == 2 && DJGPP_MINOR <= 1)
    #define SET_BIN_MODE() _fmode = O_BINARY
    #define SET_TXT_MODE() _fmode = O_TEXT
  #else
    #define SET_BIN_MODE() ((void)0)
    #define SET_TXT_MODE() ((void)0)
  #endif

#else
  #define CONIO_INIT()     ((void)0)
  #define SET_BIN_MODE()   ((void)0)
  #define SET_TXT_MODE()   ((void)0)
  #define SLASH            '\\'
#endif

#if defined(__HIGHC__)
  #include <conio.h>
  #include <mw/conio.h>  /* extensions for Metaware's conio.h */
  #include <mw/exc.h>    /* exception handler library */
  #include <alloca.h>
  #include <pharlap.h>

  #define FILENO(x)              x
  #define BINARY_MODE            _BINARY
  #define VSPRINTF(buf,fmt,va)   _vbprintf (buf,sizeof(buf),fmt,va)
  #define setmode                _setmode /* doesn't have setmode() */
  #pragma stack_size_warn(8500)

#elif defined(__WATCOMC__)
  #include <conio.h>
  #include <mw/conio.h>          /* conio extensions */

  #define FILENO(x)              fileno(x)
  #define BINARY_MODE            O_BINARY
  #define VSPRINTF(buf,fmt,va)   _vsnprintf (buf,sizeof(buf),fmt,va)

#elif defined(__DMC__)
  #include <conio.h>
  #include <sound.h>

  #define FILENO(x)              fileno(x)
  #define BINARY_MODE            O_BINARY
  #define VSPRINTF(buf,fmt,va)   _vsnprintf (buf,sizeof(buf),fmt,va)

#elif defined(WIN32)
  #include <windows.h>
  #include <mw/conio.h>          /* conio extensions */

  #define FILENO(x)              fileno(x)
  #define BINARY_MODE            O_BINARY
  #define VSPRINTF(buf,fmt,va)   _vsnprintf (buf,sizeof(buf),fmt,va)

#else
  #include <conio.h>
  #define FILENO(x)              fileno(x)
  #define BINARY_MODE            O_BINARY

  #if defined(__DJGPP__) && (DJGPP_MINOR >= 4)
    #define VSPRINTF(buf,fmt,va) vsnprintf (buf,sizeof(buf),fmt,va)
  #else
    #define VSPRINTF(buf,fmt,va) vsprintf (buf,fmt,va)
  #endif
#endif

#if (defined(__SMALL__) && !defined(DOS386)) || defined(__LARGE__)
  #define SIZEOF_SHORT  2
  #define SIZEOF_INT    2
  #define SIZEOF_LONG   4
#else
  #define SIZEOF_SHORT  2
  #define SIZEOF_INT    4
  #define SIZEOF_LONG   4
#endif

#if (FTP_DEBUG)
  #define FTP_LOG(s)   ftp_log s
#else
  #define FTP_LOG(s)   ((void)0)
#endif

#define ARGSUSED(x)    (void)(x)
#define DIM(x)         (sizeof((x)) / sizeof((x)[0]))
#define TIME           struct timeval

#include <malloc.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>
#include <netdb.h>
#include <tcp.h>

#include "fortify.h"
#include "screen.h"
#include "macro.h"
#include "config.h"
#include "cgets.h"
#include "util.h"

#if defined(USE_SRP)
  #include "srp.h"
  #include "radix.h"
  #include "secure.h"
#endif

#if defined(WIN32)
  #define sleep(s)  Sleep((s)*1000)

  #if defined(_MSC_VER) && defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
  #endif
#endif

#if !defined(BOOL)
  #define BOOL  int
#endif

#if !defined(TRUE)
  #define TRUE  1
  #define FALSE (~TRUE)
#endif

extern int     interactive, passive, debug_mode;
extern int     editing, fast_exit, use_parse, c_flag;
extern int     curr_drive, home_drive, is_init;
extern int     sigint_active, sigint_counter;
extern int     script_echo, stdin_mode, in_a_shell;
extern int     prot_level, proxy_flag;
extern FILE   *script_file;
extern char   *initial_url;

extern char    progPath   [_MAX_PATH];
extern char    progName   [_MAX_PATH];
extern char    local_dir  [_MAX_PATH];
extern char    local_home [_MAX_PATH];
extern jmp_buf cmdline;

extern int   (*background)(void);
extern void   *ctrl_socket, *data_socket;
extern char   *reply_parse;

extern int ftp_init    (void);
extern int ftp_shell   (char *cmd);
extern int ftp_open_url(char *str, WORD port);
extern int ftp_open    (int argc, char **argv);
extern int ftp_cwd     (int argc, char **argv);
extern int ftp_lcd     (int argc, char **argv);
extern int ftp_put     (int argc, char **argv);
extern int ftp_mput    (int argc, char **argv);
extern int ftp_append  (int argc, char **argv);
extern int ftp_get     (int argc, char **argv);
extern int ftp_reget   (int argc, char **argv);
extern int ftp_mget    (int argc, char **argv);
extern int ftp_restart (int argc, char **argv);
extern int ftp_rename  (int argc, char **argv);
extern int ftp_delete  (int argc, char **argv);
extern int ftp_mdelete (int argc, char **argv);
extern int ftp_rmdir   (int argc, char **argv);
extern int ftp_mkdir   (int argc, char **argv);
extern int ftp_rhelp   (int argc, char **argv);
extern int ftp_debug   (int argc, char **argv);
extern int ftp_user    (int argc, char **argv);
extern int ftp_passw   (int argc, char **argv);
extern int ftp_acct    (int argc, char **argv);
extern int ftp_quote   (int argc, char **argv);
extern int ftp_site    (int argc, char **argv);
extern int ftp_modtime (int argc, char **argv);
extern int ftp_fsize   (int argc, char **argv);
extern int ftp_newer   (int argc, char **argv);
extern int ftp_rstat   (int argc, char **argv);
extern int ftp_type    (int argc, char **argv);
extern int ftp_list    (int argc, char **argv);
extern int ftp_nlist   (int argc, char **argv);
extern int ftp_mdir    (int argc, char **argv);
extern int ftp_idle    (int argc, char **argv);
extern int ftp_chmod   (int argc, char **argv);
extern int ftp_proxy   (int argc, char **argv);

extern int ftp_close   (void);
extern int ftp_exit    (void);
extern int ftp_fexit   (void);
extern int ftp_cdup    (void);
extern int ftp_pwd     (void);
extern int ftp_isconn  (void);
extern int ftp_isdata  (void);
extern int ftp_hash    (void);
extern int ftp_pasv    (void);
extern int ftp_port    (void);
extern int ftp_binary  (void);
extern int ftp_ascii   (void);
extern int ftp_tenex   (void);
extern int ftp_syst    (void);
extern int ftp_feat    (void);
extern int ftp_status  (void);
extern int ftp_cls     (void);
extern int ftp_bell    (void);
extern int ftp_beep    (void);
extern int ftp_verb    (void);
extern int ftp_prompt  (void);
extern int ftp_glob    (void);
extern int ftp_sunique (void);
extern int ftp_runique (void);
extern int ftp_struct  (void);
extern int ftp_form    (void);
extern int ftp_ftmode  (void);
extern int ftp_trace   (void);
extern int ftp_reset   (void);

extern void int29_init (void);
extern void int29_exit (void);
extern void DataAbort  (void);
extern void DoKeepAlive(void);
extern void DoCommand  (char *cmd, int argc, char **argv);
extern void ProxyHelp  (void);
extern void CloseTempF (void);
extern void TranslateCmdTab (void);
extern void CloseWattFiles  (void);
extern void ReopenWattFiles (void);

extern int Command (const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;

#ifdef USE_SRP  /* in srp_cmd.c */
  extern char *ftp_getlevel (void);
  extern int   ftp_setlevel (int argc, char **argv);
  extern int   ftp_setclear (void);
  extern int   ftp_setsafe  (void);
  extern int   ftp_setpriv  (void);
#endif

#endif
