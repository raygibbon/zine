#include "boa.h"

void print_set_fds (void)
{
  int i;

  printf ("\nSET FDS:\n");
  for (i = 0; i < FOPEN_MAX; i++)
  {
    if (FD_ISSET(i, &block_write_fdset))
       printf ("  %d: writing\n", i);
    if (FD_ISSET(i, &block_read_fdset))
       printf ("  %d: reading\n", i);
  }
}

void print_current_fds (void)
{
  struct request *current = request_ready;

  printf ("\nReady requests: ");

  while (current)
  {
    printf ("%d ", current->fd);
    current = current->next;
  }

  printf ("\nBlocked requests: ");
  current = request_block;
  while (current)
  {
    printf ("%d ", current->fd);
    current = current->next;
  }
  printf ("\n");
}

