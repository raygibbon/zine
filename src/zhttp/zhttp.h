#ifndef ZHTTP_H
#define ZHTTP_H

#include "../shared/zpath.h"

#define ZHTTP_DEFAULT_PORT 8080
#define ZHTTP_RECV_BUFFER 512

#ifndef ZHTTP_FILE_BUFFER
#define ZHTTP_FILE_BUFFER 8192
#endif

#ifndef ZHTTP_LISTEN_SLOTS
#define ZHTTP_LISTEN_SLOTS 2
#endif

#ifndef ZHTTP_IDLE_DELAY_MS
#define ZHTTP_IDLE_DELAY_MS 1
#endif

#ifndef ZHTTP_TIMING_LOG
#define ZHTTP_TIMING_LOG 0
#endif

#ifndef ZHTTP_SEND_TIMEOUT_MS
#define ZHTTP_SEND_TIMEOUT_MS 15000UL
#endif

#ifndef ZHTTP_USE_FASTWRITE
#define ZHTTP_USE_FASTWRITE 1
#endif

typedef struct {
    char method[8];
    char path[ZPATH_MAX];
    int head_only;
} ZHttpRequest;

#endif
