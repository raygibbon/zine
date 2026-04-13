DOS VNC Viewer by Marinos J. Yannikos (mjy@pobox.com)
               and Gisle Vanem (giva@bgnett.no)

Date: Jan 2, 2000

  1. Prerequisites
  2. Configuration
  3. Starting the VNC viewer
  4. Features
  5. Compilation
  6. Copyright
  7. Known Bugs/Problems
  8. Work to do...

1. In order to use this VNC viewer for DOS, you'll need the following:

   A) Hardware

      * A 386+ CPU with ??MB of memory (8MB work fine, 4MB should be no
        problem but have not been tested, even 2MB should suffice; 1MB
        would probably be enough if the network read buffer was smaller)
      * A graphics card with VESA support or a graphics card supported
        by the Allegro library (just try it...). Preferably an SVGA
        compatible one (you need at least a 256 colour mode and that
        isn't very useful with a resolution smaller than 640x400 or so)
      * An Ethernet card with a packet driver.
      * Keyboard, Mouse (not required, but quite useful)

   B) Software

      * An Ethernet packet driver for your Ethernet card. Some drivers
        are available from

             ftp://sunsite.doc.ic.ac.uk/packages/simtel/msdos/pktdrvr/pkt*

      * (optional in some cases) VESA drivers for your graphics hardware
      * (optional) mouse driver

2. Some preliminary instructions on configuring the VNC viewer:

   A) Configure the TCP/IP library as described in the file
      $(WATT_ROOT)\wattcp.txt (edit $(WATT_ROOT)\bin\wattcp.cfg to reflect
      your LAN configuration).
   B) Configure the keyboard mapping using the provided keyconf.exe
      from Allegro (not necessary in most cases).

3. Start the VNC session by running vncserver on your Unix system
   (this DOS viewer hasn't been tested with other versions of the
   VNC server; please test and report any problems/success stories)
   with a resolution and colour depth your graphics hardware supports
   with the Allegro drivers (check this with the provided test.exe
   program from Allegro). Note that 32bpp modes will not work 
   properly since the X clients seem to get confused about them.
   While the vncserver is running, start dosvnc.exe with the
   display name as its first argument, e.g. dosvnc 195.58.162.29:0
   if the Unix server with the vncserver has the IP address 
   195.58.162.29 (if you configure wattcp properly, name resolution
   should work as well). You may leave the VNC viewer by pressing
   Ctrl-Break; the vncserver will keep running.

4. Features (so far):
   * 8/16/24/32 bpp modes seem to work as long as the resolution of the
     VNC desktop is available as a graphics mode with Allegro. 
   * No bell
   * No Clipboard/Cut&Paste support (sorry)
   * No extensive error checking
   * No virtual screens (yet; will start working on this soon)

5. Compilation
   A) Get the Watt-32 TCP library from
      http://www.bgnett.no/~giva/ and compile libwatt.a (the djgpp version)
   B) Make sure Allegro is compiled properly and the header
      files and library are visible to DJGPP.
   C) Fix the paths in the 'Makefile.dj' and say 'make -f makefile.dj'.

   Note: The source code is a mess - it's a quick&dirty hack, so
         be warned...

6. All my code is in the Public Domain (i.e. do whatever you want with
   it) and I don't take any responsibility for its use (i.e. do so at
   your own risk). For all other code, see the respective licenses
   (unless I'm mistaken, the code by the VNC folks is GPL'd, Waterloo
   TCP is copyrighted by its author but may be used freely, Allegro is
   swap-ware (have to send Shawn something...).

7. Known Bugs/Problems:

   - The keyboard handling in the Xvnc server requires that the client's
     keyboard mapping is identical to what X thinks it looks like, in
     order for the X key events to be correct. For example, if you use
     a german keyboard, but X's mapping (as in `xmodmap -pke') is an
     US one, pressing the `^' key will yield the following sequence of
     X keysyms: Shift pressed, `^' pressed, Shift released, `6' released(!).
     As you can see, the `6' is bogus and the correct sequence would
     be `^' pressed, `^' released (or these with a Shift pressed/released
     before and after). I don't like this, and I don't know of a good
     work-around other than faking the required shift events in the client,
     which isn't too difficult, but ugly. Any other ideas?

   - I've tried to hack in support for lsck (a Winsock library for DJGPP,
     which should allow the viewer to use Windows' networking functions
     while running in a DOS box), but all I got was an "unknown error 99".
     Perhaps I can fix it, but don't bet on it.

   - A big problem is that the graphics drivers sometimes break and the
     display gets corrupted if a non-working graphics mode is selected.
     To get around this, try to select the VESA drivers and use a VESA
     TSR program if necessary. This results in more stable graphics
     modes and sometimes higher refresh rates.

   - I don't know whether the binaries work on 386-based PCs (with or
     without FPU). Please let me know if you've tried this.

8. Work to do:

   At some point, I'll try to make a version of the viewer for 16-bit
   DOS, since this has been requested many times and seems useful
   (esp. for my old Sharp PC-3000...). Waterloo TCP/IP exists in a
   16-bit version, I've got a free 16-bit compiler (Pacific C). The
   main obstacle is the lack of a graphics library.

   I'd also like to implement multiple connection support, i.e. the
   ability to switch between different VNC sessions which can be open
   simultaneously.

I hope you'll find this little hack useful. Thanks to the ORL guys for
the concept and well-written code. Please contact me for more information
and/or bug reports (PLEASE report bugs and problems, other people will
benefit from this too!), ideas, success stories...

                                                    Marinos (mjy@pobox.com)
