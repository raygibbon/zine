/* This is really a -*- C -*- file */

/* define if you ioctl(2) knows about TIOCNOTTY */
#undef HAVE_TIOCNOTTY

/* Solaris 2.5 and 2.5.1 systems have __snprintf() and __vsnprintf() */
#undef HAVE___SNPRINTF

/* Solaris systems use streams for local logging */
#undef STREAMS_LOGGING

/* HP-UX uses FIFOs for local logging */
#undef FIFO_LOGGING

/* HP-UX uses /dev/klog for kernel logging */
#undef KLOG_LOGGING

/* Linux uses stream based sockets for local logging */
#undef SOCKET_STREAM_LOGGING

/* BSD systems use datagram based sockets for local logging */
#undef SOCKET_DGRAM_LOGGING

/* do we have <sys/strlog.h> */
#undef HAVE_SYS_STRLOG_H

/* do we have <sys/stropts.h> */
#undef HAVE_SYS_STROPTS_H

/* do we have <sys/log.h> */
#undef HAVE_SYS_LOG_H

/*
** do we have <sys/sigevent.h>
** For some stupid reason HP-UX's <stdlib.h> needs <sys/sigevent.h>
** but doesn't incude it.  Sigh.
*/
#undef HAVE_SYS_SIGEVENT_H
