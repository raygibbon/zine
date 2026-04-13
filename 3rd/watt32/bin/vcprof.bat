@echo off
::
:: Profile Visual VC6 programs.
:: Run this batch as "vcprof prog args..."  (no .exe suffix)
::
set VC6_DIR=G:\vc_2010\VC6

%VC6_DIR\prep.exe    -nologo -om -ft -fv -inc %1.obj %1.exe
%VC6_DIR\profile.exe -nologo -o %1.pbo %1.exe %2 %3 %4 %5 %6 %7
%VC6_DIR\prep.exe    -nologo -fv -m -at %1
%VC6_DIR\plist.exe   -nologo -sc -d ..\src -trace %1
