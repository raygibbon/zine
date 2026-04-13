set progs=ping.exe finger.exe tcpinfo.exe htget.exe tracert.exe host.exe popdump.exe revip.exe daytime.exe ph.exe vlsm.exe country.exe
rm -f %progs%
make -f cygwin.mak %progs%


