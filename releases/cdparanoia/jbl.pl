#!/usr/bin/perl 
#
# jbl.pl - Jewel Box Labeler v0.1
#
# This script interfaces cdparanoia to obtain disc information,
# then connects to a cddb server to get more information.  Finally
# It creates an (I hope) attractive postscript label for your
# jewel box.  The postscript jewel-box routines are pretty generic, 
# so you can also use them for other labeling things.
#
# Copyright (C) 1998 - Jacob Langford 

# This program is free software; you can modify it, distribute it, discard
# this license, whatever.  

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

# Please send improved versions to (langford@uiuc.edu).
# Please send complaints or suggestions to /dev/null.

@cddb_server_list = (
   "cddb.moonsoft.com:8880",
   "cddb.sonic.net:888",
   "sunsite.unc.edu:8880",
   "www.cddb.com:8880",
   "cddb.netads.com:8880",
   "cddb.kagomi.com:8880",
   "cddb.celestial.com:888" );

$user = (getpwuid ($<))[0];
chop ($host = `hostname`);

use Socket;

   $options = shift;
   $quiet    = 1 if ($options =~ /f/);
   unless ($quiet == 1) {
      print <<EOF;

Jewel Box Labeler v0.1
Copyright (C) 1998 Jacob Langford
This is free software, distributed without warranty. 

Usage: jbl.pl        - fully interactive
       jdb.pl -f     - No interaction, output to stdout

EOF
   }

   %disc_info = get_track_info();
   if (%disc_info == 0) {
      print <<EOF;
This program requires that cdparanoia be installed and functioning.
Please check that cdparanoia is installed and functioning, or that
there is a disc in the drive.
EOF
      die;
   }
   $disc_info {id} = get_disc_id (disc_info);
   for $cddb_server (@cddb_server_list) {
      ($server, $port) = split (":", $cddb_server);
      %cddb_entry = 
         get_cddb_entry ($server, $port, disc_info);
      last unless ( %cddb_entry == NULL  );
   }
   if ($quiet == 1) {
      $cddb_choice = 0;
      if (%cddb_entry == NULL) {
	 $cddb_entry {count} = 1;
	 $cddb_entry {full_entry} [0] = empty_cddb_entry (disc_info);
      }
   } else {
CHOOSE:
      print "\nPlease choose one of the following entries:\n";
      for ($i = 0; $i < $cddb_entry {count}; $i++) {
	 print "   $i.  $cddb_entry{title}[$i]\n";
      }
      print "   $i.  Blank cddb entry\n";
      chop ($cddb_choice = <STDIN>);
      goto CHOOSE if ( ($choice < 0) || ($choice > $i) ); 
      if ($choice == $i) {
         $cddb_entry {full_entry} [$i] = empty_cddb_entry (disc_info);
      }
   }

VERIFY:
   set_textual_info (disc_info, 
      $cddb_entry {full_entry} [$cddb_choice] );
   unless ($quiet == 1) {
      $output_filename = "label.ps";
      display_textual_info (disc_info);
      print "Would you like to edit the entry before printing?  [n] :";
      $response = <STDIN>; 
      if ($response =~ /y/i) {
	 $cddb_entry {full_entry} [$cddb_choice] =
	    external_edit ( $cddb_entry {full_entry} [$cddb_choice] );
	 goto VERIFY;
      }
      print "Please enter a filename for output.  [label.ps] :";
      chop ($response = <STDIN>);
      unless ($response eq '') {
	 $output_filename = $response;
      }
   }

   make_label ( disc_info, @args );

# End of main()
  
sub set_textual_info {

   %disc_info = %{ shift() };
   my ($cddb_text) = shift;

   my ($track);

   $disc_info {title} = '';
   $disc_info {artist} = '';
   if ($cddb_text =~ /DTITLE=(.*?)\n/) {
      ($disc_info {artist}, $disc_info {title}) = 
         split (" / ", $1);
   } 
   for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $disc_info {track_title} [$track] = '';
      if ($cddb_text =~ /TITLE${track}=(.*?)\n/) {
	 $disc_info {track_title} [$track] = $1;
      } 
   }

}

