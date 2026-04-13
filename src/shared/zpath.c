#include <string.h>

#include "zpath.h"

void zstr_copy(char *dest, int dest_size, const char *src) {
    int i;

    if (dest_size <= 0) {
        return;
    }

    i = 0;
    while (i < dest_size - 1 && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

int zstr_append(char *dest, int dest_size, const char *src) {
    int len;
    int i;

    len = (int)strlen(dest);
    if (len >= dest_size - 1) {
        return 0;
    }

    i = 0;
    while (src[i] != '\0') {
        if (len + i >= dest_size - 1) {
            return 0;
        }
        dest[len + i] = src[i];
        ++i;
    }
    dest[len + i] = '\0';
    return 1;
}

void zpath_join(char *dest, int dest_size, const char *left, const char *right) {
    int len;

    zstr_copy(dest, dest_size, left);
    len = (int)strlen(dest);

    if (len > 0 && dest[len - 1] != '\\' && dest[len - 1] != '/') {
        zstr_append(dest, dest_size, "\\");
    }

    zstr_append(dest, dest_size, right);
}
