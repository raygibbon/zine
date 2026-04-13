#ifndef HTTP_REPLY_H
#define HTTP_REPLY_H

#include "net_watt32.h"

int zhttp_send_status(ZNetSocket client, int status, const char *reason, const char *mime_type, long content_length);
int zhttp_send_text_response(ZNetSocket client, int status, const char *reason, const char *body);
int zhttp_send_cached_no_content(ZNetSocket client);

#endif