sub display_textual_info {

   %disc_info = %{ shift() };
   my ($track);

format STDOUT =
   @|  @>:@>  @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
   $format_i, $format_m,$format_s, $format_t
.
   print "\n   $disc_info{title}\n   $disc_info{artist}\n\n";
   for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $format_i = $track + 1;
      $format_m = $disc_info {length_minutes} [$track];
      $format_s = $disc_info {length_seconds} [$track];
      $format_t = $disc_info {track_title} [$track];
      write;
   }

}



sub get_cddb_entry {

   
   my ($id, %disc_info, $track, $query, $response, %cddb_entry, $i);
   my ($remote, $port, $iaddr, $paddr, $proto, $line);
   $remote = shift;
   $port = shift();

   unless ($quiet == 1) {
      print "Contacting cddb server $remote at port $port...\n";
   }

   %disc_info = %{ shift() };
   $id = $disc_info {id};

   # Generate cddb query string
   $query = "$id $disc_info{tracks} ";
   for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $query .= "$disc_info{frame_offset}[$track] ";
   }
   $query .= "$disc_info{length} \n";
   
   if ($port =~ /\D/) {$port = getservbyname($port, 'tcp') }
   die "no port" unless $port;
   $iaddr = inet_aton($remote); 
   $paddr = sockaddr_in($port, $iaddr);
   $proto = getprotobyname('tcp');
   socket(SERVER, PF_INET, SOCK_STREAM, $proto); 
   unless ( connect(SERVER, $paddr) ){
      print "   Couldn't connect.\n" unless ($quiet == 1);
      return (0);
   }
   select (SERVER); $| = 1;  # Disable buffering
   select (STDOUT); $| = 1;


   $response = <SERVER>;
   if ($response =~ /^200/) {
      print "   Connected for read/write.\n" unless ($quiet == 1);
   } elsif ($response =~ /^201/) {
      print "   Connected for read only.\n" unless ($quiet == 1);
   } elsif ($response =~ /^432/) {
      print "   Permission denied.\n" unless ($quiet == 1);
      close (SERVER);
      return ();
   } elsif ($response =~ /^433/) {
      print "   Too many users.\n" unless ($quiet == 1);
      close (SERVER);
      return ();
   } elsif ($response =~ /^434/) {
      print "   Remote system too busy.\n" unless ($quiet == 1);
      close (SERVER);
      return ();
   } else {
      print "   Unknown response (follows).\n" unless ($quiet == 1);
      print $response unless ($quiet == 1);
      close (SERVER);
      return ();
   }

   print SERVER "cddb hello $user $host jbl 0.1\n";
   $response = <SERVER>;
   print "   $response";
   unless ($response =~ /^2/) {
      print "   Couldn't complete handshake\n";
      close (SERVER);
      return ();
   }

   print SERVER "cddb query $query";
   $match = 0;
   $response = <SERVER>;
   if ($response =~ /^200 ([^ ]*) ([^ ]*) (.*)/) {  # single match
      $cddb_entry {category} [$match] = $1;
      $cddb_entry {disc_id} [$match] = $2;
      $cddb_entry {title} [$match] = $3;
      $match++;
      print "   Single match found.\n" unless ($quiet == 1);
   } elsif ($response =~ /^21[01]/) {  # multiple matches/guesses
      my ($saved) = $response;
MM:   $response = <SERVER>;         
      if ($response =~ /^([^ ]*) ([^ ]*) (.*)/) {
	 $cddb_entry {category} [$match] = $1;
	 $cddb_entry {disc_id} [$match] = $2;
	 $cddb_entry {title} [$match] = $3;
	 $match++;
	 goto MM;
      }
      if ($saved =~ /^211/) {
         print "   Inexact match count = $match\n" unless ($quiet == 1);
      } elsif ($saved =~ /^210/) {
         print "   Exact match count = $match\n" unless ($quiet == 1);
      } else {
         print "   Multiple matches of unknown type (follows):"
	    unless ($quiet == 1);
	 print $saved unless ($quiet == 1);
      }
   } elsif ($response =~ /^202/) {     # no matches
      print "   No matches." unless ($quiet == 1);
      close (SERVER);
      return ();
   } elsif ($response =~ /^403/) {
      print "   Database entry corrupt.\n" unless ($quiet == 1);
      close (SERVER);
      return ();
   } elsif ($response =~ /^409/) {
      print "   No handshake.\n" unless ($quiet == 1);
      close (SERVER);
      return ();
   } elsif ($response =~ /^501/) {  # Not documented!
      print "   Invalid disc ID.\n" unless ($quiet == 1);
      close (SERVER);
      return ();
   } else {
      print "   Unknown server response (follows):\n" 
         unless ($quiet == 1);
      print $response unless ($quiet == 1);
      close (SERVER);
      return ();
   }
   $cddb_entry {count} = $match;

   for ($i=0; $i < $cddb_entry {count}; $i++) {
      print SERVER "cddb read $cddb_entry{category}[$i] ";
      print SERVER "$cddb_entry{disc_id}[$i]\n";
      $response = <SERVER>;
      if ($response =~ /^210/) {
         print "   Downloading entry.\n" unless ($quiet == 1);
      } elsif ($response =~ /^401/) {
         print "   Specified entry not found.\n" unless ($quiet == 1);
	 next;
      } elsif ($response =~ /^403/) {
         print "   Server error.\n" unless ($quiet == 1);
	 next;
      } elsif ($response =~ /^409/) {
         print "   No handshake.\n" unless ($quiet == 1);
	 next;
      } elsif ($response =~ /^417/) {
         print "   Access limit exceeded.\n" unless ($quiet == 1);
	 next;
      } else {
         print "   Unrecognized server response (follows):\n"
	    unless ($quiet == 1);
	 print $response unless ($quiet == 1);
	 next;
      }
RE:   $response = <SERVER>;
      unless ($response =~ /^\./) {
         $cddb_entry {full_entry} [$i] .= $response;
	 goto RE;
      }
   }

   print SERVER "exit\n";
   close (SERVER);

   $cddb_entry {exists} = "true";
   return (%cddb_entry);

}


