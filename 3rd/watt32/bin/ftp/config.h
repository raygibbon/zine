#ifndef __CONFIG_H
#define __CONFIG_H

struct Colour {
       int user_fg, user_bg;
       int data_fg, data_bg;
       int ctrl_fg, ctrl_bg;
       int warn_fg, warn_bg;
       int stat_fg, stat_bg;
       int dos_fg,  dos_bg;
     };

struct LoginData {
       int    def;
       char   *host;
       char   *user;
       char   *pass;
       char   *acct;
       struct LoginData *next;
     };

struct FtpConfig  {
       struct Colour     colour;
       struct LoginData *login;
       struct MacroData *macro;
       char  *log_file;
       int    scr_width;
       int    scr_height;
       int    status_line;
       int    data_port;
       int    ctrl_port;
       int    dataconn_to;
       int    datawait_to;
       int    keepalive_to;
       int    send_socksize;
       int    recv_socksize;
       int    do_ping;
       int    bell_mode;
       int    auto_login;
       int    allow_syst;
       int    initial_syst;
       int    hash_marks;
       int    verbose;
       int    use_comspec;
       int    do_ansi_test;
       int    tracing;
       char  *auth_type;
     };

struct URL {
       char  proto[32];
       char  user [128];
       char  pass [128];
       char  acct [128];
       char  host [256];
       char  path [256];
       char  file [256];
       int   port;
       int   type;
     };

extern struct FtpConfig cfg;

extern int    ParseURL    (const char *url, struct URL *res);
extern int    OpenIniFile (const char *name);
extern int    GetUserPass (struct URL *url);
extern char **MakeArgv    (char *str, int *argc);

extern int ftp_log (const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)));
#endif
;

extern int   ftp_log_init    (void);
extern void  ftp_log_exit    (void);
extern void  ftp_log_raw     (const char *str);
extern void  ftp_log_flush   (void);
extern void  ftp_log_pause   (void);
extern void  ftp_log_continue(void);
extern void  ftp_lang_init   (const char *value);

extern const char *lang_xlat    (const char *s);
extern const char *_w32_expand_var_str (const char *str);

/*
 * Strings with _LANG() around them are found by automatic tools and put
 * into the special text file to be translated into foreign languages.
 *
 * __LANG() must be used for array string constants. This macro is used by
 * the automatic tool to generate an entry for it in the language database.
 */

#define _LANG(x)  lang_xlat(x)
#define __LANG(x) x

#endif

