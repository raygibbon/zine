#ifndef __SECURE_H
#define __SECURE_H

int secure_flush  (int fd);
int secure_putc   (char c, FILE *stream);
int secure_getc   (FILE *stream);
int secure_write  (int fd, BYTE *buf, int nbyte);
int secure_putbuf (int fd, BYTE *buf, int nbyte);
int secure_read   (int fd, char *buf, int nbyte);

#endif