sub get_disc_id {

   my (%disc_info) = %{ shift() };

   my ($total_digit_sum) = 0;
   for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $total_digit_sum += 
         digit_sum( $disc_info {time_offset} [$track] );
   }


   $id = to_hex (2, $total_digit_sum) .
         to_hex (4, $disc_info {length}) .
	 to_hex (2, $disc_info {tracks});
   print "Computing disc id...   ($id)\n" unless ($quiet == 1);


   return ($id);

}

sub get_track_info {

   my ($track, %info);

   open (TOC, "cdparanoia -Q 2>&1 |"); 
   print "Running cdparanoia -Q to determine disc information.\n" 
      unless ($quiet == 1);

   $track = 0;
   while (<TOC>) {
      if (  /(\d+)[^\d]+\d\d:\d\d\.\d\d.*?(\d+)[^\d]/  )  {
	 $info {frame_offset} [$track] = $2;
	 $info {frame_offset} [$track+1] = $1 + $2;
         $track++;
      }
   }
   close (TOC);

   $info {tracks} = $track;

   for ($track = 0; $track <= $info {tracks}; $track++) {
      $info {time_offset} [$track] = 
         2 + int ($info {frame_offset} [$track] / 75 ); 
   }
   for ($track = 0; $track < $info {tracks}; $track++) {
      $info {length_minutes} [$track] =  
         int( ( $info {time_offset} [$track+1] - 
	        $info {time_offset} [$track] ) / 60 );
      $info {length_seconds} [$track] =  
         int( ( $info {time_offset} [$track+1] - 
	        $info {time_offset} [$track] ) % 60 );
      if ($info {length_seconds} [$track] < 10) {
      $info {length_seconds} [$track] = "0".
         $info {length_seconds} [$track]; 
      }
   }
    
   $info {length} = $info {time_offset} [$info {tracks}]
      - $info {time_offset} [0];
   $info {length_pretty} = int ($info{length} / 60) . ':' .
      $info{length} % 60 ;


   return (%info)

}

sub digit_sum {
     my ($integer,$sum,$j);
     $integer = shift;
     $sum = 0;
     for ($j=0; $j < length($integer); $j++) {
        $sum += substr($integer,$j,1);
     }
     return ($sum);
}

sub to_hex {
   my ($pad, $input, $output, $digit, $i);
   $pad = shift;
   $input = shift;
   $output = '';
   $i = 0;
   while ($input > 0) {
      $digit = (0 .. 9, 'a' .. 'f')[$input % 16];
      $input = int ($input / 16);
      $output = "$digit"."$output";
      $i++;
      last if ($i == $pad);
   }
   while ($i < $pad) {
      $output = "0"."$output";  #pad with zeros
      $i++;
   }
   return ($output);
}

