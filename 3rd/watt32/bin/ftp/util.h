#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <time.h>
#include <assert.h>

#ifndef NDEBUG
#undef  assert
#define assert(x) ((x) ? (void)0 : assert_fail (#x, __FILE__, __LINE__))
#endif

extern void   assert_fail  (const void *what, const void *file, unsigned line);
extern void   setup_sigint (void);
extern void   setup_sigsegv(void);
extern void   sig_handler  (int sig);

extern char  *RamFree   (void);
extern char  *DiskFree  (void);
extern void   CleanUp   (void);
extern int    wait_until(int argc, char **argv);
extern int    echo_cmd  (int argc, char **argv);

extern void   CloseScript (int fail);
extern int    StopScript  (void);
extern int    sock_dump   (void);
extern FILE  *_OpenFile   (const char **name, const char *mode, int *overwrite);
extern FILE  *AppendFile  (const char **name, DWORD *pos);
extern void   UpdateTime  (time_t start);
extern void   ShowPrompt  (void);
extern char  *GetPass     (const char *prompt);
extern void   SetIntensity(int on);

extern char       *_strncpy  (char *dest, const char *src, size_t len);
extern struct tm  *FileTime  (const char *file);
extern const char *OnOff     (int val);
extern const char *OkComplete(int rc);
extern const char *OkFailed  (int rc);

extern int UserPrompt (char *buf, int size, const char *prompt, ...)
#ifdef __GNUC__
__attribute__((format(printf,3,4)))
#endif
;

extern int YesNoPrompt (const char *prompt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;

#endif
