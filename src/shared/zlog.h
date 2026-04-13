#ifndef ZLOG_H
#define ZLOG_H

void zlog_info(const char *message);
void zlog_request(const char *method, const char *path, int status);

#endif