sub external_edit {
   my ($buffer) = shift;
   open (BUF, ">/tmp/tmp_perl_buffer");
   print BUF $buffer;
   close (BUF);
   system ("vi  /tmp/tmp_perl_buffer");
   open (BUF, "</tmp/tmp_perl_buffer");
   $buffer = '';
   while (<BUF>) {
      $buffer .= $_;
   }
   close (BUF);
   unlink ("/tmp/tmp_perl_buffer");
   return ($buffer);
}

sub make_label {

   %disc_info = %{ shift() };
   @args  = @_;
   chop ($year = `date +%Y`);

   unless ($quiet == 1) { 
      open (PS, ">$output_filename") 
	 || die "Couldn't open $output_filename for writing.\n";
      select PS;
      # Otherwise we just go to STDOUT
   } 

# Begin the postscript file
print <<EOF;
%!PS
%%Creator: Jewel Box Labeler v0.1 Copyright (C) 1998 Jacob Langford
%%EndComments

/back_cover_dim { 391 333 } def
/spine_dim { 333 16 } def
/front_cover_dim { 340 340 } def

/rect_path { % x y -> path
   /y exch def /x exch def
   newpath 0 0 moveto x 0 lineto 
   x y lineto 0 y lineto closepath 
} def

/front_cover_path { front_cover_dim rect_path } def
/back_cover_path { back_cover_dim rect_path } def
/spine_path { spine_dim rect_path } def

/rel_text_left { % x y rxl ry family size text -> rxr text
   /text exch def /size exch def /family exch def
   /ry exch def /rxl exch def 
   /y exch def /x exch def
    
   family findfont size scalefont setfont
   text stringwidth pop x div rxl add

   x rxl mul y ry mul moveto
   text
} def

/rel_text_right { % x y rxr ry family size text -> rxl text
   /text exch def /size exch def /family exch def
   /ry exch def /rxr exch def 
   /y exch def /x exch def
    
   family findfont size scalefont setfont
   text stringwidth pop x div neg rxr add dup 
   
   x mul y ry mul moveto
   text
} def

/rel_text_centered { % x y rxl rxr ry family size text -> text
   /text exch def /size exch def /family exch def
   /ry exch def /rxr exch def /rxl exch def
   /y exch def /x exch def

   family findfont size scalefont setfont
   rxr rxl sub 
   text stringwidth pop x div sub 2 div rxl add
   x mul y ry mul moveto
   text
} def

/set_spine_text_left { % rxl ry family size text -> rxr text
   spine_dim 7 2 roll rel_text_left
} def

/set_back_cover_text_left { % rxl ry family size text -> rxr text
   back_cover_dim 7 2 roll rel_text_left
} def

/set_front_cover_text_left { % rxl ry family size text -> rxr text
   front_cover_dim 7 2 roll rel_text_left
} def

/set_spine_text_right { % rxr ry family size text -> rxl text
   spine_dim 7 2 roll rel_text_right
} def

/set_back_cover_text_right { % rxr ry family size text -> rxl text
   back_cover_dim 7 2 roll rel_text_right
} def

/set_front_cover_text_right { % rxr ry family size text -> rxl text
   front_cover_dim 7 2 roll rel_text_right
} def

/set_spine_text_centered { % rxl rxr ry family size text -> text
   spine_dim 8 2 roll rel_text_centered
} def

/set_back_cover_text_centered { % rxl rxr ry family size text -> text
   back_cover_dim 8 2 roll rel_text_centered
} def

/set_front_cover_text_centered { % rxl rxr ry family size text -> text
   front_cover_dim 8 2 roll rel_text_centered
} def


/fill_rectangle { % x y vspace text
   /text exch def /vspace exch def
   /y exch def /x exch def
   0 y moveto 
   {  
      {pop pop currentpoint pop x gt 
         {currentpoint vspace sub exch pop 0 exch moveto} if
      } text kshow
      currentpoint exch pop 0 lt 
   {exit} if } loop
} def

/fill_front_cover { % rxo ryo rxs rys angle family size text 
   /text exch def /size exch def /family exch def /angle exch def
   /rys exch def /rxs exch def /ryo exch def /rxo exch def
   front_cover_dim /y exch def /x exch def

   gsave front_cover_path clip
     x rxo mul neg y ryo mul neg translate angle rotate
     family findfont size scalefont setfont
     x rxs mul y rys mul size text fill_rectangle
   grestore
} def




EOF

   $artist = $disc_info {artist};
   $artist =~ /^([\S]+)\s/;
   $artist1 = $1;
   $artist =~ /\s([\S]+)$/;
   $artist2 = $1;
   $title = $disc_info {title};
   $length = $disc_info {length_pretty};

# Print the front cover and start the back cover
print <<EOF;

% Set coordinates for front cover
   100 375 translate
   gsave front_cover_path clip
   .90 setgray
   0 0.5 1.3 1.5 15 /Times-Bold 65 
      ($artist  ) fill_front_cover
   0.0 setgray front_cover_path 2 setlinewidth stroke
   0 1 0.65 /Helvetica-Bold 30
      ($artist) set_front_cover_text_centered show
   0 1 0.55 /Helvetica-Bold 20
      ($title) set_front_cover_text_centered show

   grestore



% Set coordinates for back cover
   0 -350 translate
   gsave back_cover_path clip
   
   0.80 setgray

   0.02 0.50 /Helvetica-Bold 200
      ($artist1)
      set_back_cover_text_left show pop

   0.98 0.05 /Helvetica-Bold 200
      ($artist2)
      set_back_cover_text_right show pop

   0.0 setgray

   0.05 0.06 /Helvetica-Bold 08
      (Proudly labeled with free, open-source software, 1998.) 
      set_back_cover_text_left show pop
   0.05 0.04 /Helvetica-Bold 08
      (Distribution of disc contents may be restricted by copyright law.) 
      set_back_cover_text_left show pop
   0.95 0.04 /Helvetica-Bold 08
      (Total running time $length.) 
      set_back_cover_text_right show pop

   %draw_back_frame
   back_cover_path 2 setlinewidth stroke

EOF

# Sixteen-and-a-half tracks fit nicely with no shrinkage
$shrinkage =  16.5 / $disc_info {tracks};
$shrinkage = 1 if ($shrinkage > 1);
$trackfontsize = 14 * $shrinkage;
$timefontsize = 8 * $shrinkage;

for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $track_number = $track + 1;
      $track_time = $disc_info {length_minutes} [$track]
          .':'.$disc_info {length_seconds} [$track];
      $track_title = $disc_info {track_title} [$track];
      $vlocation = 0.90 - $track * 0.05 * $shrinkage;

      print <<EOF

   0.15 $vlocation /Helvetica $trackfontsize
      ($track_number.) set_back_cover_text_right show pop
   0.18 $vlocation /Helvetica $trackfontsize
      ($track_title) set_back_cover_text_left show
   0.015 add $vlocation /Courier $timefontsize
      (\($track_time\)) set_back_cover_text_left show pop

EOF
}

