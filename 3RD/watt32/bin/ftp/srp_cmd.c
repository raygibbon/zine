
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/ftp.h>

#ifdef USE_SRP

#include "ftp.h"

unsigned maxbuf;
unsigned actualbuf;
BYTE    *ucbuf;

static char *plevel[1];

static struct levels {
       char  *p_name;
       char  *p_mode;
       int    p_level;
     } levels[] = {
       { "clear",  "C", PROT_C },
       { "safe",   "S", PROT_S },
       { "private","P", PROT_P },
       { NULL                  }
     };

static void setpbsz (unsigned size)
{
  int verb = cfg.verbose;

  if (ucbuf)
     free (ucbuf);
  actualbuf = size;

  while ((ucbuf = malloc(actualbuf)) == NULL)
  {
    if (actualbuf)
       actualbuf >>= 2;
    else
    {
      WARN (_LANG("Error while trying to malloc PROT buffer.\r\n"));
      srp_reset();
      exit (1);
    }
  }
  cfg.verbose = 0;
  reply_parse = "PBSZ=";

  ftp_log ("SRP command: `PBSZ %u'", actualbuf);
  if (Command("PBSZ %u", actualbuf) != COMPLETE)
  {
    WARN (_LANG("Cannot set PROT buffer size.\r\n"));
    exit (1);
  }

  if (reply_parse)
  {
    maxbuf = atol (reply_parse);
    if (maxbuf > actualbuf)
        maxbuf = actualbuf;
  }
  else
    maxbuf = actualbuf;

  reply_parse = NULL;
  cfg.verbose = verb;
}

/*
 * Get current protection level.
 */
char *ftp_getlevel (void)
{
  struct levels *p;

  for (p = levels; p && p->p_level != prot_level; p++)
     ;
  return (p->p_name);
}

/*
 * Set protection level.
 */
int ftp_setlevel (int argc, char **argv)
{
  struct levels *p;

  if (argc > 1)
  {
    char *sep = " ";

    xputs (CtrlText, _LANG("usage: protect ["));
    for (p = levels; p && p->p_name; p++)
    {
      xprintf (CtrlText, "%s%s", sep, p->p_name);
      if (*sep == ' ')
         sep = " | ";
    }
    xputs (CtrlText, " ]\r\n");
    return (0);
  }

  if (argc == 0)
  {
    char *lvl = ftp_getlevel();

    xprintf (CtrlText,
             _LANG("Using %s protection level to transfer files.\r\n"),
             lvl ? lvl : "no");
    return (1);
  }

  for (p = levels; p && p->p_name; p++)
      if (!strcmp(argv[0], p->p_name))
         break;

  if (!p->p_name)
  {
    WARN1 (_LANG("%s: unknown protection level.\r\n"), argv[0]);
    return (0);
  }

  if (!cfg.auth_type || !strcmp(cfg.auth_type,"NONE"))
  {
    if (strcmp(p->p_name, "clear"))
       xprintf (CtrlText, _LANG("Cannot set protection level to %s.\r\n"),
                argv[0]);
    return (0);
  }

  /* Start with a PBSZ of 1 meg
   */
  if (p->p_level != PROT_C)
     setpbsz (1 << 10);

  ftp_log ("SRP command: `PROT %s'", p->p_mode);
  if (Command("PROT %s", p->p_mode) == COMPLETE)
     prot_level = p->p_level;
  return (1);
}

/*
 * command-handlers for setting protection level
 */
int ftp_setclear (void)
{
  plevel[0] = levels[0].p_name;
  return ftp_setlevel (1, plevel);
}

int ftp_setsafe (void)
{
  plevel[0] = levels[1].p_name;
  return ftp_setlevel (1, plevel);
}

int ftp_setpriv (void)
{
  plevel[0] = levels[2].p_name;
  return ftp_setlevel (1, plevel);
}
#endif  /* USE_SRP */

