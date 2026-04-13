#ifndef __EDITGETS_H
#define __EDITGETS_H

extern int tab_size, password_mode;

/*
 *  Password mode:
 *
 *  0 - Normal operation
 *  1 - Conventional password mode - '*' is echoed for all characters,
 *      only ESC, ENTER, BACKSPC, and CTLHOME are active.
 */

/* Line editing formatted text entry function */

extern int cgets_ed (char *ret,           /* pointer to return buffer      */
                     int   maxlen,        /* length of return buffer       */
                     int   hist_add,      /* add result to history queue   */
                     char *string,        /* initial entry string          */
                     int (*hook)(void));  /* hook called while no keypress */

extern int getch_extended (void);

#ifdef WIN32
  #define DEF_KEY(dos_raw, win_vk, win_shift)  (win_vk + 0x100*(win_shift))
#else
  #define DEF_KEY(dos_raw, win_vk, win_shift)  dos_raw
#endif

#define Key_ESC         0x1b          /* ASCII codes */
#define Key_ENTER       '\r'
#define Key_TAB         '\t'
#define Key_BACKSPC     '\b'
#define Key_NL          '\n'
#define Key_LFEED       '\n'
#define Key_FFEED       '\f'

/* Function keys
 */
#define Key_F1          DEF_KEY (0x13B, VK_F1, 0)
#define Key_F2          DEF_KEY (0x13C, VK_F2, 0)
#define Key_F3          DEF_KEY (0x13D, VK_F3, 0)
#define Key_F4          DEF_KEY (0x13E, VK_F4, 0)
#define Key_F5          DEF_KEY (0x13F, VK_F5, 0)
#define Key_F6          DEF_KEY (0x140, VK_F6, 0)
#define Key_F7          DEF_KEY (0x141, VK_F7, 0)
#define Key_F8          DEF_KEY (0x142, VK_F8, 0)
#define Key_F9          DEF_KEY (0x143, VK_F9, 0)
#define Key_F10         DEF_KEY (0x144, VK_F10, 0)
#define Key_F11         DEF_KEY (0x185, VK_F11, 0)
#define Key_F12         DEF_KEY (0x186, VK_F12, 0)

/* Ctrl-Function keys
 */
#define Key_CF1         DEF_KEY (0x15E, VK_F1, LEFT_CTRL_PRESSED)
#define Key_CF2         DEF_KEY (0x15F, VK_F2, LEFT_CTRL_PRESSED)
#define Key_CF3         DEF_KEY (0x160, VK_F3, LEFT_CTRL_PRESSED)
#define Key_CF4         DEF_KEY (0x161, VK_F4, LEFT_CTRL_PRESSED)
#define Key_CF5         DEF_KEY (0x162, VK_F5, LEFT_CTRL_PRESSED)
#define Key_CF6         DEF_KEY (0x163, VK_F6, LEFT_CTRL_PRESSED)
#define Key_CF7         DEF_KEY (0x164, VK_F7, LEFT_CTRL_PRESSED)
#define Key_CF8         DEF_KEY (0x165, VK_F8, LEFT_CTRL_PRESSED)
#define Key_CF9         DEF_KEY (0x166, VK_F9, LEFT_CTRL_PRESSED)
#define Key_CF10        DEF_KEY (0x167, VK_F10, LEFT_CTRL_PRESSED)
#define Key_CF11        DEF_KEY (0x189, VK_F11, LEFT_CTRL_PRESSED)
#define Key_CF12        DEF_KEY (0x18A, VK_F12, LEFT_CTRL_PRESSED)

/* Shift-Function keys
 */
