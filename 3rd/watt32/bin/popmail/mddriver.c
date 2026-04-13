/* 
 * MDDRIVER.C - test driver for MD5 
 *
 * Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
 * rights reserved.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "md5.h"

/* Length of test block, number of test blocks */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

static void MDString    (char *);
static void MDTimeTrial (void);
static void MDTestSuite (void);
static void MDFile      (char *);
static void MDFilter    (void);
static void MDPrint     (BYTE [16]);

/* 
 * Main driver.
 *
 * Arguments (may be any combination):
 *  -sstring - digests string
 *  -t       - runs time trial
 *  -x       - runs test script
 *  filename - digests file
 *  (none)   - digests standard input
 */

int main (int argc, char **argv)
{
  int i;

  if (argc > 1)
     for (i = 1; i < argc; i++)
     {
       if (!strcmp(argv[i],"-s"))
          MDString (argv[i]+2);
       else 
       if (!strcmp (argv[i],"-t"))
          MDTimeTrial();
       else
       if (!strcmp (argv[i],"-x"))
            MDTestSuite();
       else MDFile (argv[i]);
     }
  else
    MDFilter();

  return (0);
}

/* Digests a string and prints the result */
static void MDString (char *string)
{
  MD5_CTX context;
  BYTE    digest[16];
  int     len = strlen (string);

  MD5Init (&context);
  MD5Update (&context,(BYTE*)string, len);
  MD5Final (digest, &context);

  printf ("MD5 (\"%s\") = ", string);
  MDPrint (digest);
  printf ("\n");
}

/* Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte
 * blocks.
 */
static void MDTimeTrial (void)
{
  MD5_CTX context;
  time_t  endTime, startTime;
  BYTE block[TEST_BLOCK_LEN];
  BYTE digest[16];
  int i;

  printf ("MD5 time trial. Digesting %d %d-byte blocks ...",
          TEST_BLOCK_LEN, TEST_BLOCK_COUNT);

  /* Initialize block */
  for (i = 0; i < TEST_BLOCK_LEN; i++)
      block[i] = (BYTE)(i & 0xff);

  /* Start timer */
  time (&startTime);

  /* Digest blocks */
  MD5Init (&context);
  for (i = 0; i < TEST_BLOCK_COUNT; i++)
      MD5Update (&context, block, TEST_BLOCK_LEN);
  MD5Final (digest, &context);

  /* Stop timer */
  time (&endTime);

  printf (" done\n");
  printf ("Digest = ");
  MDPrint (digest);
  printf ("\nTime = %ld seconds\n", (long)(endTime-startTime));
  printf ("Speed = %ld bytes/second\n",
          (long)TEST_BLOCK_LEN * (long)TEST_BLOCK_COUNT/(endTime-startTime));
}

/* Digests a reference suite of strings and prints the results */
static void MDTestSuite (void)
{
  printf ("MD5 test suite:\n");

  MDString ("");
  MDString ("a");
  MDString ("abc");
  MDString ("message digest");
  MDString ("abcdefghijklmnopqrstuvwxyz");
  MDString ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
  MDString ("1234567890123456789012345678901234567890"\
            "1234567890123456789012345678901234567890");
}

/* Digests a file and prints the result */
static void MDFile (char *filename)
{
  FILE    *file;
  MD5_CTX  context;
  BYTE     buffer[1024];
  BYTE     digest[16];
  int      len;

  if ((file = fopen (filename, "rb")) == NULL)
     printf ("%s can't be opened\n", filename);
  else 
  {
    MD5Init (&context);
    while ((len = fread (buffer,1,sizeof(buffer),file)) > 0)
          MD5Update (&context, buffer, len);
    MD5Final (digest, &context);
    fclose (file);

    printf ("MD5 (%s) = ", filename);
    MDPrint (digest);
    printf ("\n");
  }
}

/* Digests the standard input and prints the result */
static void MDFilter (void)
{
  MD5_CTX context;
  BYTE    buffer[16], digest[16];
  int     len;

  MD5Init (&context);
  while ((len = fread (buffer,1,sizeof(buffer),stdin)) > 0)
        MD5Update (&context, buffer, len);
  MD5Final (digest, &context);

  MDPrint (digest);
  printf ("\n");
}

/* Prints a message digest in hexadecimal */
static void MDPrint (BYTE digest[16])
{
  int i;

  for (i = 0; i < 16; i++)
       printf ("%02x", digest[i]);
}
