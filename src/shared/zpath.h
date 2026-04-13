#ifndef ZPATH_H
#define ZPATH_H

#define ZPATH_MAX 260

void zstr_copy(char *dest, int dest_size, const char *src);
int zstr_append(char *dest, int dest_size, const char *src);
void zpath_join(char *dest, int dest_size, const char *left, const char *right);

#endif