#define Key_SF1         DEF_KEY (0x154, VK_F1, SHIFT_PRESSED)
#define Key_SF2         DEF_KEY (0x155, VK_F2, SHIFT_PRESSED)
#define Key_SF3         DEF_KEY (0x156, VK_F3, SHIFT_PRESSED)
#define Key_SF4         DEF_KEY (0x157, VK_F4, SHIFT_PRESSED)
#define Key_SF5         DEF_KEY (0x158, VK_F5, SHIFT_PRESSED)
#define Key_SF6         DEF_KEY (0x159, VK_F6, SHIFT_PRESSED)
#define Key_SF7         DEF_KEY (0x15A, VK_F7, SHIFT_PRESSED)
#define Key_SF8         DEF_KEY (0x15B, VK_F8, SHIFT_PRESSED)
#define Key_SF9         DEF_KEY (0x15C, VK_F9, SHIFT_PRESSED)
#define Key_SF10        DEF_KEY (0x15D, VK_F10, SHIFT_PRESSED)
#define Key_SF11        DEF_KEY (0x187, VK_F11, SHIFT_PRESSED)
#define Key_SF12        DEF_KEY (0x188, VK_F12, SHIFT_PRESSED)

/* Alt-Function keys
 */
#define Key_AF1         DEF_KEY (0x168, VK_F1, LEFT_ALT_PRESSED)
#define Key_AF2         DEF_KEY (0x169, VK_F2, LEFT_ALT_PRESSED)
#define Key_AF3         DEF_KEY (0x16A, VK_F3, LEFT_ALT_PRESSED)
#define Key_AF4         DEF_KEY (0x16B, VK_F4, LEFT_ALT_PRESSED)
#define Key_AF5         DEF_KEY (0x16C, VK_F5, LEFT_ALT_PRESSED)
#define Key_AF6         DEF_KEY (0x16D, VK_F6, LEFT_ALT_PRESSED)
#define Key_AF7         DEF_KEY (0x16E, VK_F7, LEFT_ALT_PRESSED)
#define Key_AF8         DEF_KEY (0x16F, VK_F8, LEFT_ALT_PRESSED)
#define Key_AF9         DEF_KEY (0x170, VK_F9, LEFT_ALT_PRESSED)
#define Key_AF10        DEF_KEY (0x171, VK_F10, LEFT_ALT_PRESSED)
#define Key_AF11        DEF_KEY (0x18B, VK_F11, LEFT_ALT_PRESSED)
#define Key_AF12        DEF_KEY (0x18C, VK_F12, LEFT_ALT_PRESSED)

/* Numeric pad keys
 */
#define Key_INS         DEF_KEY (0x152, VK_INSERT, 1)
#define Key_DEL         DEF_KEY (0x153, VK_DELETE, 1)
#define Key_HOME        DEF_KEY (0x147, VK_HOME, 1)
#define Key_END         DEF_KEY (0x14F, VK_END, 1)
#define Key_PGUP        DEF_KEY (0x149, VK_PRIOR, 1)
#define Key_PGDN        DEF_KEY (0x151, VK_NEXT, 1)
#define Key_UPARROW     DEF_KEY (0x148, VK_UP, 1)
#define Key_DNARROW     DEF_KEY (0x150, VK_DOWN, 1)
#define Key_LTARROW     DEF_KEY (0x14B, VK_LEFT, 1)
#define Key_RTARROW     DEF_KEY (0x14D, VK_RIGHT, 1)

#if !defined(WIN32) || 1
  /* Numeric pad grey keys
   */
  #define Key_PADMIDDLE   0x14C
  #define Key_PADEQ       0x03D
  #define Key_PADPLUS     0x22B
  #define Key_PADMINUS    0x22D
  #define Key_PADASTERISK 0x22A
  #define Key_PADSLASH    0x22F
  #define Key_PADENTER    0x20D

  /* Ctrl-Numeric pad keys
   */
  #define Key_CEND        0x175
  #define Key_CDNARROW    0x191
  #define Key_CPGDN       0x176
  #define Key_CLTARROW    0x173
  #define Key_CPADMIDDLE  0x18F
  #define Key_CRTARROW    0x174
  #define Key_CHOME       0x177
  #define Key_CUPARROW    0x18D
  #define Key_CPGUP       0x184
  #define Key_CINS        0x192
  #define Key_CDEL        0x193
#endif

/* Cursor pad keys
 */
