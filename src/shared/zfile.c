#include <io.h>
#include <stdio.h>

#include "zfile.h"

int zfile_exists(const char *path) {
    return access(path, 0) == 0;
}

FILE *zfile_open_read(const char *path) {
    return fopen(path, "rb");
}