# Finish up
print <<EOF;

% finished with back cover
   grestore

/draw_spine {
   spine_path 2 setlinewidth stroke

   .05 0.25 /Helvetica-Bold 12 
      ($artist) set_spine_text_left show
   0.95 0.35 /Helvetica 08 
      ($disc_info{id}) set_spine_text_right show
   % First two arguments left by last two calls
   0.25 /Helvetica 10 
      ($title) set_spine_text_centered show
} def

% Draw both spines
   90 rotate gsave spine_path clip draw_spine grestore

   -180 rotate back_cover_dim neg exch translate
   gsave spine_path clip draw_spine grestore

showpage

EOF

   unless ($quiet == 1) { 
      close (PS);
   } 
   
}

sub empty_cddb_entry {

   my (%disc_info) = %{ shift() };
   my ($entry) = '';
   my ($track);
   $entry .= "# xmcd\n#\n# comments, comments, yah de dah de day\n#\n";
   $entry .= "DISCID=$disc_info{id}\nDTITLE=\n";
   for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $entry .= "TTITLE${track}=\n"
   }
   $entry .= "EXTD=\n";
   for ($track = 0; $track < $disc_info {tracks}; $track++) {
      $entry .= "EXTT${track}=\n"
   }
   $entry .= "PLAYORDER=\n";

   return ($entry);

}

