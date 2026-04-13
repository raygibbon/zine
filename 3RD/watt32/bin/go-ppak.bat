set progs=ping.exe finger.exe tcpinfo.exe ident.exe htget.exe tracert.exe
rm -f %progs%
maker -f powerpak.mak %progs%

