set progs=ping.exe ident.exe htget.exe finger.exe tcpinfo.exe tracert.exe
rm -f %progs%
maker -f bcc_win.mak %progs%

