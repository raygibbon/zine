
#include "vncview.h"
#include "xkeys.h"

#if (ALLEGRO_DATE >= 19991212) /* 3.9.29 WIP */
#define SCANCODE_TO_KEY(k) scancode_to_ascii(k)
#endif

int wanted_w, wanted_h;
int wanted_card = GFX_AUTODETECT;
int in_gfx_mode = 0;

BITMAP *scr = NULL;
RGB     pal[256];

static Bool mouseUpdate = False;

void mouse_cb (int flags)
{
  mouseUpdate = True;
}

static int get_shift (int val)
{
  int sh = 0;

  while ((val & 1) == 0)
  {
    sh++;
    val >>= 1;
  }
  return sh;
}

void init_screen (void)
{
  char s[100];

  /* try an exact match for the requested resolution first */

  if (set_gfx_mode (GFX_AUTODETECT, wanted_w, wanted_h, 0, 0) < 0)
  {
    /* didn't work, so let's inform the user about it */
    sprintf (s, "This display needs %dx%d pixels.", si.framebufferWidth, si.framebufferHeight);
    set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0);
    set_palette (desktop_palette);
    if (alert (s, "Please choose a sufficient resolution.",
               "(greater, for now)\n", " OK ", " Quit ",
               KEY_ENTER, KEY_Q) == 2)
      exit (0);

    for (;;)                /* require explicit "cancel" or working mode */
    {
      if (gfx_mode_select (&wanted_card, &wanted_w, &wanted_h))
      {
	if (wanted_w < si.framebufferWidth || wanted_h < si.framebufferHeight)
	{
	  sprintf (s, "(Requested: %dx%d)", wanted_w, wanted_h);
          alert ("Virtual screens with hardware scrolling",
                 "aren't supported yet!", s, " OK ", NULL, KEY_ENTER, 0);
	}
	else
	{
	  if (set_gfx_mode (wanted_card, wanted_w, wanted_h, 0, 0) < 0)
	  {
	    set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0);
	    alert ("Can't set this graphics mode", NULL, NULL, " Sorry ", NULL, KEY_ENTER, 0);
	  }
	  else
            break;              /* worked */
	}
      }
      else
	return;			/* cancelled; if scr==NULL, we should quit */
    }
  }

  in_gfx_mode = 1;

  /* we've changed to the new graphics mode... */

  if (SCREEN_W == si.framebufferWidth && SCREEN_H == si.framebufferHeight)
  {
    scr = screen;
  }
  else
  {
    scr = create_sub_bitmap (screen,
			     (SCREEN_W - si.framebufferWidth) / 2,
			     (SCREEN_H - si.framebufferHeight) / 2, si.framebufferWidth, si.framebufferHeight);
  }
  set_pallete (pal);
}

void init_ui (void)
{
  fprintf (stderr, "Installing Allegro keyboard routines...\n");
  install_timer();
  install_keyboard();

  mouse_callback = mouse_cb;
  if (install_mouse() < 0)
     fprintf (stderr, "Can't find a mouse!\n");

  set_color_depth (si.format.bitsPerPixel);
  fprintf (stderr, "Setting graphics mode...\n");

  wanted_w = si.framebufferWidth;
  wanted_h = si.framebufferHeight;

  init_screen();
  if (scr == NULL)
    exit (1);			/* no valid mode chosen */

  myFormat.bitsPerPixel = si.format.bitsPerPixel;
  myFormat.depth = si.format.depth;
  if (myFormat.depth == 8)
  {
    myFormat.trueColour = False;
    myFormat.bigEndian = False;
    myFormat.redShift = myFormat.blueShift = myFormat.greenShift = 0;
    myFormat.redMax = myFormat.blueMax = myFormat.greenMax = 0;
  }
  else
  {
    myFormat.trueColour = si.format.trueColour;
    myFormat.bigEndian = False;	/* XXX */
    myFormat.redShift = get_shift (makecol (255, 0, 0));
    myFormat.blueShift = get_shift (makecol (0, 0, 255));
    myFormat.greenShift = get_shift (makecol (0, 255, 0));
    myFormat.redMax = makecol (255, 0, 0) >> myFormat.redShift;
    myFormat.blueMax = makecol (0, 0, 255) >> myFormat.blueShift;
    myFormat.greenMax = makecol (0, 255, 0) >> myFormat.greenShift;
  }
  return;
}

static void dosvnc_options (void)
{
  switch (alert3 ("vncviewer options", NULL, NULL, "Continue", "Quit", "Graphics Mode", KEY_C, KEY_Q, KEY_G))
  {
  case 1:
    break;
  case 2:
    exit (0);
  case 3:
    {
      if (gfx_mode_select (&wanted_card, &wanted_w, &wanted_h))
      {
        init_screen();
      }
    }
    break;

  }
  SendFramebufferUpdateRequest (updateRequestX, updateRequestY, updateRequestW, updateRequestH, False);

}

