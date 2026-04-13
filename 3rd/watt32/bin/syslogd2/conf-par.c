
/*  A Bison parser, made from conf-par.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

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

#line 26 "conf-par.y"

static char *RCSid __attribute__ ((unused)) = "$Id: conf-parse.y,v 1.28 1998/12/01 03:37:50 rand Exp $";

#include "syslogd.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

int yyerror(const char *msg); 
 

#line 39 "conf-par.y"
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
#ifndef YYDEBUG
#define YYDEBUG 1
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		216
#define	YYFLAG		-32768
#define	YYNTBASE	62

#define YYTRANSLATE(x) ((unsigned)(x) <= 316 ? yytranslate[x] : 95)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60,    61
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     4,     5,     7,    10,    11,    13,    16,    17,    19,
    22,    29,    30,    32,    35,    39,    43,    55,    61,    62,
    64,    67,    69,    73,    77,    81,    85,    89,    93,    97,
   101,   105,   109,   113,   117,   121,   125,   129,   133,   137,
   141,   145,   149,   153,   157,   161,   165,   169,   173,   177,
   181,   185,   189,   196,   197,   199,   202,   204,   206,   208,
   215,   216,   218,   221,   223,   225,   227,   231,   233,   241,
   242,   244,   247,   250,   254,   258,   262,   263,   271,   272,
   280,   281,   289,   293,   294,   296,   299,   303,   304,   306,
   309,   311,   315,   319,   323,   325,   329,   331,   333,   335,
   337
};

static const short yyrhs[] = {    63,
    64,    65,     0,     0,    66,     0,    63,    66,     0,     0,
    69,     0,    64,    69,     0,     0,    73,     0,    65,    73,
     0,    41,    60,     6,    67,     7,     8,     0,     0,    68,
     0,    67,    68,     0,     4,    60,     8,     0,     3,    60,
     8,     0,     5,    61,    10,    61,    10,    61,    10,    61,
    11,    61,     8,     0,    42,     6,    70,     7,     8,     0,
     0,    71,     0,    70,    71,     0,    72,     0,    13,    59,
     8,     0,    14,    60,     8,     0,    15,    61,     8,     0,
    16,    61,     8,     0,    17,    59,     8,     0,    18,    51,
     8,     0,    18,    60,     8,     0,    19,    59,     8,     0,
    20,    51,     8,     0,    20,    60,     8,     0,    21,    60,
     8,     0,    22,    60,     8,     0,    23,    60,     8,     0,
    24,    61,     8,     0,    25,    61,     8,     0,    26,    59,
     8,     0,    27,    94,     8,     0,    28,    61,     8,     0,
    28,    60,     8,     0,    29,    61,     8,     0,    30,    60,
     8,     0,    31,    61,     8,     0,    31,    60,     8,     0,
    32,    51,     8,     0,    32,    60,     8,     0,    33,    61,
     8,     0,    34,    51,     8,     0,    34,    60,     8,     0,
    35,    59,     8,     0,    36,    60,     8,     0,    38,    92,
     6,    74,     7,     8,     0,     0,    75,     0,    74,    75,
     0,    82,     0,    81,     0,    76,     0,    39,    79,     6,
    77,     7,     8,     0,     0,    78,     0,    77,    78,     0,
    82,     0,    81,     0,    80,     0,    80,     9,    79,     0,
    60,     0,    40,    54,    49,     6,    82,     7,     8,     0,
     0,    83,     0,    82,    83,     0,    43,     8,     0,    44,
    60,     8,     0,    45,    60,     8,     0,    46,    60,     8,
     0,     0,    44,    60,    84,     6,    88,     7,     8,     0,
     0,    45,    60,    85,     6,    88,     7,     8,     0,     0,
    46,    60,    86,     6,    90,     7,     8,     0,    27,    94,
     8,     0,     0,    89,     0,    88,    89,     0,    35,    59,
     8,     0,     0,    91,     0,    90,    91,     0,    87,     0,
    47,    60,     8,     0,    47,    61,     8,     0,    37,    59,
     8,     0,    93,     0,    92,     9,    93,     0,    48,     0,
    50,     0,    52,     0,    53,     0,    51,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   122,   124,   125,   126,   129,   130,   131,   134,   135,   136,
   139,   141,   142,   143,   146,   147,   148,   152,   154,   155,
   156,   159,   162,   163,   164,   165,   166,   167,   168,   169,
   170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
   180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
   190,   191,   194,   198,   199,   200,   203,   205,   206,   209,
   212,   213,   214,   217,   218,   221,   222,   224,   227,   231,
   232,   233,   237,   238,   239,   240,   241,   241,   243,   243,
   245,   245,   249,   251,   252,   253,   256,   259,   260,   261,
   264,   265,   266,   267,   270,   271,   273,   274,   277,   278,
   279
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","DOMAIN",
"HOST","NETWORK","LBRACE","RBRACE","SEMI","COMMA","DOT","SLASH","CREATE_FILTER",
"DAEMONIZE","DIRECTORY","HASH_TABLE_SIZE","HASH_TABLE_MAX_SIZE","INET_FORWARDING",
"INET_FORWARD_FROM","INET_RECEIVE","INET_RECEIVE_FROM","KLOG_DEVICE","LOCAL_HOSTNAME",
"LOG_DEVICE","LOG_DEVICE_MODE","LOG_FILE_MODE","LOG_UNWANTED_HOSTS","LOG_WITH_PRIORITY",
"MARK_INTERVAL","MAXIMUM_DUPLICATES","PID_FILE","REPEAT_INTERVAL","SIMPLE_HOSTNAME",
"SOCKET_BACKLOG","STRIP_DOMAINS","SYNC_EACH_MESSAGE","SYSLOG_PORT_NAME","WITH_HOST_NAME",
"FACILITY","PROGRAM","PRIORITY","ACL","OPTIONS","BREAK","FILETOK","FIFO","FORWARD",
"PORT","FACILITY_NAME","PRIORITY_NAME","ALL","NONE","NUMERIC","SYMBOLIC","COMPARISON",
"LE","GE","EQ","NE","BOOLEAN","STRING","INTEGER","confFile","aclSection","optionSection",
"facilitySection","aclDeclaration","acls","acl","optionDeclaration","options",
"option","globalOptions","facilityDeclaration","facilityStatements","facilityStatement",
"program","programStatements","programStatement","programNames","programName",
"priority","actions","action","@1","@2","@3","actionOption","fileOptions","fileOption",
"forwardOptions","forwardOption","facilityNames","facilityName","logWithPriority", NULL
};
#endif

static const short yyr1[] = {     0,
    62,    63,    63,    63,    64,    64,    64,    65,    65,    65,
    66,    67,    67,    67,    68,    68,    68,    69,    70,    70,
    70,    71,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    73,    74,    74,    74,    75,    75,    75,    76,
    77,    77,    77,    78,    78,    79,    79,    80,    81,    82,
    82,    82,    83,    83,    83,    83,    84,    83,    85,    83,
    86,    83,    87,    88,    88,    88,    89,    90,    90,    90,
    91,    91,    91,    91,    92,    92,    93,    93,    94,    94,
    94
};

static const short yyr2[] = {     0,
     3,     0,     1,     2,     0,     1,     2,     0,     1,     2,
     6,     0,     1,     2,     3,     3,    11,     5,     0,     1,
     2,     1,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     6,     0,     1,     2,     1,     1,     1,     6,
     0,     1,     2,     1,     1,     1,     3,     1,     7,     0,
     1,     2,     2,     3,     3,     3,     0,     7,     0,     7,
     0,     7,     3,     0,     1,     2,     3,     0,     1,     2,
     1,     3,     3,     3,     1,     3,     1,     1,     1,     1,
     1
};

static const short yydefact[] = {     2,
     0,     5,     3,     0,     0,     8,     4,     6,    12,    19,
     0,     1,     7,     9,     0,     0,     0,     0,    13,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    20,    22,    97,    98,     0,    95,
    10,     0,     0,     0,     0,    14,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   101,    99,   100,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    21,    54,     0,    16,    15,     0,    11,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    41,    40,    42,    43,    45,    44,
    46,    47,    48,    49,    50,    51,    52,    18,     0,     0,
     0,     0,     0,     0,     0,    55,    59,    58,    57,    71,
    96,     0,    68,     0,    66,     0,    73,    77,    79,    81,
     0,    56,    72,     0,    61,     0,     0,    74,     0,    75,
     0,    76,     0,    53,     0,     0,    62,    65,    64,    67,
    70,    84,    84,    88,     0,     0,    63,     0,     0,     0,
    85,     0,     0,     0,     0,    91,     0,    89,     0,    60,
     0,     0,     0,    86,     0,     0,     0,     0,     0,     0,
    90,     0,    69,    87,    78,    80,    83,    94,    92,    93,
    82,     0,    17,     0,     0,     0
};

static const short yydefgoto[] = {   214,
     2,     6,    12,     3,    18,    19,     8,    44,    45,    46,
    14,   135,   136,   137,   166,   167,   144,   145,   138,   139,
   140,   159,   161,   163,   186,   180,   181,   187,   188,    49,
    50,    76
};

static const short yypact[] = {    20,
   -21,   -32,-32768,    40,   114,   -31,-32768,-32768,   127,    74,
    88,   102,-32768,-32768,    61,    81,    84,    55,-32768,    85,
    86,    87,    89,    90,   -45,    92,   -28,    93,    94,    95,
    91,    96,    97,    82,   -39,    98,   100,   -33,   -26,   101,
   -25,    99,   103,    50,-32768,-32768,-32768,-32768,    27,-32768,
-32768,   134,   135,   137,   153,-32768,   156,   157,   158,   159,
   160,   161,   162,   163,   164,   165,   166,   167,   168,   169,
   170,   171,-32768,-32768,-32768,   172,   173,   174,   175,   176,
   177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
-32768,    72,    88,-32768,-32768,   136,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   138,   141,
   188,   139,   140,   142,    -2,-32768,-32768,-32768,    83,-32768,
-32768,   191,-32768,   197,   195,   187,-32768,   198,   199,   200,
   201,-32768,-32768,   144,    79,   138,   204,-32768,   205,-32768,
   206,-32768,   207,-32768,   208,     5,-32768,-32768,    83,-32768,
    83,   189,   189,   -23,   154,   209,-32768,     9,   155,    -6,
-32768,    -4,    82,   190,    53,-32768,    -7,-32768,   210,-32768,
   211,   212,   214,-32768,   215,   217,   218,   219,   220,   221,
-32768,   192,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,   222,-32768,   216,   231,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,   230,-32768,   223,   227,-32768,   193,-32768,
   226,-32768,   104,-32768,-32768,    68,   105,-32768,  -147,  -153,
  -122,-32768,-32768,-32768,-32768,    62,   -43,-32768,    56,-32768,
   147,    59
};


#define	YYLAST		261


static const short yytable[] = {   200,
   193,   169,   195,   183,   151,    62,    11,   168,     1,     5,
     5,   176,   169,   184,    63,   191,   153,   178,   168,   183,
    77,    78,    65,   185,    83,    86,    81,    82,   179,   184,
   179,    66,    92,    84,    87,    93,   129,   130,     4,   185,
   131,   132,   133,   134,   130,     9,   153,   131,   132,   133,
   134,   131,   132,   133,   134,   153,    90,    15,    16,    17,
     1,    55,    20,    21,    22,    23,    24,    25,    26,    27,
    28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
    38,    39,    40,    41,    42,    43,    20,    21,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
   129,   130,   198,   199,   131,   132,   133,   134,   130,    10,
    52,   131,   132,   133,   134,   131,   132,   133,   134,    15,
    16,    17,    73,    74,    75,    47,   194,    48,   194,    11,
    53,    94,    95,    57,    54,    58,    96,    59,    61,    60,
    64,    70,    67,    68,    69,    72,    71,    88,    79,    80,
    97,    85,    89,    98,    99,   100,   101,   102,   103,   104,
   105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
   125,   126,   127,   128,   146,   147,   142,   143,   148,   149,
   154,   150,   155,   156,   165,   158,   160,   162,   164,   171,
   172,   173,   174,   192,   189,   215,   190,   175,   203,   204,
   202,   205,   206,   179,   207,   208,   209,   210,   211,   213,
   216,     7,    13,   177,   182,   157,    91,    51,   152,   141,
    56,   196,   201,     0,     0,     0,     0,     0,   197,     0,
     0,     0,   212,     0,     0,     0,     0,     0,     0,     0,
   170
};

static const short yycheck[] = {     7,
     7,   155,     7,    27,     7,    51,    38,   155,    41,    42,
    42,     7,   166,    37,    60,     7,   139,   171,   166,    27,
    60,    61,    51,    47,    51,    51,    60,    61,    35,    37,
    35,    60,     6,    60,    60,     9,    39,    40,    60,    47,
    43,    44,    45,    46,    40,     6,   169,    43,    44,    45,
    46,    43,    44,    45,    46,   178,     7,     3,     4,     5,
    41,     7,    13,    14,    15,    16,    17,    18,    19,    20,
    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
    31,    32,    33,    34,    35,    36,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    39,    40,    60,    61,    43,    44,    45,    46,    40,     6,
    60,    43,    44,    45,    46,    43,    44,    45,    46,     3,
     4,     5,    51,    52,    53,    48,   180,    50,   182,    38,
    60,     8,     8,    59,    61,    60,    10,    61,    59,    61,
    59,    61,    60,    60,    60,    59,    61,    59,    61,    60,
     8,    61,    60,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,     8,     8,     8,     8,     8,     8,
     8,     8,     8,     8,    54,     8,    61,    60,    60,    60,
    10,    60,     6,     9,    61,     8,     8,     8,     8,     6,
     6,     6,     6,    59,    61,     0,     8,    10,     8,     8,
    11,     8,     8,    35,     8,     8,     8,     8,     8,     8,
     0,     2,     6,   166,   173,    49,    44,    12,   135,    93,
    18,   183,   187,    -1,    -1,    -1,    -1,    -1,    59,    -1,
    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
   156
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 11:
#line 139 "conf-par.y"
{ MakeACL(yyvsp[-4].string, yyvsp[-2].acls); ;
    break;}
case 12:
#line 141 "conf-par.y"
{ yyval.acls = NULL; ;
    break;}
case 13:
#line 142 "conf-par.y"
{ yyval.acls = AppendACL(NULL, yyvsp[0].acl); ;
    break;}
case 14:
#line 143 "conf-par.y"
{ yyval.acls = AppendACL(yyvsp[-1].acls, yyvsp[0].acl); ;
    break;}
case 15:
#line 146 "conf-par.y"
{ yyval.acl = CreateHostACL(yyvsp[-1].string); ;
    break;}
case 16:
#line 147 "conf-par.y"
{ yyval.acl = CreateDomainACL(yyvsp[-1].string); ;
    break;}
case 17:
#line 149 "conf-par.y"
{ yyval.acl = CreateNetworkACL(yyvsp[-9].integer, yyvsp[-7].integer, yyvsp[-5].integer, yyvsp[-3].integer, yyvsp[-1].integer); ;
    break;}
case 23:
#line 162 "conf-par.y"
{ daemonize = yyvsp[-1].boolean; ;
    break;}
case 24:
#line 163 "conf-par.y"
{ SetDirectory(yyvsp[-1].string); ;
    break;}
case 25:
#line 164 "conf-par.y"
{ hashTableSize = yyvsp[-1].integer; ;
    break;}
case 26:
#line 165 "conf-par.y"
{ hashTableMaxSize = yyvsp[-1].integer; ;
    break;}
case 27:
#line 166 "conf-par.y"
{ inetForwarding = yyvsp[-1].boolean; ;
    break;}
case 28:
#line 167 "conf-par.y"
{ SetInetForwardFrom(NULL); ;
    break;}
case 29:
#line 168 "conf-par.y"
{ SetInetForwardFrom(yyvsp[-1].string); ;
    break;}
case 30:
#line 169 "conf-par.y"
{ inetReceive = yyvsp[-1].boolean; ;
    break;}
case 31:
#line 170 "conf-par.y"
{ SetInetReceiveFrom(NULL); ;
    break;}
case 32:
#line 171 "conf-par.y"
{ SetInetReceiveFrom(yyvsp[-1].string); ;
    break;}
case 33:
#line 172 "conf-par.y"
{ klogDevice = yyvsp[-1].string; ;
    break;}
case 34:
#line 173 "conf-par.y"
{ hostName = strdup(yyvsp[-1].string); ;
    break;}
case 35:
#line 174 "conf-par.y"
{ logDevice = yyvsp[-1].string; ;
    break;}
case 36:
#line 175 "conf-par.y"
{ logDeviceMode = yyvsp[-1].integer; ;
    break;}
case 37:
#line 176 "conf-par.y"
{ logFileMode = yyvsp[-1].integer; ;
    break;}
case 38:
#line 177 "conf-par.y"
{ logUnwantedHosts = yyvsp[-1].boolean; ;
    break;}
case 39:
#line 178 "conf-par.y"
{ logWithPriority = yyvsp[-1].logWithPriority; ;
    break;}
case 40:
#line 179 "conf-par.y"
{ markInterval = yyvsp[-1].integer; ;
    break;}
case 41:
#line 180 "conf-par.y"
{ SetMarkInterval(yyvsp[-1].string); ;
    break;}
case 42:
#line 181 "conf-par.y"
{ maxDuplicates = yyvsp[-1].integer; ;
    break;}
case 43:
#line 182 "conf-par.y"
{ pidFile = yyvsp[-1].string; ;
    break;}
case 44:
#line 183 "conf-par.y"
{ repeatInterval = yyvsp[-1].integer; ;
    break;}
case 45:
#line 184 "conf-par.y"
{ SetRepeatInterval(yyvsp[-1].string); ;
    break;}
case 46:
#line 185 "conf-par.y"
{ SetSimpleHostnames(NULL); ;
    break;}
case 47:
#line 186 "conf-par.y"
{ SetSimpleHostnames(yyvsp[-1].string); ;
    break;}
case 48:
#line 187 "conf-par.y"
{ socketBacklog = yyvsp[-1].integer; ;
    break;}
case 49:
#line 188 "conf-par.y"
{ SetStripDomains(NULL); ;
    break;}
case 50:
#line 189 "conf-par.y"
{ SetStripDomains(yyvsp[-1].string); ;
    break;}
case 51:
#line 190 "conf-par.y"
{ syncEachMessage = yyvsp[-1].boolean; ;
    break;}
case 52:
#line 191 "conf-par.y"
{ SetSyslogPort(yyvsp[-1].string); ;
    break;}
case 53:
#line 195 "conf-par.y"
{ MakeFacility(yyvsp[-4].facilityBitmap, yyvsp[-2].program); ;
    break;}
case 54:
#line 198 "conf-par.y"
{ yyval.program = NULL; ;
    break;}
case 55:
#line 199 "conf-par.y"
{ yyval.program = yyvsp[0].program; ;
    break;}
case 56:
#line 200 "conf-par.y"
{ yyval.program = AppendProgram(yyvsp[-1].program, yyvsp[0].program); ;
    break;}
case 57:
#line 203 "conf-par.y"
{ yyval.program = MakeProgram(MakeProgramName(NULL),
							   MakePriority(ge, LOG_DEBUG, MakeActions(yyvsp[0].action))); ;
    break;}
case 58:
#line 205 "conf-par.y"
{ yyval.program = MakeProgram(MakeProgramName(NULL), yyvsp[0].priority); ;
    break;}
case 59:
#line 206 "conf-par.y"
{ yyval.program = yyvsp[0].program; ;
    break;}
case 60:
#line 209 "conf-par.y"
{ yyval.program = MakeProgram(yyvsp[-4].programNames, yyvsp[-2].priority); ;
    break;}
case 61:
#line 212 "conf-par.y"
{ yyval.priority = NULL; ;
    break;}
case 62:
#line 213 "conf-par.y"
{ yyval.priority = yyvsp[0].priority; ;
    break;}
case 63:
#line 214 "conf-par.y"
{ yyval.priority = AppendPriority(yyvsp[-1].priority, yyvsp[0].priority); ;
    break;}
case 64:
#line 217 "conf-par.y"
{ yyval.priority = MakePriority(ge, LOG_DEBUG, MakeActions(yyvsp[0].action)); ;
    break;}
case 65:
#line 218 "conf-par.y"
{ yyval.priority = yyvsp[0].priority; ;
    break;}
case 66:
#line 221 "conf-par.y"
{ yyval.programNames = yyvsp[0].programNames; ;
    break;}
case 67:
#line 222 "conf-par.y"
{ yyval.programNames = AppendProgramName(yyvsp[-2].programNames, yyvsp[0].programNames); ;
    break;}
case 68:
#line 224 "conf-par.y"
{ yyval.programNames = MakeProgramName(yyvsp[0].string); ;
    break;}
case 69:
#line 228 "conf-par.y"
{ yyval.priority = MakePriority(yyvsp[-5].comparison, yyvsp[-4].name, MakeActions(yyvsp[-2].action)); ;
    break;}
case 70:
#line 231 "conf-par.y"
{ yyval.action = NULL; ;
    break;}
case 71:
#line 232 "conf-par.y"
{ yyval.action = yyvsp[0].action; ;
    break;}
case 72:
#line 233 "conf-par.y"
{ yyval.action = AppendAction(yyvsp[-1].action, yyvsp[0].action); ;
    break;}
case 73:
#line 237 "conf-par.y"
{ InitActionOptions(); yyval.action = MakeAction(actionBreak, ""); ;
    break;}
case 74:
#line 238 "conf-par.y"
{ InitActionOptions(); yyval.action = MakeAction(actionFile, yyvsp[-1].string); ;
    break;}
case 75:
#line 239 "conf-par.y"
{ InitActionOptions(); yyval.action = MakeAction(actionFIFO, yyvsp[-1].string); ;
    break;}
case 76:
#line 240 "conf-par.y"
{ InitActionOptions(); yyval.action = MakeAction(actionForward, yyvsp[-1].string); ;
    break;}
case 77:
#line 241 "conf-par.y"
{ InitActionOptions(); ;
    break;}
case 78:
#line 242 "conf-par.y"
{ yyval.action = MakeAction(actionFile, yyvsp[-5].string); ;
    break;}
case 79:
#line 243 "conf-par.y"
{ InitActionOptions(); ;
    break;}
case 80:
#line 244 "conf-par.y"
{ yyval.action = MakeAction(actionFIFO, yyvsp[-5].string); ;
    break;}
case 81:
#line 245 "conf-par.y"
{ InitActionOptions(); ;
    break;}
case 82:
#line 246 "conf-par.y"
{ yyval.action = MakeAction(actionForward, yyvsp[-5].string); ;
    break;}
case 83:
#line 249 "conf-par.y"
{ actionOptions.logWithPriority = yyvsp[-1].logWithPriority; ;
    break;}
case 87:
#line 256 "conf-par.y"
{ actionOptions.syncEachMessage = yyvsp[-1].boolean; ;
    break;}
case 92:
#line 265 "conf-par.y"
{ actionOptions.port = GetPortByName(yyvsp[-1].string); ;
    break;}
case 93:
#line 266 "conf-par.y"
{ actionOptions.port = htons(yyvsp[-1].integer); ;
    break;}
case 94:
#line 267 "conf-par.y"
{ actionOptions.forwardWithHostname = yyvsp[-1].boolean; ;
    break;}
case 95:
#line 270 "conf-par.y"
{ yyval.facilityBitmap = yyvsp[0].facilityBitmap; ;
    break;}
case 96:
#line 271 "conf-par.y"
{ yyval.facilityBitmap = yyvsp[-2].facilityBitmap | yyvsp[0].facilityBitmap; ;
    break;}
case 97:
#line 273 "conf-par.y"
{ yyval.facilityBitmap = facilityNames[yyvsp[0].name].bitValue; ;
    break;}
case 98:
#line 274 "conf-par.y"
{ yyval.facilityBitmap = 0xffffffff; ;
    break;}
case 99:
#line 277 "conf-par.y"
{ yyval.logWithPriority = numeric; ;
    break;}
case 100:
#line 278 "conf-par.y"
{ yyval.logWithPriority = symbolic; ;
    break;}
case 101:
#line 279 "conf-par.y"
{ yyval.logWithPriority = none; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 282 "conf-par.y"


int yyerror(const char *msg)
{
    fprintf(stderr, "%s: %d: %s\n", configFile, lineNumber, msg);
    return(1);
}
