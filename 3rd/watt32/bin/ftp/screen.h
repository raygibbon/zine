#ifndef __SCREEN_H
#define __SCREEN_H

enum TextDestination {
     UserText = 1,
     DataText,
     CtrlText,
     WarnText
   };

enum StatusColumn {
     ProgramVer = 0,
     HostName,
     XferStatistics,
     OnlineTime,
     CurrentTime,
     LastColumn
   };

#define WARN(str)            xputs (WarnText, str)
#define WARN1(fmt,a1)        xprintf (WarnText, fmt, a1)
#define WARN2(fmt,a1,a2)     xprintf (WarnText, fmt, a1, a2)
#define WARN3(fmt,a1,a2,a3)  xprintf (WarnText, fmt, a1, a2, a3)


extern int xprintf (enum TextDestination dest, const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,2,3)))
#endif
;

extern int StatusFill (const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format(printf,1,2)))
#endif
;

extern int  StatusLine (enum StatusColumn column,  const char *fmt, ...);
extern void xputs      (enum TextDestination dest, const char *buf);
extern void xputch     (enum TextDestination dest, int ch);
extern void SetColour  (enum TextDestination dest);
extern int  ScreenInit (void);
extern int  ScreenExit (void);
extern void StatusRedo (void);

#endif
