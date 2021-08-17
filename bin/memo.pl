#!/usr/bin/perl

use strict;
#use warnings;

if ($#ARGV < 1 or $ARGV[1] !~ /^[0-9]*$/) {
   print "USAGE : $ENV{_} <users db> <seuil de listing>\n";
   exit;
}

open(DB, "$ARGV[0]") or die "Unable to open $ARGV[0]\n";

my $current_user;
my %top_recv;
my %top_send;
my $count = 0;

while(my $line = <DB>) {
	if($line =~ /^NICK ([^\s]*) .*/) {
		# save memo count for current user before counting next's ones
		if($count >= $ARGV[1])
		{
			$top_recv{$current_user} = $count;
		}
		$current_user = $1;
		$count = 0;
	}
	elsif($line =~ /^MEMO ([^\s]*) .*/) {
		$top_send{$1}++;
		$count++;
	}
}

close(DB);

my @keyl = sort {$top_recv{$a} - $top_recv{$b}} keys(%top_recv);

foreach my $l (@keyl) {
	print "$l a $top_recv{$l} memos\n";
}

@keyl = sort {$top_send{$b} - $top_send{$a}} keys(%top_send);

print "[ TOP SENDER ]\n";
my $a = 15;
my $x = 1;
foreach my $l (@keyl) {
	if($x > $a) { last; }
        print "$x. $l a émis $top_send{$l} memos\n";
	$x++;
}

