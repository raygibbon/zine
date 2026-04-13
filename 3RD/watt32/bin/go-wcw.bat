set progs=ping.exe htget.exe finger.exe tcpinfo.exe con-test.exe gui-test.exe tracert.exe
rm -f %progs%
wmake -f wc_win.mak %progs%

