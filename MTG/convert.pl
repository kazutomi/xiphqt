#!/usr/bin/perl -w

my $from=$ARGV[0];
my $to=$ARGV[1];

print "convert.pl 20020130 from $from to $to\n";

my $output=`file $from`;
my $channels=0;

if($output=~m/WAVE/){
    print "input file is WAV...\n";
    if($output=~m/stereo/){
	$channels=2;
    }
    if($output=~m/mono/){
	$channels=1;
    }

    $output=`sox $from -t RAW -w -s -c $channels -r 44100 $to`;
    tack_header();
    exit(0);
}

print "input is an unsupported file type (for now)\n";

1;

sub tack_header{
    my$footer=pack("ii",55,$channels);
    die $! unless open(TEMP,">>$to");
    syswrite TEMP,$footer;
    close(TEMP);
}
