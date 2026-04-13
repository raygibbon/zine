#ifndef __MISC_H__
#define __MISC_H__

#define copymem( s, d, l)  memcpy( d, s, l)
#define iniciamem( d, l)  memset( d, 0, l)

#define MAX(x)  ( sizeof(x)/sizeof(x[0]) )

#endif
