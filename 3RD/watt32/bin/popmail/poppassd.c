/*
 *
 * Ported by Forrest W. Christian  "Finished" 10/15/95
 * If someone's using shadow passwords and would like a port
 * let me know.  It should take me about an hour.
 *
 * Thanks to the original author of this code for providing
 * a framework for me to port.
 *
 * If you use this port I'd like to hear about it! 
 *
 * Drop any comments, questions, or complaints to
 * forrestc@imach.com
 *
 * Tested on Slackware linux, kernel 1.2.13.
 * compiled with:
 *
 *  gcc -o poppassd poppassd.c 
 *
*/


/* 
 * poppassd.c
 *   Update passwords on the UNIX system from the Eudora
 *   "Change Password" client. Keeps a log of change attempts
 *   in /etc/poppassd.log but suppresses recording of passwords
 *   given by the client.
 *
 *   Must be owned by root, and executable only by root.  It can
 *   be started with an entry in /etc/inetd.conf such as the
 *   following:
 *
 *       poppassd stream tcp nowait root  /usr/local/poppassd/poppassd poppassd
 * 
 *   and in /etc/services:
 * 
 *       poppassd        106/tcp
 *
 */

#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#include <varargs.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/file.h>
#include <time.h>

#define PF_PLATE        "./ptmp%06x"	/* original temporary file */
#define PF_TEMP         "./ptmp"	/* lock file */

#define PW_CHANGED      0	/* a OK */
#define PW_CANTLOCK     -1	/* error code -- locking failed */
#define PW_CANTOPEN     -2	/* error code -- open failed */
#define PW_CANTRENAME   -3	/* error code -- rename failed */
#define PW_NOCHANGE     -4	/* error code -- no passwd entry */

#define SUCCESS 1
#define FAILURE 0
#define BUFSIZE 255
#define MAXARGS 32
#define SUPPRESS 1

#define FI_NULL ((FILE *) NULL)	/* NULL file pointer */
#define TI_NULL ((time_t *) NULL)	/* NULL time pointer */


/* 
 * global variables
 */
char *pf_name = "passwd";	/* password file name */
char  pf_tnam[128];             /* name of temporary file */

FILE  *pf_fp;                   /* pointer to real password file */
FILE  *pf_tfp;                  /* pointer to temporary file */
FILE  *log;
time_t gNow;
char   gDate[BUFSIZE];

int main (int argc, char **argv)
{
  struct passwd *pw;
  char   line[BUFSIZE];
  char   user[BUFSIZE];
  char   pass[BUFSIZE];
  char   newpass[BUFSIZE];
  char   logFile[BUFSIZE];

  chdir ("/etc");
  sprintf (logFile, "/etc/%s.log", argv[0]);
  strcpy (user, "");
  strcpy (pass, "");
  strcpy (newpass, "");

  /*
   * Open the logfile for append
   */
  if (!(log = fopen (logFile, "a+")))
  {
    WriteToClient ("500 Can't open %s.", logFile);
    exit (1);
  }

 /*
  *   The server's responses should be like an FTP server's responses;
  *   1xx for in progress, 2xx for success, 3xx for more information
  *   needed, 4xx for temporary failure, and 5xx for permanent failure.
  *   Putting it all together, here's a sample conversation:
  *
  *   S: 200 hello\r\n
  *   E: user yourloginname\r\n
  *   S: 300 please send your password now\r\n
  *   E: pass yourcurrentpassword\r\n
  *   S: 200 My, that was tasty\r\n
  *   E: newpass yournewpassword\r\n
  *   S: 200 Happy to oblige\r\n
  *   E: quit\r\n
  *   S: 200 Bye-bye\r\n
  *   S: <closes connection>
  *   E: <closes connection>
  */

  WhoAreYou();

  WriteToClient ("200 hello, who are you?");
  ReadFromClient (line, 0);
  sscanf (line, "user %s", user);
  if (strlen(user) == 0)
  {
    WriteToClient ("500 username required.");
    exit (1);
  }

  WriteToClient ("200 your password please.");
  ReadFromClient (line, SUPPRESS);
  sscanf (line, "pass %s", pass);
  if (strlen(pass) == 0)
  {
    WriteToClient ("500 password required.");
    exit (1);
  }

  WriteToClient ("200 your new password please.");
  ReadFromClient (line, SUPPRESS);
  sscanf (line, "newpass %s", newpass);

  /* new pass required
   */
  if (strlen (newpass) == 0)
  {
    WriteToClient ("500 new password required.");
    exit (1);
  }
  /* new pass must be 6 char or longer
   */
  else if (strlen (newpass) < 6)
  {
    WriteToClient ("500 New password too short");
    exit (1);
  }
  /* new pass must be different from old pass
   */
  else if (!strcmp (pass, newpass))
  {
    WriteToClient ("500 New password must be different.");
    exit (1);
  }

  /* test for valid user
   */
  if ((pw = getpwnam (user)) == NULL)
  {
    WriteToClient ("500 Unknown user, %s.", user);
    exit (1);
  }

  /* authenticate user
   */
  if (chkPass (user, pass, pw) == FAILURE)
  {
    WriteToClient ("500 Authentication failure.");
    exit (1);
  }

  if (setPass (user, pass, newpass, pw) == FAILURE)
  {
    WriteToClient ("500 Unable to change password.");
    exit (1);
  }

  WriteToClient ("200 Password changed, thank-you.");
  return (1);
}


