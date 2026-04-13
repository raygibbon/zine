#include <tcp.h>      /* Watt-32 header */
#include <dir.h>      /* MAXPATH        */

#include "vncview.h"

int main (int argc, char **argv)
{
  int last_retrace = 0;

  printf ("DOS VNC viewer v1.1 (Aug-1999) by Marinos J. Yannikos (mjy@pobox.com)\n"
          "and Gisle Vanem <gvanem@yahoo.no>\n"
          "Compiled with DJGPP by DJ Delorie,\n"
          "linked with Allegro %s by Shawn Hargreaves\n"
          "and Watt-32 TCP by Erick Engelke and Gisle Vanem.\n"
          "Contains code (C) by Olivetti & Oracle Research Laboratory (under the GPL).\n\n"
          "Press Ctrl+Alt+Break for options.\n\n", ALLEGRO_VERSION_STR);

  processArgs (argc, argv);

  allegro_init();
  sockets_init();

  if (ConnectToRFBServer (hostname, port) == NULL)
     exit (1);

  if (!InitialiseRFBConnection (rfbsock))
     exit (1);

  init_ui();

  if (!OpenLogFile(argv[0]))
     exit (1);

  if (!SetFormatAndEncodings())
     exit (1);

  if (!SendFramebufferUpdateRequest (updateRequestX, updateRequestY,
                                     updateRequestW, updateRequestH,
                                     False))
     exit (1);

  while (True)
  {
    process_input();

    if (sendUpdateRequest && last_retrace + 3 < retrace_count)
    {
      if (!SendIncrementalFramebufferUpdateRequest())
         exit (1);

      last_retrace = retrace_count;
    }

    if (socket_data_available (rfbsock) > 0)
       if (!HandleRFBServerMessage())
          exit (1);
  }
  return (0);
}

int OpenLogFile (char *argv0)
{
  char   logPath[MAXPATH];
  time_t now;

  strcpy (logPath, argv0);
  *(strrchr(logPath, '/') + 1) = 0;
  strcat (logPath, "dosvnc.log");

  logFile = fopen (logPath, "at");
  if (!logFile)
     return (0);

  now = time (NULL);
  fprintf (logFile, "Session started %s\n", ctime(&now));
  return (1);
}
