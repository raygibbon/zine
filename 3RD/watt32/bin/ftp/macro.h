#ifndef __MACRO_H
#define __MACRO_H

#define MAX_MACDEF  20   /* max # of commands in a macro */

struct MacroData {
       char *name;
       char *def [MAX_MACDEF];
       struct MacroData *next;
     };

extern char *init_macro;
extern int   macro_mode;

extern int               ftp_macro (int argc, char **argv);
extern struct MacroData *FindMacro (char *name);

#endif