void WhoAreYou (void)
{
  struct sockaddr_in bobby;
  struct passwd     *pw;
  int    bobbyLen = sizeof (struct sockaddr);

  fputs ("\n", log);

  if (isatty (fileno (stdin)))
  {
    pw = getpwuid (getuid());
    WriteLog ("Connection on %s by %s",
	      ttyname (fileno (stdin)), pw->pw_name);
  }
  else if (lseek (fileno (stdin), 0, L_INCR) >= 0)
  {
    pw = getpwuid (getuid());
    WriteLog ("Connection on FILE by %s", pw->pw_name);
  }
  else if (getpeername (fileno(stdin), (struct sockaddr*)&bobby, &bobbyLen) < 0)
  {
    pw = getpwuid (getuid());
    WriteLog ("Connection on PIPE by %s", pw->pw_name);
  }
  else
  {
    WriteLog ("NET connection from %s", inet_ntoa (bobby.sin_addr));
  }
}

void WriteToClient (char *fmt, ...)
{
  char string[BUFSIZE];

  vsprintf (string, fmt, (&fmt)+1);
  fprintf (stdout, "%s\r\n", string);
  fflush (stdout);

  WriteLog ("Server> %s", string);
}

void ReadFromClient (char *line, int suppress)
{
  char *sp;
  int   i;

  UpdateTime();
  strcpy (line, "");
  fgets (line, BUFSIZE, stdin);
  sp = strchr (line, '\n');
  if (sp)
     *sp = '\0';

  sp = strchr (line, '\r');
  if (sp)
     *sp = '\0';
  if (suppress)
       WriteLog ("Client> ...");
  else WriteLog ("Client> %s", line);
}

void UpdateTime (void)
{
  struct tm *date;

  gNow = time (NULL);		/* get current calendar time */
  date = localtime (&gNow);	/* convert it to a broken-down time */
  strftime (gDate, BUFSIZE, "%b %d %H:%M:%S", date);
}

void WriteLog (char *fmt, ...)
{
  char string[BUFSIZE];

  UpdateTime();
  vsprintf (string, fmt, (&fmt)+1);
  fprintf (log, "%s: %s\n", gDate, string);
}

int chkPass (char *user, char *pass, struct passwd *pw)
{
  /*  Compare the supplied password with the password file entry
   */
  if (!strcmp (crypt (pass, pw->pw_passwd), pw->pw_passwd))
    return (SUCCESS);
  return (FAILURE);
}



