set progs=ping.exe ident.exe htget.exe finger.exe tcpinfo.exe
rm -f %progs%

:: wmake -f hx-dos.mak %progs%
:: wmake -f dos32a.mak %progs%
:: wmake -f watcom.mak %progs%
   wmake -f causeway.mak %progs%
