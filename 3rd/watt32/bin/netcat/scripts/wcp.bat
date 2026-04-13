::
::  wcp: a simple HTTP URL cp for UNIX
::   _________________________________________________________________
::
:: More and more developers of applications/programs/other stuff on the
:: Internet are now beginning to distribute their files via HTTP only.
:: One drawback for people who do not have the fastest lines in the
:: world, is that you cannot retrieve a document and have your proccess
:: in the background.
::
:: So I've built this simple but very helpful script named wcp which
:: simply copies the URL in question. It uses the netcat program which is
:: developed by: *Hobbit* (hobbit@avian.org). Netcat is available from
:: the following URL: ftp://ftp.avian.org/src/hacks/. If you do not want
:: to use netcat you may substitute nc with telnet.
::
:: -- adamo@dblab.ntua.gr -.
::
:: Converted to 4DOS and netcat32 (Watt-32/djgpp) G.Vanem <giva@bgnett.no>
::

if %@eval[1+1]. != 2. goto No4DOS

iff %& lt 1 then
  @echo Usage: %0 http://host:port/path/name
  quit
endiff

setlocal
set verbose=-v
set nc=        %WATT_ROOT\bin\netcat\netcat32.exe %verbose
set host_port= `echo %1 | cut -d/ -f3`
set host=      `echo %host_port | cut -d: -f1`
set port=      `echo %host_port | cut -d: -f2`
set pathname=  `echo %1 | cut -d/ -f4-`

@echo GET %1 >> %@path[%0]\wcp.log
echo GET /%pathname | %nc %host %port
quit

:No4DOS
@echo This batch file requires 4DOS.
