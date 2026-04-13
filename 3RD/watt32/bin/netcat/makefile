#
# Makefile for netcat program
#
# Ken Yap <ken@nlc.net.au>
#

SRC         = netcat.c

COMMON_DEFS = -DMSDOS -DDEBUG -DGAPING_SECURITY_HOLE -DTELNET
BCC_DEFS    = -g200
POWERPAK    = 1

BORLAND_EXE = netcat.exe
PHARLAP_EXP = netcat.exp
WATCOM_EXE  = netcat4.exe  # not yet
DIGMARS_EXE = netcat2.exe  # not yet
QUICKC_EXE  = nul          # not yet

!include "..\common.mak"

HEADERS = generic.h

netcat.obj:  netcat.c $(HEADERS)
netcat.o32:  netcat.c $(HEADERS)
