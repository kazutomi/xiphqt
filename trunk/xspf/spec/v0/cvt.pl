#!/usr/bin/perl

my @doc = ();
my @arr = ();
while(<STDIN>){

	if( m/<h(.)>([^<]*)<\/h.>/ && $1 ne "1" && $2 ne ""){
		s/<h(.)>([^<]*)<\/h.>/<\/dd>\n<\/dl>\n<dl>\n<dt class="header$1">$2<\/dt>\n<dd>\n/;
	}

	print;
}

