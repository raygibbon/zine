/*
 *  demime.c - MIME base-64 decoder
 *  Christopher Giese <geezer@netwurx.net>
 *
 *  Release date 5/1/98. Distribute freely. ABSOLUTELY NO WARRANTY.
 *
 *  * Compile with a decent compiler like DJGPP or you'll be sorry.
 *  * The input file must be pure MIME data, with Usenet headers
 *    and other stuff snipped away.
 *  * Output file has the same name as input file, with ".out"
 *    extension.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 *  name:   GetByte
 *  action: gets MIME (base-64) encoded byte from opened file Handle
 *  returns:-2  if bad MIME
 *          -1  if EOF or I/O error
 *          >=0 decoded byte
 *
 *  I forget which RFC this came from...
 */
int GetByte (FILE * Handle)
{
  int temp;

  do
    temp = fgetc (Handle);
  while (temp == '=' || temp == '\x0D' || temp == '\x0A');

  if (temp == EOF)
     return (-1);

  if (temp == '+')  return (62);
  if (temp == '/')  return (63);
  if (temp < '0')   return (-2);
  if (temp <= '9')  return (temp - '0' + 52);
  if (temp < 'A')   return (-2);
  if (temp <= 'Z')  return (temp - 'A');
  if (temp < 'a')   return (-2);
  if (temp <= 'z')  return (temp - 'a' + 26);
  return (-2);
}

/*
 *  name:   GetWord
 *  action: gets 24-bit MIME-encoded value from opened file Handle
 *  returns:-2  if bad MIME
 *          -1  if EOF or I/O error
 *          >=0 decoded 24-bit value
 */
long GetWord (FILE *handle)
{
  BYTE buffer[4];
  long rv;

  rv = GetByte (handle);
  if (rv < 0)
     return (rv);

  buffer[0] = rv;
  rv = GetByte (handle);
  if (rv < 0)
     return (rv);

  buffer[1] = rv;
  rv = GetByte (handle);
  if (rv < 0)
     return (rv);

  buffer[2] = rv;
  rv = GetByte (handle);
  if (rv < 0)
     return (rv);
  buffer[3] = rv;
  rv = buffer[0];
  rv <<= 6;
  rv |= buffer[1];
  rv <<= 6;
  rv |= buffer[2];
  rv <<= 6;
  rv |= buffer[3];
  return (rv);
}

int main (int argc, char **argv)
{
  char *name = NULL;
  FILE *Infile, *Outfile;
  long  temp, whichArg;

  if (argc < 2)
  {
    printf ("MIME decoder. Usage:\ndemime [file]\n");
    return (1);
  }

  for (whichArg = 1; whichArg < argc; whichArg++)
  {
    /* copy filename to buffer, with space for 4 extra chars
     */
    char *newName = realloc (name, strlen(argv[whichArg]) + 4);
    if (!newName)
    {
      printf ("Out of memory.\n");
      continue;
    }
    name = newName;
    strcpy (name, argv[whichArg]);

    Infile = fopen (name, "rb");
    if (Infile == NULL)
    {
      printf ("Can't open input file '%s'\n", name);
      continue;
    }

    /* add ".out" extension, open output file
     */
    strcat (name, ".out");
    Outfile = fopen (name, "wb");
    if (Outfile == NULL)
    {
      printf ("Can't open output file '%s'\n", name);
      continue;
    }

    /* start decoding! Instead of Outfile, I was writing to stdout,
     * but DOS (or something) was putting unwanted newlines in the data...
     */
    while ((temp = GetWord (Infile)) >= 0)
       fprintf (Outfile, "%c%c%c", temp >> 16, temp >> 8, temp);

    /* -2=bad MIME, -1=EOF. -1 also equal I/O error...
     */
    if (temp < -1)
       printf ("Error reading input file at offset %lu\n", ftell(Infile));

    fclose (Outfile);
    fclose (Infile);
  }
  if (name)
     free (name);
  return (0);
}
