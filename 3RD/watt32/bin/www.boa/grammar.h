typedef union {
  char *sval;
  int   ival;
} YYSTYPE;
#define	B_PORT	258
#define	B_USER	259
#define	B_GROUP	260
#define	B_SERVERADMIN	261
#define	B_SERVERROOT	262
#define	B_ERRORLOG	263
#define	B_ACCESSLOG	264
#define	B_AUXLOG	265
#define	B_SERVERNAME	266
#define	B_DOCUMENTROOT	267
#define	B_USERDIR	268
#define	B_DIRECTORYINDEX	269
#define	B_DIRECTORYCACHE	270
#define	B_DEFAULTTYPE	271
#define	B_ADDTYPE	272
#define	B_ALIAS	273
#define	B_SCRIPTALIAS	274
#define	B_REDIRECT	275
#define	B_KEEPALIVEMAX	276
#define	B_KEEPALIVETIMEOUT	277
#define	MIMETYPE	278
#define	STRING	279
#define	INTEGER	280


extern YYSTYPE yylval;