void makesalt (char c[2])       /* salt characters */
{
  long salt;           /* used to compute a salt */
  int  i;              /* counter in a for loop */

  /* just mix a few things up for the salt ...
   * no rhyme or reason here
   */
  salt = (((long)time (TI_NULL)) & 0x3f) | (getpid() << 5);

  /* use the bottom 12 bits and map them into the legal alphabet
   */
  for (i = 0; i < 2; i++)
  {
    c[i] = (salt & 0x3f) + '.';
    if (c[i] > '9')
      c[i] += 7;
    if (c[i] > 'Z')
      c[i] += 6;
    salt >>= 6;
  }
}


int setPass (char *user, char *pass, char *newpass, struct passwd *pw)
{
  char saltc[2];                /* the password's salt */

  makesalt (saltc);
  strcpy (pw->pw_passwd, crypt (newpass, saltc));

  if (UpdatePasswordFile (pw) == PW_CHANGED)
    return (SUCCESS);
  return (FAILURE);
}



int UpdatePasswordFile (struct passwd *p)
{
  char  nbuf[BUFSIZ];            /* buffer for passwords being read */
  char  obuf[BUFSIZ];            /* buffer for new entry */
  char *q, *s;                   /* used for quick name comparison */
  char *ptr;
  int   retval = PW_NOCHANGE;

  /* disable ALL signals at this point
   */
  sigoff();

  /* open the temporary password file
   */
  umask (022);
  ptr = mktemp (PF_PLATE);

  sprintf (pf_tnam, PF_PLATE, getpid());
  pf_tfp = fopen (pf_tnam, "w");
  if ((pf_tfp = fopen (pf_tnam, "w")) == FI_NULL)
  {
    retval = PW_CANTOPEN;
    goto cantlock;
  }

  /* lock the password file
   */
  if (link (pf_tnam, PF_TEMP) < 0)
  {
    retval = PW_CANTLOCK;
    goto cantlock;
  }

  /* copy the new password structure
   */
  sprintf (obuf, "%s:%s:%d:%d:%s:%s:%s\n",
	   p->pw_name, p->pw_passwd, p->pw_uid, p->pw_gid,
	   p->pw_gecos, p->pw_dir, p->pw_shell);

  /* open the password file
   */
  if ((pf_fp = fopen (pf_name, "r")) == FI_NULL)
  {
    retval = PW_CANTOPEN;
    goto getout;
  }

  /* copy the password file into the temporary one
   */
  while (fgets (nbuf, BUFSIZ, pf_fp))
  {
    for (s = nbuf, q = p->pw_name; *s && *s != ':'; s++, q++)
      if (*s != *q)
	break;

    if (*s == ':' && *q == '\0')
    {
      fputs (obuf, pf_tfp);
      retval = PW_CHANGED;
    }
    else
      fputs (nbuf, pf_tfp);
  }
  if (retval == PW_NOCHANGE)
    goto getout;

  /* close the temporary file and the real one
   */
  fclose (pf_tfp);
  pf_tfp = FI_NULL;
  fclose (pf_fp);
  pf_fp = FI_NULL;

  /* now relink; note the lock file is still there
   */
  if (unlink (pf_name) >= 0 &&
      link (pf_tnam, pf_name) >= 0 &&
      unlink (pf_tnam) >= 0)
       retval = PW_CHANGED;
  else retval = PW_CANTRENAME;

getout:
  /* Only remove lock file if this program obtained it
   */
  unlink (PF_TEMP);

cantlock:
  /* some clean up */
  if (pf_tfp != FI_NULL)
     fclose (pf_tfp);
  if (pf_fp != FI_NULL)
     fclose (pf_fp);

  unlink (pf_tnam);
  sigon();                     /* re-enable ALL signals at this point */
  return (retval);
}


/* 
 * signal handling routines
 */
void (*trap[NSIG])();          /* values returned by signal() */

/* 
 * disable ALL signal trapping
 */
void sigoff (void)
{
  int i;               /* counter in a for loop */

  for (i = 0; i < NSIG; i++)
    signal (i, SIG_IGN);
}

/* 
 * restore to the initial setting all signals
 */
void sigon (void)
{
  register int i;		/* counter in a for loop */

  for (i = 0; i < NSIG; i++)
    signal (i, trap[i]);
}
