#!/usr/bin/perl

use Socket;
use Sys::Hostname;
use Tk;

use Tk qw(exit); 

my $backchannel_socket="/tmp/snatch";
my $uaddr=sockaddr_un($backchannel_socket);
my $proto=getprotobyname('tcp');

my $username="e6dbvfc6";
my $password="uwvdgjzy";
my $openfile="/home/xiphmont/foo.ram";
my $openloc="rtp://blah";

my $playcode=join "",("Ks",pack ("S",4),"Kp",pack ("S",4));
my $stopcode=join "",("Ks",pack ("S",4));
my $exitcode=join "",("Kq",pack ("S",4));
my $opencode=join "",("Ko",pack ("S",4));
my $loccode=join "",("Kl",pack ("S",4));

die $! unless socket(LISTEN_SOCK, PF_UNIX, SOCK_STREAM,0);
unlink($backchannel_socket);
die $! unless bind(LISTEN_SOCK,$uaddr);
die $! unless listen(LISTEN_SOCK,SOMAXCONN);
die $! unless accept(COMM_SOCK,LISTEN_SOCK);

send_string("P",$password);
send_string("U",$username);
send_string("O",$openfile);
send_string("L",$openloc);

while(1){
    $char=getc STDIN;
    if($char eq "P"){
	syswrite COMM_SOCK,$playcode;
    }
    if($char eq "S"){
	syswrite COMM_SOCK,$stopcode;
    }
    if($char eq "O"){
	syswrite COMM_SOCK,$opencode;
    }
    if($char eq "L"){
	syswrite COMM_SOCK,$loccode;
    }
    if($char eq "Q"){
	syswrite COMM_SOCK,$exitcode;
    }
    if($char eq "A"){
	syswrite COMM_SOCK,'A';
    }
    if($char eq "I"){
	syswrite COMM_SOCK,'I';
    }
    if($char eq "T"){
	syswrite COMM_SOCK,'T';
    }
}

sub send_string{
    my($op,$code)=@_;
    syswrite COMM_SOCK,$op;
    syswrite COMM_SOCK, (pack 'S', length $code);
    syswrite COMM_SOCK, $code;
}









