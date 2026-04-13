#include <stdio.h>
#include <allegro.h>

volatile int bye = 0;

int kb (int key)
{
  fprintf (stderr,"%04x\n", key);
  if ((key >> 8) == KEY_ESC)
     bye = 1;
  return (key);
}

int main (void)
{
  allegro_init();
  install_keyboard();
  keyboard_callback = kb;

  while (!bye)
    ;

  return (0);
}

