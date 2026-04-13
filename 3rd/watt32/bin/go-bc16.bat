set progs=ping.exe ident.exe htget.exe finger.exe tcpinfo.exe
rm -f %progs%
maker -f bcc_dos.mak %progs%

