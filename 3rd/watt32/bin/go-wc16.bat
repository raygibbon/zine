set progs=ping.exe ident.exe htget.exe finger.exe tcpinfo.exe
rm -f %progs%
wmake -f watcom.mak %progs%

