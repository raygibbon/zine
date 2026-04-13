setlocal
:: set CPU=x64
set progs=ping.exe finger.exe tcpinfo.exe host.exe tracert.exe con-test.exe gui-test.exe htget.exe

rm -f %progs%
make -f mingw64.mak %progs%


