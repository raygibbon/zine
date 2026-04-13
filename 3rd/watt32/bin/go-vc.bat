set progs=ping.exe finger.exe tcpinfo.exe host.exe htget.exe ^
          popdump.exe tracert.exe revip.exe con-test.exe gui-test.exe ^
          rexec.exe cookie.exe daytime.exe dayserv.exe lpq.exe lpr.exe ^
          ntime.exe ph.exe stat.exe vlsm.exe whois.exe ident.exe country.exe
rm -f %progs%
nmake %1 -f visualc.mak %progs%


