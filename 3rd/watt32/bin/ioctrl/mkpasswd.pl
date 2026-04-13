#!/usr/bin/perl

use MIME::Base64;
die "Usage: mkpasswd.pl user password fullname\n" unless $#ARGV >= 2;
($user, $passwd, $fullname) = @ARGV;

$passwd = encode_base64($passwd, '');
print "$user:$passwd:$fullname\n";
