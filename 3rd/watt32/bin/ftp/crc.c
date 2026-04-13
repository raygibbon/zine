/*
 *  These CRC functions are derived from code in chapter 19 of the book
 *  "C Programmer's Guide to Serial Communications", by Joe Campbell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "ftp.h"
#include "crc.h"

#define CRCBITS       32
#define MASK_CRC(crc) ((DWORD)(crc))
#define CRCHIBIT      ((DWORD) (1L<<(CRCBITS-1)))
#define CRCSHIFTS     (CRCBITS-8)
#define PRZCRC        0x864CFBL   /* PRZ's 24-bit CRC generator polynomial */

static DWORD *crctable;      /* Table for speeding up CRC's */

/*
 * mk_crctbl derives a CRC lookup table from the CRC polynomial.
 * The table is used later by the crcbytes function given below.
 * mk_crctbl only needs to be called once at the dawn of time.
 *
 * The theory behind mk_crctbl is that table[i] is initialized
 * with the CRC of i, and this is related to the CRC of i>>1,
 * so the CRC of i>>1 (pointed to by p) can be used to derive
 * the CRC of i (pointed to by q).
 */
static void mk_crctbl (DWORD poly, DWORD *tab)
{
  int   i;
  DWORD t;
  DWORD *p = tab;
  DWORD *q = tab;

  crctable = p;
  *q++ = 0;
  *q++ = poly;
  for (i = 1; i < 128; i++)
  {
    t = *++p;
    if (t & CRCHIBIT)
    {
      t <<= 1;
      *q++ = t ^ poly;
      *q++ = t;
    }
    else
    {
      t <<= 1;
      *q++ = t;
      *q++ = t ^ poly;
    }
  }
}

/*
 * Accumulate a buffer's worth of bytes into a CRC accumulator,
 * returning the new CRC value.
 */
DWORD string_crc (const char *str)
{
  DWORD       accum = 0;
  const BYTE *buf   = (const BYTE*)str;
  unsigned    len   = strlen (str);

  if (!crctable)
     return (0L);
  do
  {
    accum = accum << 8 ^ crctable[(BYTE)(accum >> CRCSHIFTS) ^ *buf++];
  }
  while (--len);
  return MASK_CRC (accum);
}

/*
 * Initialize the CRC table using our codes
 */
int crc_init (void)
{
  void *p;

  if (crctable)
     return (1);

  p = calloc (256 * sizeof(DWORD), 1);
  if (p)
     mk_crctbl (PRZCRC, (DWORD*)p);
  return (p != NULL);
}

