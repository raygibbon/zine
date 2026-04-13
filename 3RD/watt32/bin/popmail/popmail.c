#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <tcp.h>

#include "popsmtp.h"

int debug_mode = 0;
int apop_mode  = 0;
int fast_close = 0;
int keep_msg   = 0;

void WattInit (void)
{
  POP_debugOn = SMTP_debugOn = 1;
  if (debug_mode)
  {
    tcp_set_debug_state (debug_mode);
    dbug_init();
  }
  sock_init();
}

void Usage (void)
{
  puts("Usage: popmail [-adfkv] {receive|list|head} <host> <user-name> <password>\n"
       "       popmail [-adfkv] send <host> <mail-domain> <file-name>\n"
       "       popmail [-adfkv] from <mailfile>\n"
       "         -a = use APOP login\n"
       "         -d = debug mode\n"
       "         -f = fast socket close\n"
       "         -k = keep message on server\n"
       "         -v = show version");

  exit(1);
}

void SendMail (int argc, char **argv)
{
  tcp_Socket *smtp_sock;

  if (argc != 4)
     Usage();

  WattInit();

  printf ("Sending message %s via %s as %s\n", argv[3], argv[1], argv[2]);

  if ((smtp_sock = smtp_init(argv[1],argv[2])) != NULL)
  {
    FILE *f = fopen (argv[3],"r");
    if (f)
         smtp_sendf (smtp_sock, f);
    else printf ("Cannot open mail-file `%s'\n",argv[3]);
    smtp_shutdown (smtp_sock);
  }
}

void GetMail (int argc, char **argv)
{
  tcp_Socket *pop_sock;
  long count = 0;
  long size  = 0;

  if (argc != 4)
     Usage();

  WattInit();
  pop_sock = pop_init (argv[1]);
  if (!pop_sock)
     return;

  if (pop_login(pop_sock,apop_mode,argv[2],argv[3]))
  {
    if (pop_status(pop_sock, &count, &size))
    {
      int  num = 0;
      FILE *f  = NULL;

      if (count > 0)
         f = fopen ("popmail.out", "ab");

      printf ("%s@%s has %ld messages.  Total size: %ld\n",
              argv[2], argv[1], count, size);

      while (count--)
      {
        time_t now = time (NULL);
        int    ok  = 0;
        fprintf (f,"^*^*- %3d ========================================== %s",
                 ++num,ctime(&now));
        if (keep_msg)
             ok = pop_getf (pop_sock, f, num);
        else ok = pop_get_nextf (pop_sock, f);
        if (!ok)
           break;
      }
      if (f)
      {
        fclose (f);
        printf ("%d messages written to popmail.out\n", num);
      }
    }
  }
  pop_shutdown (pop_sock,fast_close);
  free (pop_sock);
}

void MailFrom (int argc, char **argv)
{
  FILE *f;
  char **to_list;
  int   i;

  if (argc != 2)
     Usage();

  f = fopen (argv[1], "r");
  if (!f)
  {
    printf ("Cannot open mail-file `%s'\n", argv[1]);
    return;
  }
  printf ("From: %s\n", smtp_parse_from_line(f));
  to_list = smtp_parse_to_line(f);
  for (i = 0; to_list[i]; i++)
      printf ("To: %s\n", to_list[i]);
}

void MailList (int argc, char **argv)
{
  tcp_Socket *pop_sock;
  long count = 0;
  long size  = 0;
  
  if (argc != 4)
     Usage();
 
  WattInit();
  pop_sock = pop_init (argv[1]);
  if (!pop_sock)
     return;

  if (pop_login(pop_sock,apop_mode,argv[2],argv[3]))
  {
    if (pop_status(pop_sock,&count,&size))
    {
      long *msg_nums = calloc (sizeof(long),count);
      long *sizes    = calloc (sizeof(long),count);
      assert (msg_nums);
      assert (sizes);
      printf ("You have %ld messages. Total size: %ld\n",count,size);
      pop_list (pop_sock,msg_nums,sizes);
    }
  }
  pop_shutdown (pop_sock,fast_close);
  free (pop_sock);
}

void MailHeads (int argc, char **argv)
{
  tcp_Socket *pop_sock;
  long count = 0;
  long size  = 0;
  long i;

  if (argc != 4)
     Usage();
 
  WattInit();
  pop_sock = pop_init (argv[1]);
  if (!pop_sock)
     return;

  if (pop_login(pop_sock,apop_mode,argv[2],argv[3]))
  {
    if (pop_status(pop_sock,&count,&size))
    {
      int  num = 0;
      FILE *f  = NULL;
      if (count > 0)
         f = fopen ("popmail.out","ab");

      printf ("You have %ld messages. Total size: %ld\n", count, size);
      for (i = 1; i <= count; i++)
      {
        time_t now = time (NULL);
        fprintf (f, "^*^*- %3d ========================================== %s",
                 ++num, ctime(&now));
        if (!pop_header(pop_sock,i,f))
           break;
      }
      if (f)
      {
        fclose (f);
        printf ("%d headers written to popmail.out\n", num);
      }
    }
  }
  pop_shutdown (pop_sock, fast_close);
  free (pop_sock);
}


int main (int argc, char **argv)
{
  int ch;

  while ((ch = getopt(argc,argv,"vadDkf?")) != EOF)
     switch(ch)
     {
       case 'v': puts (wattcpVersion());
                 break;
       case 'a': apop_mode = 1;
                 break;
       case 'd': debug_mode = 1;
                 break;
       case 'D': debug_mode = 2;
                 break;
       case 'k': keep_msg = 1;
                 break;
       case 'f': fast_close = 1;
                 break;
       case '?':
       default : Usage();
     }

  argc -= optind;
  argv += optind;
  if (argc < 2)
     Usage();

       if (!stricmp(argv[0],"send"))    SendMail (argc,argv);
  else if (!stricmp(argv[0],"receive")) GetMail  (argc,argv);
  else if (!stricmp(argv[0],"from"))    MailFrom (argc,argv);
  else if (!stricmp(argv[0],"list"))    MailList (argc,argv);
  else if (!stricmp(argv[0],"head"))    MailHeads(argc,argv);
  else Usage();

  return (0);
}