/* dumb... */

void process_input (void)
{
  if (mouseUpdate)
  {
    int buttons;

    mouseUpdate = False;	/* can lose events here? */
    /* swap buttons 2 & 3 */
    buttons = (mouse_b & ~6) | ((mouse_b & 2) << 1) | ((mouse_b & 4) >> 1);
    SendPointerEvent (mouse_x, mouse_y, buttons);
  }
#if 1
  if (keypressed())
  {
    int k = readkey();
    int r = 0;
    int pressed;

    if ((k & 0xffff) == 0xffff)	/* released */
    {
      pressed = 0;
      k >>= 16;
    }

    switch (k >> 8)
    {
    case KEY_PAUSE:
      if ((key_shifts & (KB_ALT_FLAG | KB_CTRL_FLAG)) == (KB_ALT_FLAG | KB_CTRL_FLAG))
      {
        dosvnc_options();
	/* we'll release those in the mode chooser */
	SendKeyEvent (XK_Control_L, 0);
	SendKeyEvent (XK_Alt_L, 0);
	return;
      }
      else
	r = XK_Break;
      break;
    case KEY_UP:
      r = XK_Up;
      break;
    case KEY_DOWN:
      r = XK_Down;
      break;
    case KEY_LEFT:
      r = XK_Left;
      break;
    case KEY_RIGHT:
      r = XK_Right;
      break;
    case KEY_LCONTROL:
      r = XK_Control_L;
      break;
    case KEY_RCONTROL:
      r = XK_Control_R;
      break;
    case KEY_CAPSLOCK:
      r = XK_Caps_Lock;
      break;
    case KEY_LSHIFT:
      r = XK_Shift_L;
      break;
    case KEY_RSHIFT:
      r = XK_Shift_R;
      break;
    case KEY_PGUP:
      r = XK_Page_Up;
      break;
    case KEY_PGDN:
      r = XK_Page_Down;
      break;
    case KEY_ALT:
      r = XK_Alt_L;
      break;
    case KEY_ALTGR:
      r = XK_Alt_R;
      break;
    case KEY_LWIN:
      r = XK_Meta_L;
      break;
    case KEY_RWIN:
      r = XK_Meta_R;
      break;
    case KEY_F1:
      r = XK_F1;
      break;
    case KEY_F2:
      r = XK_F2;
      break;
    case KEY_F3:
      r = XK_F3;
      break;
    case KEY_F4:
      r = XK_F4;
      break;
    case KEY_F5:
      r = XK_F5;
      break;
    case KEY_F6:
      r = XK_F6;
      break;
    case KEY_F7:
      r = XK_F7;
      break;
    case KEY_F8:
      r = XK_F8;
      break;
    case KEY_F9:
      r = XK_F9;
      break;
    case KEY_F10:
      r = XK_F10;
      break;
    case KEY_F11:
      r = XK_F11;
      break;
    case KEY_F12:
      r = '^';
      break;
    case KEY_HOME:
      r = XK_Home;
      break;
    case KEY_END:
      r = XK_End;
      break;
    case KEY_INSERT:
      r = XK_Insert;
      break;
    case KEY_DEL:
      r = XK_Delete;
      break;
    case KEY_NUMLOCK:
      r = XK_Num_Lock;
      break;
    case KEY_ENTER:
      r = XK_Return;
      break;
    case KEY_ESC:
      r = XK_Escape;
      break;
    case KEY_TAB:
      r = XK_Tab;
      break;
    case KEY_MENU:
      r = XK_Menu;
      break;
    default:
      r = (k & 0xff);
      if (r == 0)
        r = SCANCODE_TO_KEY (k >> 8);
      break;

    }

    SendKeyEvent (r, pressed);
  }
#else
  if (_bios_keybrd (_KEYBRD_READY))
  {
    unsigned k = _bios_keybrd (_KEYBRD_READ);

    if (k == 0xffff)
      exit (0);
    SendKeyEvent (k & 0xff, 1);
  }
#endif
}

void CopyDataToScreen (CARD8 * buf, int x, int y, int width, int height)
{
  int i, j, jj, k;
  unsigned int c, w, bpp;

  bpp = myFormat.bitsPerPixel / 8;
  w = width * bpp;
  switch (bpp)
  {
  case 1:
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
	putpixel (scr, x + j, y + i, buf[j + i * w]);
    break;
  case 2:
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
	putpixel (scr, x + j, y + i, *(CARD16 *) (&buf[j * bpp + i * w]));
    break;
  case 4:
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++)
	putpixel (scr, x + j, y + i, *(CARD32 *) (&buf[j * bpp + i * w]));
    break;
  }
}
