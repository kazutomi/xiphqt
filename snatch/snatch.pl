#!/usr/bin/perl

use Socket;
use Sys::Hostname;
use Tk;

use Tk qw(exit); 

my $backchannel_socket="/tmp/snatch";
my $uaddr=sockaddr_un($backchannel_socket);
my $proto=getprotobyname('tcp');

die $! unless socket(LISTEN_SOCK, PF_UNIX, SOCK_STREAM,0);
unlink($backchannel_socket);
die $! unless bind(LISTEN_SOCK,$uaddr);
die $! unless listen(LISTEN_SOCK,SOMAXCONN);
die $! unless accept(COMM_SOCK,LISTEN_SOCK);

undef $/;

while(1){
    $char=getc STDIN;
    syswrite COMM_SOCK,$char,1;
}
