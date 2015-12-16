#!/usr/bin/perl

use strict;
use warnings;
use Data::Dumper qw(Dumper);
use CGI qw(:standard);
use List::Util qw(min max);

my $fileName = $ARGV[0];
my $counter = 0;
my %wiordcounter;
my @xdata;
my @ydata;
my $maxVal = 0;
my $listCount = 1;

unlink('histogram.png');

foreach my $i (1..$#ARGV){
     push @xdata, $ARGV[$i];
}

open(my $fh, '<:encoding(UTF-8)', $fileName) or die "Could not open file '$fileName' $!";
    while (my $row = <$fh>){
        chomp $row;
        for(my $j=1; $j <= $#ARGV; $j++){
            $counter = ()= $row =~ /$ARGV[$j]/g;
            @ydata[$j]+=$counter;
            $counter = 0;
            }
    }

print "@xdata\n";
print "@ydata\n";

my $filename = 'data.txt';
open(my $fs, '>', $filename) or die "Could not open file!";
foreach (@xdata){
	print $fs "$_\t@ydata[$listCount]\n";
	$listCount++;
}

close $fh;

my $max = max @ydata;

open my $GNUPLOT, "|gnuplot" or die "Couldn't pipe Welp :(";

say {$GNUPLOT} "set terminal png size 1000,1000"; 
say {$GNUPLOT} "set output 'histogram.png'";
say {$GNUPLOT} "set style fill solid";
say{$GNUPLOT} "set xlabel 'Strings'";
say{$GNUPLOT} "set boxwidth 0.5";
say{$GNUPLOT} "set ylabel '# of times shown'";
say{$GNUPLOT} "set yrange [0:$max]";
print {$GNUPLOT} "plot 'data.txt' using 2:xticlabels(1) with boxes ";

close $GNUPLOT;

print "Content-type: text/html\n\n
<html>
	<body>
		<center>
		<h1 style='color:red'> CS410 WebServer</h1>
		<br>
		<img src='histogram.png'>
		</center>			
	</body>
</html>
\n";

