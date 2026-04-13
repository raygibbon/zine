#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ftp.h"

int   macro_mode = 0;
char *init_macro = NULL;

struct MacroData *FindMacro (char *name)
{
  struct MacroData *macro;

  for (macro = cfg.macro; macro; macro = macro->next)
      if (!strcmp(name,macro->name))
         return (macro);
  return (NULL);
}

/*
 * Execute macro at argv[0][1], macro arguments in argv[1..]
 */
int ftp_macro (int argc, char **argv)
{
  int    count    = 2;
  int    loopflg  = 0;
  char  *mac_name = argv[0]+1;
  char   line [200];
  struct MacroData *macro = FindMacro (mac_name);

  if (!macro)
  {
    xprintf (CtrlText,_LANG("`%s' macro not found.\r\n"), mac_name);
    return (0);
  }

  do
  {
    char  *cp1, *cp2;
    char **_argv;
    int    _argc;
    int    i,j;

    i = 0;
    while ((cp1 = macro->def[i]) != NULL)
    {
      cp2 = line;
      while (*cp1)
      {
        switch (*cp1)
        {
          case '\\':
               *cp2++ = *++cp1;
               break;
          case '$':
               if (isdigit(*(cp1+1)))
               {
                 j = 0;
                 while (isdigit(*++cp1))
                       j = 10*j +  *cp1 - '0';
                 cp1--;
                 if (argc - 2 >= j)
                 {
                   strcpy (cp2, argv[j]);
                   cp2 += strlen (argv[j]);
                 }
                 break;
               }
               if (*(cp1+1) == 'i')
               {
                 loopflg = 1;
                 cp1++;
                 if (count < argc)
                 {
                   strcpy (cp2, argv[count-1]);
                   cp2 += strlen (argv[count-1]);
                 }
                 break;
               }
               /* fall-through */
          default:
               *cp2++ = *cp1;
               break;
        }
        if (*cp1)
           ++cp1;
      }

      _argv = MakeArgv (line, &_argc);
      DoCommand (line, _argc, _argv);
    }
  }
  while (loopflg && ++count < argc);

  return (1);
}