#define Key_PINS        DEF_KEY (0x252, VK_INSERT, 1)
#define Key_PDEL        DEF_KEY (0x253, VK_DELETE, 1)
#define Key_PHOME       DEF_KEY (0x247, VK_HOME, 1)
#define Key_PEND        DEF_KEY (0x24F, VK_END, 1)
#define Key_PPGUP       DEF_KEY (0x249, VK_PRIOR, 1)
#define Key_PPGDN       DEF_KEY (0x251, VK_NEXT, 1)
#define Key_PUPARROW    DEF_KEY (0x248, VK_UP, 1)
#define Key_PDNARROW    DEF_KEY (0x250, VK_DOWN, 1)
#define Key_PLTARROW    DEF_KEY (0x24B, VK_LEFT, 1)
#define Key_PRTARROW    DEF_KEY (0x24D, VK_RIGHT, 1)

#if !defined(WIN32) || 1
  /* Ctrl-Cursor pad keys
   */
  #define Key_CPEND       0x275
  #define Key_CPDNARROW   0x291
  #define Key_CPPGDN      0x276
  #define Key_CPLTARROW   0x273
  #define Key_CPRTARROW   0x274
  #define Key_CPHOME      0x277
  #define Key_CPUPARROW   0x28D
  #define Key_CPPGUP      0x284
  #define Key_CPINS       0x292
  #define Key_CPDEL       0x293

  #define Key_ALTPSLASH   0x1A4         /* Alt-numeric pad grey keys */
  #define Key_ALTPASTRSK  0x137
  #define Key_ALTPMINUS   0x14A
  #define Key_ALTPPLUS    0x14E
  #define Key_ALTPEQUALS  0x183
  #define Key_ALTPENTER   0x1A6

  #define Key_ALTBACKSPC  0x10E         /* Special PC keyboard keys */
  #define Key_CTRLBACKSPC 0x07F
  #define Key_SHIFTTAB    0x10F
  #define Key_CTRLTAB     0x194
  #define Key_ALTESC      0x101
#endif

#define Key_ALT1        0x178         /* Alt keys - number row */
#define Key_ALT2        0x179
#define Key_ALT3        0x17A
#define Key_ALT4        0x17B
#define Key_ALT5        0x17C
#define Key_ALT6        0x17D
#define Key_ALT7        0x17E
#define Key_ALT8        0x17F
#define Key_ALT9        0x180
#define Key_ALT0        0x181
#define Key_ALTMINUS    0x182
#define Key_ALTEQUALS   0x183

#define Key_ALTQ        0x110         /* Alt keys - top alpha row */
#define Key_ALTW        0x111
#define Key_ALTE        0x112
#define Key_ALTR        0x113
#define Key_ALTT        0x114
#define Key_ALTY        0x115
#define Key_ALTU        0x116
#define Key_ALTI        0x117
#define Key_ALTO        0x118
#define Key_ALTP        0x119
#define Key_ALTLBRACE   0x11A
#define Key_ALTRBRACE   0x11B

#define Key_ALTA        0x11E         /* Alt keys - mid alpha row */
#define Key_ALTS        0x11F
#define Key_ALTD        0x120
#define Key_ALTF        0x121
#define Key_ALTG        0x122
#define Key_ALTH        0x123
#define Key_ALTJ        0x124
#define Key_ALTK        0x125
#define Key_ALTL        0x126
#define Key_ALTCOLON    0x127
#define Key_ALTQUOTE    0x128
#define Key_ALTENTER    0x11C

#define Key_ALTZ        0x12C         /* Alt keys - lower alpha row */
#define Key_ALTX        0x12D
#define Key_ALTC        0x12E
#define Key_ALTV        0x12F
#define Key_ALTB        0x130
#define Key_ALTN        0x131
#define Key_ALTM        0x132
#define Key_ALTCOMMA    0x133
#define Key_ALTPERIOD   0x134
#define Key_ALTSLASH    0x135
#define Key_ALTBSLASH   0x12B
#define Key_ALTTILDE    0x129

#define Key_CTRL_C      0x003
#define Key_CTRL_Z      0x01A

#endif /* __EDITGETS_H */
