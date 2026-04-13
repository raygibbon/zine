#ifndef ZFILE_H
#define ZFILE_H

#include <stdio.h>

int zfile_exists(const char *path);
FILE *zfile_open_read(const char *path);

#endif
