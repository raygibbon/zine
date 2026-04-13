#
#  Makefile for Waterloo BOA 0.93.
#
#  Targets:
#     Metaware HighC 3.1+ (PharLap 386|DosX)
#     Borland 4.0+        (DOS large model)
#     Watcom C 10+        (DOS4GW DOS extender)
#     Digital Mars C 8+   (X32VM DOS extender)
#
#  THIS MAKEFILE NEEDS A BORLAND COMPATIBLE MAKE PROGRAM
#

YACC = bison -y
LEX  = flex

#
# Add -DVERBOSE_CGI_LOGS to COMMON_DEFS if you want logs
# of CGI program start and stop times
#
COMMON_DEFS = -DDEBUG -D__MSDOS
DMC_DEFS    = -DYY_USE_PROTOS

PHARLAP_EXP = boa.exp
BORLAND_EXE = boa.exe
WATCOM_EXE  = boa4.exe
DIGMARS_EXE = boa2.exe
QUICKC_EXE  = nul

SRC = grammar.c lexer.c alias.c boa.c cgi.c config.c debug.c get.c \
      hash.c log.c queue.c read.c request.c response.c signals.c util.c

!include "..\common.mak"

lexer.c:   lexer.l
           $(LEX) -olexer.c lexer.l

grammar.c: grammar.y
           $(YACC) -d -ogrammar.c grammar.y

veryclean: clean
           @del lexer.c
           @del grammar.c
           @del grammar.h
