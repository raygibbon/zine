// =====================================================================
// util.c - Utility functions for talk
//
// (C) 1993, 1994 by Michael Ringe, Institut fuer Theoretische Physik,
// RWTH Aachen, Aachen, Germany (michael@thphys.physik.rwth-aachen.de)
//
// This program is free software; see the file COPYING for details.
// =====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "talk.h"

// ---------------------------------------------------------------------
// dostime() - Return date and time in ascii format
// ---------------------------------------------------------------------

char *dostime (void)
{
  struct dostime_t t;
  struct dosdate_t d;
  static char buf[25];
  static char *dow = "SunMonTueWedThuFriSat";
  static char *mon = "JanFebMarAprMayJunJulAugSepOctNovDec";

  _dos_gettime (&t);
  _dos_getdate (&d);
  sprintf (buf,"%.3s %.3s %2.2d %2.2d:%2.2d:%2.2d %4.4d",
           dow + 3*d.dayofweek, mon + 3*(d.month-1),
           d.day, t.hour, t.minute, t.second, d.year);
  return buf;
}

// ---------------------------------------------------------------------
// setsockaddr() - Set socketaddr fields in network byte order
// ---------------------------------------------------------------------

void setsockaddr (struct sockaddr_in *s, WORD port, DWORD ip)
{
  s->sin_family      = intel16 (AF_INET);
  s->sin_port        = intel16 (port);
  s->sin_addr.s_addr = intel (ip);
  memset (s->sin_zero, 0, sizeof(s->sin_zero));
}

