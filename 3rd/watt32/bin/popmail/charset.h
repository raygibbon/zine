#ifndef __CHARSET_H
#define __CHARSET_H

extern int  SetCharSet     (const char *str);
extern void decode_ISO8859 (char *str);
extern void encode_ISO8859 (char *str);

#endif
