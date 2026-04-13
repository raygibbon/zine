
What is it ?
-------------
    massdns reads a list of numeric IP addresses and
    tries to convert them to a domain name.

Why would I want to do that ?
-------------------------------
    If you are running a high-load WEB server a worry about
    its efficiency, you most probably have turned off reverse IP
    lookups for the logs, because using reverse DNS to map a raw
    IP to a name is quite the resource waster.

    This means that your http access logs are full of things
    like 234.54.122.31, but you have no idea who actually came knocking.

    Once in a while, you would like to take that log and batch process
    it (maybe on another machine) to convert those IP's back to names.

    This is precisely what massdns does.

How do I use it ?
------------------
    massdns relies on you having one or many real DNS servers
    around. Let's say you have access to ns1 ns2 ns3 ns4.

    If you have a http log with raw IP's in it, just do:

	    cat httpd.log | cut -d' ' -f1 | massdns ns1 ns2 ns3 ns4

    and watch it go.

What does it run on
-------------------
    Linux.
    Chances are it will run on other Unixes.
    If someone's life depended on it, I could make a Win32 port.

What's nice about massdns
--------------------------
	. fast
	. asynchronous
	. simple to run
	. simple to compile
	. select based (no threads)
	. small (500 lines of code, 40k binary)
	. doesn't rely on gethostbyaddr or resolv(3)
	. doesn't rely on other software/libraries/whatever
	. spreads the resolving tasks on multiple DNS servers, and gets the fastest answer.

What's all that cruft after the actual domain name 
---------------------------------------------------
    Just stats. massdns lets you know how much is done, how much is left to do,
    what was the resolution error code, etc...
    If you don't want to see it:

	    cat httpd.log | cut -d' ' -f1 | massdns ns1 ns2 ns3 ns4 | cut -d' ' -f1-2

Its seems to do almost all of my list, then it hangs
-----------------------------------------------------
    There's always a few raw IP's in your list that are going to give
    the DNS servers a headache. Those are dealt with by massdns by
    a resend+timeout strategy. Currently, the resend is set to 5 times,
    and the timeout is set to 10 seconds for each time. So, massdns
    will only give up on an IP after 50 seconds.

    Practically, it means that massdns should never hang at
    the end of a job for more than a minute. If it does, it's a bug.

I want to change the timeouts
------------------------------
    That would be around line 277 in massdns.cpp
    The next release will probably have options and stuff, but for now ...

Who wrote it
--------------
    Emmanuel Mogenet
    Send bug and suggestions to <mgix@mgix.com>

Where can I find the latest
-----------------------------
    http://www.mgix.com/massdns

Copyright
---------

    a) This code is 100% free.
       Not half-baked free as in the GPL.
       Free as in free: do whatever you want with it.

    b) There is no warranty whatsoever:

			    NO WARRANTY

BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY
FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN
OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES
PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS
TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE
PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,
REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR
REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,
INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED
TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY
YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

