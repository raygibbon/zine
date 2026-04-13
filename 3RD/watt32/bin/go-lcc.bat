set progs=ping.exe finger.exe tcpinfo.exe
rm -f %progs%
nmake -f lcc.mak %progs%


