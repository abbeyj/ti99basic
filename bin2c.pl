#!/usr/bin/perl -w
use strict;

if (@ARGV != 2)
{
	printf STDERR "Usage: bin2c.pl <input_filename> <output_filename>\n";
	exit 1;
}

my ($in_filename, $out_filename) = @ARGV;
my $out_name = $out_filename;
$out_name =~ s/.*\///;            # remove Unix path
$out_name =~ s/.*\\//;            # remove DOS path
$out_name =~ s/\.[^.]*$//;        # remove extension
$out_name =~ s/[^[:alnum:]_]/_/g; # remove illegal characters

open(IN, '<', $in_filename) or die "can't open $in_filename: $!";
binmode IN;
local $/;
my @in = unpack('C*', <IN>);
close(IN);

open(OUT, '>', $out_filename) or die "can't open $out_filename: $!";
print OUT "/* automatically generated from $in_filename */\n";
print OUT "const unsigned char ${out_name}[] = {\n";
my $i;
for ($i = 0; $i < @in; $i++)
{
	printf OUT "0x%02X, ", $in[$i];
	print OUT "\n" if (($i + 1) % 16 == 0);
}
print OUT "\n" if ($i % 16 != 0);
print OUT "};\n";
close(OUT);

exit 0;
