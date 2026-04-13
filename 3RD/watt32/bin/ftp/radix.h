#ifndef __RADIX_H
#define __RADIX_H

extern int   radix_encode (BYTE *inbuf, BYTE *outbuf, int *len, int decode);
extern char *radix_error  (int e);

#endif
