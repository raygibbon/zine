#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#include "ftp.h"

#if !defined(__WATCOMC__)

static int ctrl_state;
static int last_ch;

static void print_it (char c1, char c2)
{
  char buf[3];

  SetColour (CtrlText);
  buf[0] = c1;
  buf[1] = c2;
  buf[2] = 0;
  cputs (buf);
}

static void process_it (char ch)
{
  if (in_a_shell)
  {
    print_it (ch, 0);
    StatusRedo();
    return;
  }

  if (ch == '^')                         /* possible '^C\r\n' ahead */
  {
    ctrl_state = 1;
  }
  else if (last_ch == '^' && ch != 'C')  /* no it wasn't, print it */
  {
    print_it ('^', ch);
    ctrl_state = 0;
  }
  else if (last_ch == '^' && ch == 'C')  /* yes, push it for conio */
  {
    ungetch (3);
  }
  else if (!ctrl_state)                  /* normal state, print it */
  {
    print_it (ch, 0);
  }
  else if (ch == '\n')                   /* ctrl-state ended */
  {
    ctrl_state = 0;
  }
  last_ch = ch;
}
#endif /* __WATCOMC__ */


#ifdef __HIGHC__
  #include <pharlap.h>
  #include <mw/exc.h>

  static REALPTR int29_old, rm_cb;

  static void int29_isr (SWI_REGS *reg)
  {
    process_it (reg->eax & 255);
  }

  void int29_exit (void)
  {
    if (rm_cb)
    {
      _dx_rmiv_set (0x29, int29_old);
      _dx_free_rmode_wrapper (rm_cb);
    }
    rm_cb = 0;
  }

  void int29_init (void)
  {
    _dx_rmiv_get (0x29, &int29_old);
    rm_cb = _dx_alloc_rmode_wrapper_iret (int29_isr, 40000);
    if (rm_cb)
    {
      _dx_rmiv_set (0x29, rm_cb);
      atexit (int29_exit);
    }
  }

#elif defined(__DJGPP__)
  #include <dpmi.h>
  #include <go32.h>
  #include <crt0.h>

  int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

  static _go32_dpmi_seginfo rm_cb, int29_old;
  static __dpmi_regs        rm_reg;

  static void int29_isr (void)
  {
    process_it (rm_reg.h.al);
  }

  void int29_exit (void)
  {
    if (rm_cb.pm_offset)
    {
      _go32_dpmi_set_real_mode_interrupt_vector (0x29, &int29_old);
      _go32_dpmi_free_real_mode_callback (&rm_cb);
      rm_cb.pm_offset = 0;
    }
  }

  void int29_init (void)
  {
#if 0
    if (int29_old.rm_offset)
       return;
    _go32_dpmi_get_real_mode_interrupt_vector (0x29, &int29_old);
    rm_cb.pm_offset = (unsigned long) &int29_isr;
    if (_go32_dpmi_allocate_real_mode_callback_iret(&rm_cb,&rm_reg) == 0)
    {
      _go32_dpmi_lock_data (&rm_reg, sizeof(rm_reg));
      _go32_dpmi_set_real_mode_interrupt_vector (0x29, &rm_cb);
      atexit (int29_exit);
    }
#endif
  }

/*---------------------------------------------------------------------*/

#elif defined(__WATCOMC__)
  void int29_exit (void) {}
  void int29_init (void) {}

#else   /* Borland C */

  static void interrupt (*int29_old)(void);

  #pragma argsused
  static void interrupt int29_isr (bp,di,si,ds,es,dx,cx,bx,ax,ip,cs,flags)
  {
    process_it (ax & 255);
  }

  void int29_exit (void)
  {
    if (int29_old)
    {
      setvect (0x29, int29_old);
      int29_old = NULL;
    }
  }

  void int29_init (void)
  {
    int29_old = getvect (0x29);
    setvect (0x29, int29_isr);
    atexit (int29_exit);
  }
#endif

