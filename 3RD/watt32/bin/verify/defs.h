/*
** Declaration of functions.
**
**	@(#)defs.h              e07@nikhef.nl (Eric Wassenaar) 961113
*/

/*
 * Internal declarations of the vrfy utility
 * -----------------------------------------
 */

/* main.c */

void set_defaults (char *, int, char **);
int  getval       (char *, char *, int, int);
void fatal        (char *, ...);
void error        (char *, ...);
void usrerr       (char *, ...);
void message      (char *, ...);
void response     (char *);
void show         (int, char *);
void loop         (char *, char *);
void vrfy         (char *, char *);
int  file         (char *);
int  list         (char *);
int  etrn         (char *, char *);
int  ping         (char *);
int  verify       (char *, char *);
int  vrfyhost     (char *, char *);
int  expnhost     (char *, char *);
int  rcpthost     (char *, char *);
int  etrnhost     (char *, char *);
int  pinghost     (char *);
int  getmxhosts   (char *);

/* pars.c */

char *parselist   (char *);
char *parsespec   (char *, char *, char *);
char *parseaddr   (char *);
char *parsehost   (char *);
char *find_delim  (char *, char);
bool  invalidaddr (char *);
bool  invalidhost (char *);
bool  invalidloop (char *);
char *cataddr     (char *, char *, char *);

/* smtp.c */

int  smtpinit     (char *);
int  smtphelo     (char *, bool);
int  smtpehlo     (char *);
int  smtponex     (void);
int  smtpverb     (char *);
int  smtpetrn     (char *);
int  smtprset     (void);
int  smtpmail     (char *);
int  smtprcpt     (char *);
int  smtpexpn     (char *);
int  smtpvrfy     (char *);
int  smtpquit     (void);
int  smtpreply    (void);

/* stat.c */

void giveresponse (int);

/* mxrr.c */

int getmxbyname   (char *);

/* util.c */

char *maxstr      (char *, int, bool);
char *printable   (char *);
void *xalloc      (void *, u_int);

/* conn.c */

char  *sfgets         (char *, int, void *);
int    makeconnection (char *, void **, void **);
void   setmyhostname  (void);
int    getmyhostname  (char *);
bool   internet       (char *);
u_long numeric_addr   (char *);

