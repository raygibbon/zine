RDEBUG

rdebug is a server that gives access to the memory and I/O space of
the machine. It's useful for remotely debugging adaptors using a PC you
don't mind damaging if something goes wrong, say an old AT or 386.

On the server:

	rdebug -p 10101

starts the server on port 10101. Edit wattcp.cfg appropriately to assign
an IP address or use bootp/DHCP to obtain one.

On the client:

	telnet server 10101

connects you and you can use these commands:

	d 0x1000 0x100

Dumps 100 hex bytes from linear address 1000 hex. Output is formatted
as lines of up to 16 pairs of hex digits.  The count is optional and
defaults to 1.

	s 0x1000 0x55

Enters into location 1000 hex the value 55 hex, and always prints 0.

	i 0x378

Inputs one byte from I/O port 378 hex, and prints the value.

	o 0x378 0x40

Outputs 40 hex to I/O port 378 hex, and always prints 0.

	p 0x378 0x00 0x80

Poll port 378 hex: Input a byte, xor with 0 hex, then and with 80 hex.
If non-zero, print the value of the byte, otherwise repeat. This is
useful for monitoring a port until a bit changes (either to 1 or to 0,
hence the xor mask).

Values can also be specified in octal, starting with 0, or as decimal,
starting with 1-9. Values are always unsigned.

All commands print some value, even if just 0. Invalid commands, invalid
or insufficient arguments cause ? on a line by itself to be printed.

TODO: Provide a C interface client library to these calls.

BUGS: Connection drops out after a timeout (will be fixed in a new
version of the watt32 library). First connection works, but subsequent
connections cause the program to exit (also a library problem).
