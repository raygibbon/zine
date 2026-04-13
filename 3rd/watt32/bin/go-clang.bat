set progs=ping.exe finger.exe tcpinfo.exe host.exe htget.exe ^
          popdump.exe tracert.exe revip.exe con-test.exe gui-test.exe ^
          rexec.exe cookie.exe daytime.exe dayserv.exe whois.exe country.exe
rm -f %progs%
make %1 -f clang.mak %progs%


