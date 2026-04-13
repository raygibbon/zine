#ifndef SERVE_FILE_H
#define SERVE_FILE_H

#include "net_watt32.h"
#include "zhttp.h"

int zhttp_serve_request(ZNetSocket client, const char *root_dir, const ZHttpRequest *request);

#endif
