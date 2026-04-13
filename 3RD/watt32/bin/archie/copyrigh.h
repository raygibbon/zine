/*
 * Archie Client version 1.5.
 *
 * History:
 *
 * 02/08/97 v1.5        Rewritten for Waterloo TCP/IP, G.Vanem <gvanem@yahoo.no>
 *
 * 10/26/92 v1.4.1      Further fixes, avoid alloca.
 * 10/20/92 v1.4        Added PCNFS support, ported to Solaris 2, print domains
 *                      using reverse-sort, added archie.el to distribution,
 *                      added Alex mode, -V option, use getopt.
 * 04/14/92 v1.3.2      Further porting, add ARCHIE_HOST env var, new version
 *                      of make.com (VMS), support for SVR4, merged 4.2E
 *                      Prospero changes, CUTCP stack fix.
 * 01/10/92 v1.3.1      Bug.
 * 01/09/92 v1.3        CUTCP (DOS) support, more VMS support (UCX), added
 *                      option -L to list hosts & default server.
 * 11/20/91 v1.2        VMS support (Wallongong and Multinet),
 *                      DOS and OS/2 support, new regex.c from oz, added
 *                      udp.c and the testing info for UDP blockage, cope
 *                      with GCC 1.x's bad sparc structure handling, and
 *                      total rewrite of dirsend.c to be more modular (more
 *                      usable by xarchie).
 * 08/20/91 v1.1        Major revisions and hacks of the Prospero code.
 * 07/31/91 v1.0        Original.
 *
 */

/* These are the uw-copyright.h and usc-copyright.h files that appear in
   the Prospero distribution.  */

/*
  Copyright (c) 1989, 1990, 1991 by the University of Washington

  Permission to use, copy, modify, and distribute this software and its
  documentation for non-commercial purposes and without fee is hereby
  granted, provided that the above copyright notice appear in all copies
  and that both the copyright notice and this permission notice appear in
  supporting documentation, and that the name of the University of
  Washington not be used in advertising or publicity pertaining to
  distribution of the software without specific, written prior
  permission.  The University of Washington makes no representations
  about the suitability of this software for any purpose.  It is
  provided "as is" without express or implied warranty.

  Prospero was written by Clifford Neuman (bcn@isi.edu).

  Questions concerning this software should be directed to
  info-prospero@isi.edu.

  */

/*
  Copyright (c) 1991, 1992 by the University of Southern California
  All rights reserved.

  Permission to use, copy, modify, and distribute this software and its
  documentation in source and binary forms for non-commercial purposes
  and without fee is hereby granted, provided that the above copyright
  notice appear in all copies and that both the copyright notice and
  this permission notice appear in supporting documentation. and that
  any documentation, advertising materials, and other materials related
  to such distribution and use acknowledge that the software was
  developed by the University of Southern California, Information
  Sciences Institute.  The name of the University may not be used to
  endorse or promote products derived from this software without
  specific prior written permission.

  THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
  the suitability of this software for any purpose.  THIS SOFTWARE IS
  PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

  Other copyrights might apply to parts of the Prospero distribution
  and are so noted when applicable.

  Prospero was originally written by Clifford Neuman (bcn@isi.edu).
  Contributors include Benjamin Britt (britt@isi.edu)
  and others identified in individual modules.

  Questions concerning this software should be directed to
  info-prospero@isi.edu.

  */
