#!/usr/bin/perl -w

use strict;
use Shout;
use bytes;

# start the connection
my $conn = new Shout;

# setup all the params
$conn->host('localhost');
$conn->port(8000);
$conn->mount('/testing');
$conn->password('hackme');
$conn->public(0);
$conn->format(SHOUT_FORMAT_MP3);
$conn->protocol(SHOUT_PROTOCOL_HTTP);

# try to connect
if ($conn->open) {
    print "connected...\n";
    $conn->setMetadata("song" => "Streaming from standard in");

    # if we connect, grab data from stdin and shoot it to the server
    my ($buff, $len);
    while (($len = sysread(STDIN, $buff, 4096)) > 0) {
	unless ($conn->send_data($buff)) {
	    print "Error while sending: " . $conn->get_error . "\n";
	    last;
	}
	
	# must be careful not to send the data too fast :)
	$conn->sync;
    }

    # all done
    $conn->close;
} else {
    print "couldn't connect...\n";
}


