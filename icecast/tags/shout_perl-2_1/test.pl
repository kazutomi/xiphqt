#!/usr/bin/perl -w
use strict;
use Test;

BEGIN { plan tests	=> 3; }

### Test 1: require
use Shout		qw{};
ok( 1 );

### Test 2: constructor
my $streamer = new Shout
	host			=> "localhost",
	port		=> 8000,
	mount		=> "testing",
	password	=> 'pa$$word!';
ok( defined $streamer );


### Test 3: connect/disconnect/error
if ( $streamer->open ) {
	print "Connected...\n";
	$streamer->close;
	ok( 1 );
} else {
	print "Couldn't connect: "; print $streamer->get_error . "\n";
	ok( 1 );
}

