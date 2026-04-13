#include <allegro.h>

int main (void)
{
  int w,h,card;

  allegro_init();
  install_keyboard();
  install_mouse();
  set_gfx_mode (GFX_VGA,320,200,0,0);
  set_pallete (desktop_pallete);
  gfx_mode_select (&card,&w, &h);
  return (0);
}

