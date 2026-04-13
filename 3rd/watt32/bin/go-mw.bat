set progs=ping.exe finger.exe tcpinfo.exe host.exe tracert.exe con-test.exe gui-test.exe htget.exe
rm -f %progs%
make -f mingw32.mak %progs%


