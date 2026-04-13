This is a port of syslogd-1.3.3 from the Linux sources to WATT32,
a TCP/IP stack for DOS (in normal and extended 32 bit mode). See
http://www.bgnett.no/~giva/ for WATT32.  snprintf.c was taken from
BeroFTPD but originates from sendmail, according to the comments.

READ THIS DISCLAIMER: THIS PROGRAM IS PROVIDED AS IS WITH NO WARRANTY
AND NO RESPONSIBILITY IS ACCEPTED FOR ANY CONSEQUENCES OF USING IT.
Syslogd is a useful security tool and should be used as part of a
comprehensive security policy. While a DOS machine running syslogd
cannot be broken into because the machine is only passively logging
syslog entries, offering no other services, it is still vulnerable to
denial of service attacks, e.g. flooding the machine with packets to
overflow the log. Therefore it is recommended that you set your firewall
to block syslog packets from any machine outside your net. You may also
wish to log entries to a printer for an indelible record.

The differences from the Linux version are all due to OS support. All
changes to the original source are ifdefed so that future changes to
syslogd can be tracked more easily.

Note these points:

+ You need to configure the supplied wattcp.cfg before running syslogd. If
you set the envvar ETC to /etc, there should be an /etc/services file
and it should contain these lines:

ntp	123/udp
syslog	514/udp

+ Only -r operation makes sense since under DOS there are no other
processes to log, only other machines on the net. Thus most people will
want to start it thus:

	syslogd -r -m1

+ The default config file is in ./syslog.conf.  Users, pipes and wall (*)
are not supported and will fail.  Remote machines don't make sense, except
perhaps for the startup message, so this has not been tested. Filenames
of the form /dev/xxx are converted to filenames of the form xxx so that
/dev/con will log to the console and /dev/prn will log to the printer.
As a special case /dev/console will log to stdout.

+ SIGALRM is not supported in many DOS C compilers so the -m interval
option doesn't work for local marks.

+ One enhancement over the Linux syslogd: this one receives NTP broadcasts
and prints a timestamp to the log. The timestamp is that of the NTP
packet, the local clock is not changed. (In fact, except for the starting
banner, all times are that of the hosts sending the log messages.) If
you want the starting banner and NTP timestamp to be printed as the
local time, you must set the envvar TZ to XXX[+-][N]N for no daylight
saving or XXX[+-][N]NYYY for daylight saving, where XXX is the usual
name of the timezone, YYY the daylight saving name of the timezone, and
[N]N the number of hours west (blank or +) or west (-) of UTC normally.
(Incidentally the names of the timezones are not relevant, what matter
are the offsets.)  The -m option also affects how often the NTP timestamps
are printed.
