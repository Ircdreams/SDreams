#!/usr/bin/perl
#&check_line("%s %c %% pouet %d ", "%s%c pouet %% %d");
#exit;

if ($#ARGV < 1) {
   print "USAGE : $ENV{_} <file1> <file2>\n";
   exit;
}

open(FICH1, "$ARGV[0]") || die "Unable to open $ARGV[0]\n";
open(FICH2, "$ARGV[1]") || die "Unable to open $ARGV[1]\n";

@FICH1 = <FICH1>;
@FICH2 = <FICH2>;
$erreur = 0;
for ($i=0;$i <= $#FICH1 && $i <= $#FICH2;$i++) {
	if(check_line($FICH1[$i],$FICH2[$i]) == 0) { print 'Error line '. $i . " ! \n"; $erreur++; }
}
print "$erreur error(s) were found\n";
if ($#FICH1 != $#FICH2) { print "Warning ! $ARGV[0] and $ARGV[1] don't have equal line count ! ($#FICH1 != $#FICH2)\n"; }

exit;


sub check_line {
    if($#_ < 1) { return 0; } #wtf ?
	my @line1 = split("", $_[0]);
	my $line2 = $_[1];
	my $pattern = "^[^%]*";
	my $precedent = 0;
	my $i = 0;
	my @table;
	foreach my $char (@line1) { #let's build %X array
		if ($precedent == 1) { $table[$i] = $char; $i++; $precedent = 0; next; }
		if ($char eq "%") { $precedent = 1; next; }

	}
	foreach $char (@table) {
		$pattern .= '%' . $char . '[^%]*';
	}
	$pattern .= '$';
#	print $pattern."\n";
	if ($_[1] =~ /$pattern/) { return 1; }
	return 0;
}


