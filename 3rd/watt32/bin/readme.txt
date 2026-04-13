
Sample applications for Wattcp and Watt-32
------------------------------------------

This directory contains the orignal applications distributed with
Wattcp. Some more have been added.

Note that I've not tested all programs with all compilers and DOS
extenders. And beware, I've not tested the tcpport program at all.
It may not work at all !!.

The programs may be compiled for all supported targets. Since a
single makefile cannot be used to produce all variations, the
following makefiles are supplied:

 `makefile'     For Borland C (large model) and Metaware High-C (flat).
                Requires Borland's maker.exe (or other real-mode make)
                for High-C, else make.exe.

 `djgpp.mak'    For GNU C and djgpp; requires GNU make.

 `digmars.mak'  For Digital Mars Compiler (small/large/32-bit models)
                Requires Borland's maker.exe

 `pharlap.mak'  For Metaware High-C and PharLap DLL version. Currently
                not working! Requires Borland's maker.exe

 `powerpak.mak' For Borland's 32-bit PowerPak extender. Requires
                Borland's make.

 `quickc.mak'   For Microsoft Quick-C (large model) executables.
                Requires Microsoft's nmake.

 `visualc.mak'  For 32-bit targets using Visual-C and Pharlap extender.
                Currently not working! Requires Microsoft's nmake.

 `watcom.mak'   For Watcom real-mode (small/large model) executables.
                Requires Watcom's wmake.

 `dos4gw.mak'   For Watcom-386 and DOS4GW extended versions.
                Requires Watcom's wmake.
                Note: `register call' based (-3r) library is default.
                Must match option in .\src\makefile.all (-3r or -3s).

 `pmodew.mak'   For Watcom-386 and PMODE/W extended versions.
                Requires Watcom's wmake. Same note as above.

 `wdosx.mak'    For Watcom-386 and WDOSX extended versions.
                Requires Watcom's wmake. Same note as above.

 `wdosx_bc.mak' For Borland's BCC32 and WDOSX extended versions.
                Currently not working because tlink32 doesn't support
                16-bit modules. Requires Borland's make or maker.

 `wdosx_dm.mak' For Digital Mars Compiler and WDOSX extended versions.
                Requires Borland's make.

Example usage:

 `make -f djgpp.mak'    Uses GNU make to make all djgpp programs.
 `wmake -f watcom.mak'  Uses Watcom's make to make all Watcom real-mode
                        programs.

Compiling single programs is done like. e.g.:

 `make -f djgpp.mak finger.exe' -> to compile only the djgpp version of
                                   finger.exe


Notes
-----

1. `blather.exe' and `lister.exe' requires that the Multicast feature
   is compiled into the Watt-32 library.  Refer .\src\config.h
   and add `#define USE_MULTICAST' to the file.

2. djgpp programs compiles using `tiny.c' to make the .exe-file a bit
   slimmer. Add the `-s' option to `CFLAGS' in `djgpp.mak' to make an
   even slimmer .exe-file.

3. Using `bash.exe' or `sh.exe' won't work for some (all) of the
   djgpp makefiles. I'm uncertain why, but disable any "+SHELL" line
   in your `djgpp.env' file or remove SHELL from the environment while
   building programs with djgpp. Or use e.g. `SHELL=C:\DOS\COMMAND.COM'
   in your `AUTOEXEC.BAT' or `djgpp.env' file.


Ported applications
--------------------

All sub-directories under `.\bin\' contains applications ported from
various sources by me and others. These are mostly programs that is
found on every Unix machine.

`common.mak' is included from each `makefile' in these sub-directories to
ease the support for the various targets.  These targets are supported:

  Borland C large model            (BORLAND_EXE in common.mak)
  MS Quick-C large model           (QUICKC_EXE  in common.mak)
  Watcom-386 with DOS4GW extender  (WATCOM_EXE  in common.mak)
  High-C with PharLap extender     (PHARLAP_EXP in common.mak)
  Digital Mars with X32VM extender (DIGMARS_EXE in common.mak)

`djcommon.mak' is similarily included from each `makefile.dj' files and
is used to ease writing for the djgpp target.

For example, to compile wget for djgpp, cd to `wget' and say:
  `make -f makefile.dj'

This will produce `wget32.exe'. If you're not interested in compiling
with other compilers (and DOS extenders), simply rename .exe-file to
`wget.exe' or modify `makefile.dj' to produce `wget.exe' instead; i.e.
change target line to `DJGPP_EXE = wget.exe'

Note: not all compilers are supported in all sub-directories.
      djgpp is however supported for all programs.

Note: You probably need to remake the dependencies at bottom of the
      `Makefile.dj' files. Do a "make -f Makefile.dj depend" first.

