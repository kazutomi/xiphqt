#!/usr/bin/perl

# name all dt elements and compile a table of contents based on them

my @doc = ();
my @arr = ();
while(<STDIN>){

  if( m/<dt( [^>]*)?>([^<]*)<\/dt>/ ){
	push(@arr, "<li><a href=\"\#$2\">$2</a></li>");
	s/$2/<a name="$2"\/>$2/;
  }

# handles indentation within the table of contents
  if( m/<dl( ([^>]*))?>/ ) {
	push(@arr,"<li $2><ol>");
  }

  if( m/<\/dl>/ ) {
	push(@arr,"</ol></li>");
  }

  push(@doc,$_);
}

my $tocElements = "";
foreach(@arr){
	$tocElements .= "$_\n";
}
my $toc = <<END;
	  <h2>Table of Contents</h2>
	  <ol>
$tocElements
	  </ol>

END

foreach(@doc){
  s/<!-- INSERT TABLE OF CONTENTS HERE -->/$toc/;
  print $_;
}

