typedef union {
    int integer;
    enum comparisons comparison;
    enum logWithPriorities logWithPriority;
    bool boolean;
    char *string;
    int name;
    int facilityBitmap;
    struct aclEntry *acl;
    struct aclList *acls;
    struct action *action;
    struct priority *priority;
    struct facility *facility;
    struct program *program;
    struct programNames *programNames;
} YYSTYPE;
#define	DOMAIN	258
#define	HOST	259
#define	NETWORK	260
#define	LBRACE	261
#define	RBRACE	262
#define	SEMI	263
#define	COMMA	264
#define	DOT	265
#define	SLASH	266
#define	CREATE_FILTER	267
#define	DAEMONIZE	268
#define	DIRECTORY	269
#define	HASH_TABLE_SIZE	270
#define	HASH_TABLE_MAX_SIZE	271
#define	INET_FORWARDING	272
#define	INET_FORWARD_FROM	273
#define	INET_RECEIVE	274
#define	INET_RECEIVE_FROM	275
#define	KLOG_DEVICE	276
#define	LOCAL_HOSTNAME	277
#define	LOG_DEVICE	278
#define	LOG_DEVICE_MODE	279
#define	LOG_FILE_MODE	280
#define	LOG_UNWANTED_HOSTS	281
#define	LOG_WITH_PRIORITY	282
#define	MARK_INTERVAL	283
#define	MAXIMUM_DUPLICATES	284
#define	PID_FILE	285
#define	REPEAT_INTERVAL	286
#define	SIMPLE_HOSTNAME	287
#define	SOCKET_BACKLOG	288
#define	STRIP_DOMAINS	289
#define	SYNC_EACH_MESSAGE	290
#define	SYSLOG_PORT_NAME	291
#define	WITH_HOST_NAME	292
#define	FACILITY	293
#define	PROGRAM	294
#define	PRIORITY	295
#define	ACL	296
#define	OPTIONS	297
#define	BREAK	298
#define	FILETOK	299
#define	FIFO	300
#define	FORWARD	301
#define	PORT	302
#define	FACILITY_NAME	303
#define	PRIORITY_NAME	304
#define	ALL	305
#define	NONE	306
#define	NUMERIC	307
#define	SYMBOLIC	308
#define	COMPARISON	309
#define	LE	310
#define	GE	311
#define	EQ	312
#define	NE	313
#define	BOOLEAN	314
#define	STRING	315
#define	INTEGER	316


extern YYSTYPE yylval;
