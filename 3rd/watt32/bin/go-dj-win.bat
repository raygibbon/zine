:: set lfn=n
set progs=ping.exe finger.exe tcpinfo.exe ident.exe htget.exe bping.exe tracert.exe country.exe
rm -f %progs%
make -f djgpp_win.mak %progs%


