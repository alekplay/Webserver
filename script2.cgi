#!/usr/bin/perl
# perl-test.cgi -- a simple Perl script test
my $fileName = $ARGV[0];
my $param1 = $ARGV[1];
my $param2 = $ARGV[2];
print "Content-type: text/plain\n\n$fileName : $param1, $param2\n";
