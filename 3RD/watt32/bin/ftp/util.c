#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <time.h>
#include <io.h>
#include <signal.h>
#include <malloc.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/ftp.h>

#include "ftp.h"

static FILE       *current_file = NULL;
static const char *DosName (const char *name);

static int DumpPrintf (const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;


/*
 * Our assert() fail replacement routine
 */
void assert_fail (const void *what, const void *file, unsigned line)
{
  char buf[512], *p = buf;

  p += sprintf (p, "%s (%u): ", (char*)file, line);

  sprintf (p, _LANG("Assertion `%s' failed!"), (const char*)what);
  ftp_log (buf);
  xprintf (CtrlText, _LANG("Fatal: %s\r\n"), buf);
  ftp_fexit();   /* fast exit, don't QUIT */
}

/*
 * Return available memory on heap
 */
#if defined(__HIGHC__)
static int heap_avail (DWORD *mem)
{
  _HEAPINFO h_info;
  int       h_status;

  h_info._pentry = NULL;
  *mem = 0L;

  while (1)
  {
    h_status = _heapwalk (&h_info);
    if (h_status != _HEAPOK)
       break;
    if (h_info._useflag != _USEDENTRY)
       *mem += h_info._size;
  }
  if (h_status < 0)
     return (0);
  return (1);
}

#elif defined(__TURBOC__)
static int heap_avail (DWORD *mem)
{
  struct farheapinfo hi;
  DWORD  rc = 0;

  if (farheapcheck() <= 0)  /* Check for corrupted heap */
     return (0);

  hi.ptr = NULL;
  while (farheapwalk(&hi) == _HEAPOK)
        if (hi.in_use == 0 )
           rc += hi.size;

  rc += farcoreleft();
  *mem = rc;
  return (1);
}
#endif

#define _1MB_ (1024UL*1024UL)

#if defined(__LARGE__)
  #define MB_FMT "%.3lfMB"
#else
  #define MB_FMT "%.3fMB"
#endif

#if defined(WIN32)
#if !defined(_MSC_VER)
typedef struct  {
        DWORD     dwLength;
        DWORD     dwMemoryLoad;
        DWORDLONG ullTotalPhys;
        DWORDLONG ullAvailPhys;
        DWORDLONG ullTotalPageFile;
        DWORDLONG ullAvailPageFile;
        DWORDLONG ullTotalVirtual;
        DWORDLONG ullAvailVirtual;
        DWORDLONG ullAvailExtendedVirtual;
      } MEMORYSTATUSEX;
#endif

typedef BOOL (WINAPI *PFN_MS_EX) (MEMORYSTATUSEX *);

static DWORD physmem_avail (void)
{
  PFN_MS_EX      pfnex;
  MEMORYSTATUS   ms;
  MEMORYSTATUSEX ms_ex;
  HMODULE        hnd = GetModuleHandle ("kernel32.dll");

  /* Use GlobalMemoryStatusEx if available.  */
  pfnex = hnd ? (PFN_MS_EX) GetProcAddress (hnd, "GlobalMemoryStatusEx") : NULL;
  if (pfnex)
  {
    ms_ex.dwLength = sizeof(ms_ex);
    if (!(*pfnex)(&ms_ex))
       return (0UL);
    return (ms_ex.ullAvailPhys);
  }

  /* Fall back to GlobalMemoryStatus which is always available.
   * but returns wrong results for physical memory > 4GB
   */
  GlobalMemoryStatus (&ms);
  return (ms.dwAvailPhys);
}
#endif

/*
 * Return available physical memory
 */
char *RamFree (void)
{
  static char buf [100];
  DWORD  free = 0;

#if defined(WIN32)
  free = physmem_avail();

#elif defined(__HIGHC__)
  VM_STATS mem;
  _dx_vm_stats (&mem, 0);
  free = 4096 * mem.vm_maxblk;

#elif defined(__DJGPP__)
  __dpmi_free_mem_info mem;
  __dpmi_get_free_memory_information (&mem);
  free = 4096 * mem.total_number_of_free_pages;

#elif defined(__WATCOMC__)
  _nheapgrow();
  free = _memavl();
#endif

  if (free <= _1MB_)
       sprintf (buf, "%lukB", free >> 10);
  else sprintf (buf, MB_FMT, (double)free/_1MB_);

#if defined(__HIGHC__) || defined(__TURBOC__)
  {
    DWORD heap_free = 0L;
    char  hfree[30];

    if (!heap_avail (&heap_free))
       return ("Heap corrupt");

    if (heap_free <= _1MB_)
         sprintf (hfree, ". Heap free: %lukB", heap_free >> 10);
    else sprintf (hfree, ". Heap free: " MB_FMT "", (double)heap_free/_1MB_);
    strcat (buf, hfree);
  }
#endif

  return (buf);
}

/*
 * Return available space on current drive
 */
char *DiskFree (void)
{
  struct diskfree_t dfree;
  DWORD  free;
  static char buf[30];

#ifdef WIN32
  _getdiskfree (curr_drive, &dfree);
#else
  _dos_getdiskfree (curr_drive, &dfree);
#endif

  free = ((DWORD)dfree.avail_clusters      *
          (DWORD)dfree.sectors_per_cluster *
          (DWORD)dfree.bytes_per_sector);

  if (free <= _1MB_)
       sprintf (buf, "%lukB", free >> 10);
  else sprintf (buf, MB_FMT, (double)free/_1MB_);
  return (buf);
}

/*
 * Perform cleanup actions; close sockets and tcp/ip stack, chdir to
 * original drive/directory etc.
 */
void CleanUp (void)
{
#ifdef MSDOS
  unsigned drives;
#endif

  ftp_close();
  tcp_tick (NULL);  /* flush input queues */

  sock_exit();
  chdir (local_home);
  ScreenExit();

#ifdef MSDOS
  _dos_setdrive (home_drive, &drives);
  int29_exit();
#endif
  ftp_log_exit();
  setmode (FILENO(stdin), stdin_mode);
  CloseTempF();
}

/*
 * To prevent running out of file-handles, we should close the
 * Watt-32 'hosts', `services', `protocol' and `networks' files before
 * spawning a new shell.
 */
void CloseWattFiles (void)
{
#if !defined(__LARGE__) /* Assume large mode was built w/o USE_BSD_API */
  extern void _w32_CloseHostFile(void), _w32_CloseServFile(void);
  extern void _w32_CloseProtoFile(void), _w32_CloseNetworksFile(void);
#if defined(__DJGPP__) && 0
  extern void MS_CDECL _w32_CloseHost6File (void);
  _w32_CloseHost6File();
#endif
  _w32_CloseHostFile();
  _w32_CloseServFile();
  _w32_CloseProtoFile();
  _w32_CloseNetworksFile();
#endif
}

void ReopenWattFiles (void)
{
#if !defined(__LARGE__) && !defined(WIN32) /* Assumes large mode was built w/o USE_BSD_API */
  extern void ReopenHostFile(void), ReopenServFile(void);
  extern void ReopenProtoFile(void), ReopenNetworksFile(void);
#if defined(__DJGPP__) && 0
  extern void ReopenHost6File (void);
  ReopenHost6File();
#endif
  ReopenHostFile();
  ReopenServFile();
  ReopenProtoFile();
  ReopenNetworksFile();
#endif
}

/*
 * Close any open script file (unless from stdin)
 */
void CloseScript (int fail)
{
  if (script_file)
  {
    if (cfg.verbose)
       xprintf (UserText, _LANG("Script %s\r\n"),
                fail ? _LANG("aborted") : _LANG("done"));
    if (script_file != stdin)
       fclose (script_file);
    script_file = NULL;
  }
}

/*
 * Stop and close any script running
 */
int StopScript (void)
{
  CloseScript (1);
  return (1);
}

/*
 * Command handler for "wait"
 */
int wait_until (int argc, char **argv)
{
  struct tm tm;
  time_t    timeout;
  time_t    now = time (NULL);
  unsigned  num = 3;
  unsigned  add_hour = 0, add_min = 0, add_sec = 0;

  if (argv[0][0] == '+')
  {
    char *add = argv[0]+1;

    if (sscanf(add,"%u:%u:%u",&add_hour,&add_min,&add_sec) == 3)
    {
      now += (add_sec + 60 * add_min + 3600 * add_hour);
      memcpy (&tm, localtime(&now), sizeof(tm));
    }
    else if (sscanf(add,"%u:%u",&add_min,&add_sec) == 2)
    {
      now += (add_sec + 60 * add_min);
      memcpy (&tm, localtime(&now), sizeof(tm));
    }
    else if (sscanf(add,"%u",&add_sec) == 1)
    {
      now += add_sec;
      memcpy (&tm, localtime(&now), sizeof(tm));
    }
    else
      num = 0;
  }
  else
  {
    memcpy (&tm, localtime(&now), sizeof(tm));
    num = sscanf (argv[0], "%2u:%2u:%2u",
                  &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
  }

  if ((num < 2) ||
      (tm.tm_hour > 23 || tm.tm_hour < 0) ||
      (tm.tm_min  > 59 || tm.tm_min  < 0) ||
      (tm.tm_sec  > 59 || tm.tm_sec  < 0) ||
      ((timeout = mktime(&tm)) == (time_t)-1))
  {
    WARN1 ("Illegal time-format `%s'\r\n", argv[0]);
    return (0);
  }

  ftp_log ("Waiting until `%.24s'..", asctime(&tm));

  while (1)
  {
#ifdef WIN32
    Sleep (1);
#endif
    if (kbhit() && getch() == '\x1B')
       break;

    if (time(NULL) >= timeout)
       break;

    if (background)
      (*background)();

    tcp_tick (NULL);
  }
  ARGSUSED (argc);
  return (1);
}

/*
 * Command handler for "echo"
 */
int echo_cmd (int argc, char **argv)
{
  int i;

  for (i = 0; i < argc; i++)
  {
    char *str = argv[i];

    if (*str == '\\')
    {
      switch (*(str+1))
      {
        case 'a':
             ftp_beep();
             str += 2;
             break;

        case 'f':
             ftp_cls();
             str += 2;
             break;

        case 'n':
             xputs (UserText, "\r\n");
             str += 2;
             break;
      }
      if (*str == '\0')
         continue;
    }
    xprintf (UserText, "%s ", str);
  }
  xputs (UserText, "\r\n");
  return (1);
}

/*
 * Print socket debug info for ctrl- and data sockets
 */
static char *indent = NULL;

static int DumpPrintf (const char *fmt, ...)
{
  va_list args;
  char    buf [LIN_SIZE];
  int     len;

  va_start (args, fmt);
  len = VSPRINTF (buf, fmt, args);

  SetColour (CtrlText);
  if (indent)
     cputs (indent);
  cputs (buf);
  indent = (buf[len-1] == '\n') ? "             " : NULL;
  va_end (args);
  return (len);
}

int sock_dump (void)
{
  int  (*curr_printf)(const char*, ...) = _printf;
  struct watt_sockaddr sa;
  char   ip[20];

  _printf = DumpPrintf;

  xputs (CtrlText, "Ctrl-socket: ");
  if (ctrl_socket)
  {
    sock_debugdump (ctrl_socket);
    indent = NULL;
    memset (&sa, 0, sizeof(sa));
    _getpeername ((tcp_Socket*)ctrl_socket, (void*)&sa, NULL);
    xprintf (CtrlText, "             %s (%d)\r\n",
             _inet_ntoa(ip, sa.s_ip), sa.s_port);
  }
  else
    WARN ("NONE\r\n");

  xputs (CtrlText, "Data-socket: ");
  if (data_socket)
  {
    sock_debugdump (data_socket);
    indent = NULL;
    memset (&sa, 0, sizeof(sa));
    _getpeername ((tcp_Socket*)data_socket, (void*)&sa, NULL);
    xprintf (CtrlText, "             %s (%d)\r\n",
             _inet_ntoa(ip, sa.s_ip), sa.s_port);
  }
  else
    WARN ("NONE\r\n");

  _printf = curr_printf;  /* restore func-pointers */
  return (1);
}

/*
 * Open a local file in specified read/write mode ("rb" or "wb")
 * Checks for truncated name and illegal attributes. Prompt before
 * overwriting non-zero length files for writing.
 */
FILE *_OpenFile (const char **name, const char *mode, int *overwrite)
{
  const char *fn = DosName (*name);
  int         write = mode[0] == 'w';
  FILE       *fil;

  current_file = NULL;

#if defined(MSDOS)
  if (strcmp(fn,*name))
  {
    WARN1 (_LANG("Local name truncated to `%s'\r\n"), fn);
    *name = fn;
  }

  {
    struct find_t fi;
    unsigned attr  = (_A_NORMAL | _A_ARCH | _A_RDONLY | _A_HIDDEN | _A_SYSTEM);

    if (write && !_dos_findfirst(fn,attr,&fi))
    {
      if (fi.attrib & (_A_RDONLY|_A_HIDDEN|_A_SYSTEM))
      {
        WARN1 (_LANG("Cannot create `%s'. Check file attributes\r\n"), fn);
        return (NULL);
      }
      if (fi.size > 0L && *overwrite == 0)
      {
        if (!YesNoPrompt(_LANG("File `%s' exists. Overwrite (y/n)? :"), fn))
           return (NULL);
        *overwrite = 1;
      }
    }
  }
#endif

  fil = fopen (fn, mode);
  if (!fil)
  {
    if (write)
         WARN1 (_LANG("Cannot open local file `%s'\r\n"), fn);
    else WARN1 (_LANG("Local file `%s' not found\r\n"), fn);
  }
  current_file = fil;
  return (fil);
}

/*
 * Open local file for appending (used by "reget" command).
 * Checks for truncated name and illegal attributes.
 * Return file-size in '*pos'.
 */
FILE *AppendFile (const char **name, DWORD *pos)
{
  const char *fn   = DosName (*name);
  FILE       *fil;

  current_file = NULL;

  if (strcmp(fn,*name))
  {
    WARN1 (_LANG("Local name truncated to `%s'\r\n"), fn);
    *name = fn;
  }

#if defined(MSDOS)
  {
    struct find_t fi;
    unsigned attr = (_A_NORMAL | _A_ARCH | _A_RDONLY | _A_HIDDEN | _A_SYSTEM);

    if (_dos_findfirst(fn,attr,&fi))
    {
      WARN1 (_LANG("`%s' not found\r\n"), fn);
      return (NULL);
    }

    if (fi.attrib & (_A_RDONLY|_A_HIDDEN|_A_SYSTEM))
    {
      WARN1 (_LANG("Cannot append to `%s'. Check file attributes\r\n"), fn);
      return (NULL);
    }
  }
#endif

  fil = fopen (fn, "ab");
  if (!fil)
  {
    WARN1 (_LANG("Cannot open local file `%s'\r\n"), fn);
    return (NULL);
  }
  current_file = fil;
  *pos = filelength (fileno(fil));
  return (fil);
}

/*
 * Remaps some long filenames to DOS 8.3 filenames
 */
static const char *DosName (const char *name)
{
#if defined(MSDOS)
  unsigned i;
  char     tmp      [_MAX_PATH];
  static   char buf [_MAX_PATH];
  static   char *xlat[] = { ".tar.gz", "%.8s.tgz",
                            ".tar.Z",  "%.8s.tz",
                            ".dvi.gz", "%.8s.dgz",
                            ".ps.gz",  "%.8s.pgz"
                          };
#ifdef __DJGPP__
  if (_USE_LFN)
     return (name);  /* return as-is */
#endif

  strcpy (tmp, name);
  for (i = 0; i < DIM(xlat); i += 2)
  {
    char *ext = strstr (tmp, xlat[i]);

    if (ext)
    {
      *ext = '\0';
      sprintf (buf, xlat[i+1], tmp);
      return (buf);
    }
  }
#endif  /* MSDOS */
  return (name);
}

const char *OnOff (int val)
{
  return (val ? _LANG("on") : _LANG("off"));
}

const char *OkComplete (int rc)
{
  return (rc == COMPLETE ? _LANG("ok") : _LANG("failed"));
}

const char *OkFailed (int rc)
{
  return (rc ? _LANG("ok") : _LANG("failed"));
}

/*----------------------------------------------------------------*/

static char *TimeOnline (DWORD t)
{
  static char buf[20];
  int    hour = t / 3600;
  int    mins = t / 60 - 60 * hour;
  int    secs = t % 60;

  sprintf (buf, "%02d:%02d:%02d", hour, mins, secs);
  return (buf);
}

void UpdateTime (time_t start)
{
  time_t now = time (NULL);
  DWORD  onl = (DWORD) difftime (now, start);

  /* "Mon Apr 29 13:14:52 1997" */
  /*             ^ +11          */
  StatusLine (CurrentTime,"%.8s", ctime(&now)+11);
  StatusLine (OnlineTime, "%s",   TimeOnline(onl));
}

struct tm *FileTime (const char *file)
{
#if defined(WIN32)
  struct _stat s;

  if (_stat(file,&s) < 0)
     return (NULL);
  return localtime (&s.st_mtime);
#else
  static   struct tm file_time;
  struct   find_t fi;
  unsigned attr = (_A_NORMAL | _A_ARCH | _A_RDONLY);

  if (_dos_findfirst(file,attr,&fi))
     return (NULL);

  file_time.tm_hour  = (fi.wr_time >> 11) & 31;
  file_time.tm_min   = (fi.wr_time >> 5) & 63;
  file_time.tm_sec   = (fi.wr_time & 31) << 1;

  file_time.tm_year  = (fi.wr_date >> 9) + 1980 - 1900;
  file_time.tm_mday  = (fi.wr_date & 31);
  file_time.tm_mon   = (fi.wr_date >> 5) & 15;
  file_time.tm_mon--;
  file_time.tm_wday  = 0;
  file_time.tm_yday  = 0;
  file_time.tm_isdst = 0;
  return (&file_time);
#endif
}

/*----------------------------------------------------------------*/

int UserPrompt (char *buf, int size, const char *prompt, ...)
{
  char    str[70];
  va_list args;

  va_start (args, prompt);
  VSPRINTF (str, prompt, args);
  xputs (UserText, str);

  if (script_file)
     fgets (buf, size-1, script_file);
  else
  {
    cgets_ed (buf, size-1, 0, NULL, background);
    xputs (UserText, "\r\n");
  }
  rip (buf);
  va_end (args);
  return strlen (buf);
}

int YesNoPrompt (const char *prompt, ...)
{
  char    str[70];
  int     ch;
  const   char *yn;
  va_list args;

  va_start (args, prompt);
  VSPRINTF (str, prompt, args);
  xputs (UserText, str);

  if (script_file)
  {
    ch = fgetc (script_file);
    putch (ch);
  }
  else
  {
    while (!kbhit())
    {
      if (background)
        (*background)();
    }
    ch = getche();
  }
  yn = _LANG ("(y/n)");
  xputs (UserText, "\r\n");
  va_end (args);
  return (toupper(ch) == toupper(yn[1]));
}

/*----------------------------------------------------------------*/

void ShowPrompt (void)
{
  xputs (UserText, _LANG("ftp> "));
}

char *_strncpy (char *dest, const char *src, size_t len)
{
  size_t slen = strlen (src);

  len = min (len, slen);
  memcpy (dest, src, len);
  dest [len] = '\0';
  return (dest);
}

/*----------------------------------------------------------------*/

char *GetPass (const char *prompt)
{
  static char buf[100];

  password_mode = 1;
  UserPrompt (buf, sizeof(buf), prompt);
  password_mode = 0;
  return (buf);
}

/*----------------------------------------------------------------*/

void SetIntensity (int on)
{
#if defined(MSDOS)
  union REGS reg;

#if defined(__WATCOMC__)
  reg.w.ax = 0x1003;
  reg.w.bx = ~on & 1;
  int386 (0x10, &reg, &reg);
#else
  reg.x.ax = 0x1003;
  reg.x.bx = ~on & 1;
  int86 (0x10, &reg, &reg);
#endif
#endif
  ARGSUSED (on);
}

/*----------------------------------------------------------------*/

jmp_buf sigint;
int     sigint_counter = 0;

static void sigint_task (void)
{
  if (current_file)
     fclose (current_file);
  current_file = NULL;

  if (sigint_counter++ == 0)  /* got first ^C */
  {
#ifdef __HIGHC__
    _dx_reset_data();
#endif
    sigint_counter = 0;

    ftp_log ("Got ^C");
    ftp_log_flush();
    sigint_active = 1;
    signal (SIGINT, sig_handler);
    xputs (UserText, "\r\n");
    macro_mode  = 0;
    editing     = 0;
    CloseScript (1);
    CloseTempF();
    DataAbort();
    sigint_active = 0;
    longjmp (cmdline, 1);
    /* not reached */
  }

  ftp_log ("SIGINT caught. Shutting down");
  fast_exit = 1;
  ftp_exit();
}

/*
 *  Because it's not safe to do anything useful in a SIGINT handler,
 *  we use a jmp_buf to enter sigint_task() and process the signal
 *  there.
 */
void setup_sigint (void)
{
#if defined(__DJGPP__)
  __djgpp_set_ctrl_c (0);     /* this was the best I could do */
  _go32_want_ctrl_break (0);
#else
  signal (SIGINT, sig_handler);
#endif

  if (setjmp(sigint))
     sigint_task();
}

#ifdef __HIGHC__          /* disable stack-checking and tracing here */
#pragma Off(check_stack)
#pragma Off(call_trace)
#pragma Off(prolog_trace)
#pragma Off(epilog_trace)
#endif

#ifdef __TURBOC__
#pragma option -N-
#endif

void sig_handler (int sig)
{
  if (c_flag && sig == SIGINT)
     longjmp (sigint, 1);
}

#ifdef __DJGPP__
/*
 * Must install a SIGSEGV handler to free the int29 callback etc.
 */
static void sigsegv_handler (int sig)
{
  int29_exit();
  _eth_release();
  signal (sig, SIG_DFL);
  _exit (-1);
}

void setup_sigsegv (void)
{
  signal (SIGSEGV, sigsegv_handler);
  signal (SIGFPE,  sigsegv_handler);
}

#else
void setup_sigsegv (void)
{
}
#endif

