#!/usr/bin/perl

# name all dt elements and compile a table of contents based on them

my @doc = ();
my @arr = ();
while(<STDIN>){

  if( m/<dt>([^<]*)<\/dt>/ ){
	my $esc = $1;
	my $txt = $1;
	$esc =~ s/\W//g; 
	push(@arr, "<li><a href=\"\#$esc\">$txt</a></li>");
	s/<dt>([^<]*)<\/dt>/<dt><a name="$esc"\/>$txt<\/dt>/;
  } elsif( m/<h(.)>([^<]*)<\/h.>/ && $1 ne "1"){
	my $degree = $1;
	my $txt = $2;
	my $esc = $2;
	$esc =~ s/\W//g; 
	push(@arr, "<li><a href=\"\#$esc\">$txt</a></li>");
	$_ = "<a name=\"$esc\"\/><h$degree>$txt</h$degree>\n";
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

