/*
 * redit.c:   Redirect stderr
 */

#include <assert.h>
#include "boa.h"

static int old_handle = -1;

int redir_stderr (char *fname)
{
  int fd = open(fname, O_WRONLY|O_CREAT|O_TEXT,S_IWRITE);

  assert (fd >= 0);
  old_handle = dup (fileno(stderr));
  dup2 (fd,fileno(stderr));
  close (fd);
  return (fd);
}

void restore_stderr (void)
{
  if (old_handle != -1)
  {
    dup2 (old_handle,fileno(stderr));
    close (old_handle);
    old_handle = -1;
  }
}


