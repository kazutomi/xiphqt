#!/usr/bin/perl

use Socket;
use Sys::Hostname;
use Time::Local;
use IPC::Open3;
use File::Glob ':glob';
use Tk;
use Tk::Xrm;
use Tk qw(exit); 

my $HOME=$ENV{"HOME"};
if(!defined($HOME)){
    print "HOME environment variable not set.  Exiting.\n";
    exit 1;
}

$version="Snatch 20011106";
$configdir=$HOME."/.snatch";
$configfile=$configdir."/config.txt";
$historyfile=$configdir."/history.txt";
$logofile=$configdir."/logo.xpm";
$libsnatch="/home/xiphmont/snatch/libsnatch.so";

my $backchannel_socket="/tmp/snatch.$PID";
my $uaddr=sockaddr_un($backchannel_socket);
my $proto=getprotobyname('tcp');
my $comm_ready=0;

# default config
$CONFIG{'REALPLAYER'}='{realplay,~/RealPlayer8/realplay}';
$CONFIG{'OUPUT_PATH'}="$HOME";
$CONFIG{'AUDIO_DEVICE'}="/dev/dsp*";
$CONFIG{'AUDIO_MUTE'}='no';
$CONFIG{'VIDEO_MUTE'}='no';
$CONFIG{'MODE'}='active';
$CONFIG{'OUTPUT'}=$HOME;
$CONFIG{'DEBUG'}='no';

if(! -e $configdir){
    die $! unless mkdir $configdir, 0770;
}



$snatchxpm= <<'EOF';
/* XPM */
static char * snatch_xpm[] = {
"36 27 25 1",
" 	c None",
".	c #060405",
"+	c #8A8787",
"@	c #8D1D27",
"#	c #515052",
"S	c #4F1314",
"%	c #69272B",
"&	c #AE5664",
"*	c #D73138",
"=	c #252524",
"-	c #761922",
";	c #2D0505",
">	c #A53149",
",	c #C8C8C8",
"'	c #676768",
")	c #F3F3F3",
"!	c #A9A9A9",
"~	c #7C5254",
"{	c #F62F36",
"]	c #9A9A99",
"^	c #B32225",
"/	c #261E24",
"(	c #373736",
"_	c #D7D7D7",
":	c #797978",
"   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   ",
" %S%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% ",
"%->%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%>-%",
"S>;S----------------------------;@@%",
"S>;*{{{{{{{{{{{******{{*{{{{{{{{-@>%",
"S>;*{{{*^@--SSSSSSSSSSSSSS---@^{-->%",
"S>;*{{-......................(.SS@>%",
"S>;^{^...............(!/....:)..;->%",
"S>;^{S..+!]=++!:.:!]#,)#'!]=!)!]/->%",
"S>;^{;.:)'_+)]!)'_')+_,+)'_]_,:)#->%",
"S>;@*..#))]')=!_#,,)#)],,./()'')/->%",
"S>;@*;/:#])!).,,_!+)#)'_,(,:)(]_.->%",
"S>;S@S.]__:++.!'!_!!(_]'__:#!.++.->%",
"S>;-{^;........................S;->%",
"S>;S{{{-......................@*;->%",
"S>;S{{{{^;..................;^{*;->%",
"S>;S{{{{{*S................S{{{^.->%",
"S>;S{{{{{{{@..............@{{{{^.->%",
"S>;S{{{{{{{{^;..........;^{{{{{^.->%",
"S>;;{{{{{{{{{*S........-*{{{{{{^.->%",
"S>;;***********^-S..S-^********@.->%",
"S>;.;;;;;;;;;;;;;;;;;;;;;;;;;;;;.->%",
"S>;..............................->%",
"S>;..............................@&~",
"%-@%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%>~+",
" %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%S% ",
"   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%   "};
EOF

if(! -e $logofile){
    die $! unless open LFILE, ">$logofile";
    print LFILE $snatchxpm;
    close LFILE;
}

# load the config/history
if(-e $configfile){
    die $! unless open CFILE, $configfile;
    while(<CFILE>){
	/^\s*([^=]+)=([^\n]*)/;
	$CONFIG{$1}=$2;
    }
    close CFILE;
}

if(-e $historyfile){
    die $! unless open HFILE, $historyfile;
    while(<HFILE>){
	push @TIMER, $_;
    }
    close HFILE;
}

print @ARGV;

# build the UI
my $toplevel=new MainWindow(-class=>'Snatch');
my $Xname=$toplevel->Class;

$toplevel->optionAdd("$Xname.background",  "#8e3740",20);
$toplevel->optionAdd("$Xname.Panel.background",  "#8e3740",20);
$toplevel->optionAdd("$Xname.Panel.foreground",  "#d0d0d0",20);
$toplevel->optionAdd("$Xname.Panel.font",
                     '-*-helvetica-bold-o-*-*-18-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Statuslabel.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Statuslabel.foreground", "#606060");
$toplevel->optionAdd("$Xname*Status.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);

$toplevel->optionAdd("$Xname*AlertDetail.font",
                     '-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*',20);


$toplevel->optionAdd("$Xname*background",  "#d0d0d0",20);
$toplevel->optionAdd("$Xname*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*Tab*background",  "#a0a0a0",20);
$toplevel->optionAdd("$Xname*Button*background",  "#f0d0b0",20);
$toplevel->optionAdd("$Xname*Button*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*activeBackground",  "#f0f0ff",20);
$toplevel->optionAdd("$Xname*activeForeground",  '#0000a0',20);
$toplevel->optionAdd("$Xname*borderWidth",         0,20);
$toplevel->optionAdd("$Xname*activeBorderWidth",         1,20);
$toplevel->optionAdd("$Xname*highlightThickness",         0,20);
$toplevel->optionAdd("$Xname*padX",         2,20);
$toplevel->optionAdd("$Xname*padY",         2,20);
$toplevel->optionAdd("$Xname*relief",         'flat',20);
$toplevel->optionAdd("$Xname*font",    
                     '-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*quit.font",    
                     '-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*',20);

$toplevel->configure(-background=>$toplevel->optionGet("background",""));

#$toplevel->resizable(FALSE,FALSE);
my $xpm_snatch=$toplevel->Pixmap("_snatchlogo_xpm",-file=>$logofile);

$window_shell=$toplevel->Label(Name=>"shell",borderwidth=>1,relief=>raised)->
    place(-x=>10,-y=>36,-relwidth=>1.0,-relheight=>1.0,
	  -width=>-20,-height=>-46,-anchor=>'nw');

$window_setupbar=$toplevel->Button(-class=>Tab,Name=>"setup",text=>"configuration",
				   borderwidth=>1,relief=>raised)->
    place(-relx=>1.0,-anchor=>'se',-in=>$window_shell,-bordermode=>outside);
$window_timerbar=$toplevel->Button(-class=>Tab,Name=>"timer",text=>"timer setup",
				   borderwidth=>1,relief=>raised)->
    place(-bordermode=>outside,-anchor=>'ne',-in=>$window_setupbar);

$window_quit=$window_shell->Button(-class=>Tab,Name=>"quit",text=>"quit",
				   -padx=>1,-pady=>1,
				   relief=>'groove',borderwidth=>2)->
    place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');

$window_logo=$toplevel->
    Label(Name=>"logo",-class=>"Panel",image=>$xpm_snatch,
	  relief=>flat,borderwidth=>0)->
    place(-x=>5,-y=>5,-anchor=>'nw');

$window_version=$toplevel->
    Label(Name=>"logo text",-class=>"Panel",text=>$version,
	  relief=>flat,borderwidth=>0)->
    place(-x=>5,-relx=>1.0,-rely=>1.0,-anchor=>'sw',-in=>$window_logo);


$window_statuslabel=$window_shell->
    Label(Name=>"statuslabel",-class=>"Statuslabel",text=>"Status: ",
	  relief=>flat,borderwidth=>0)->
    place(-x=>5,-y=>0,-rely=>.2,-relheight=>.4,-anchor=>'w');

$window_status=$window_shell->
    Label(Name=>"status",-class=>"Status",text=>"Starting...",
	  relief=>flat,borderwidth=>0)->
    place(-x=>5,-y=>0,-relx=>1.0,-relheight=>1.0,
	  -anchor=>'nw',-in=>$window_statuslabel);

$window_active=$window_shell->Button(Name=>"active",text=>"record all",
				    state=>disabled,
				   relief=>'groove',borderwidth=>2)->
    place(-x=>5,-y=>0,-relx=>0.,-rely=>.55,-relwidth=>.33,
	  -width=>-5,-anchor=>'w',-in=>$window_shell);

$window_timer=$window_shell->Button(Name=>"timer",text=>"timed record",
				    state=>disabled,
				   relief=>'groove',borderwidth=>2)->
    place(-x=>0,-y=>0,-relx=>.333,-rely=>.55,-relwidth=>.33,
	  -width=>-0,-anchor=>'w',-in=>$window_shell);

$window_inactive=$window_shell->Button(Name=>"inactive",text=>"record off",
				    state=>disabled,
				   relief=>'groove',borderwidth=>2)->
    place(-x=>0,-y=>0,-relx=>.667,-rely=>.55,-relwidth=>.33,
	  -width=>-5,-anchor=>'w',-in=>$window_shell);


$window_mute=$window_shell->Label(Name=>"mute",text=>"mutes: ")->
    place(-x=>5,-y=>0,-relx=>0.,-rely=>.85,
	  -anchor=>'w',-in=>$window_shell);

$window_amute=$window_shell->Button(Name=>"audio",text=>"audio",
				    state=>disabled,
				   relief=>'groove',borderwidth=>2)->
    place(-x=>2,-relx=>1.0,-relheight=>1.0,-anchor=>'nw',-in=>$window_mute,
	  -bordermode=>outside);

$window_vmute=$window_shell->Button(Name=>"video",text=>"video",
				    state=>disabled,
				    relief=>'groove',borderwidth=>2)->
    place(-x=>2,-relx=>1.0,-relheight=>1.0,-anchor=>'nw',-in=>$window_amute,
	  -bordermode=>outside);

$minwidth=
    $window_logo->reqwidth()+
    $window_version->reqwidth()+
    $window_setupbar->reqwidth()+
    $window_timerbar->reqwidth()+
    30;
$minheight=
    $window_logo->reqheight()+
    $window_statuslabel->reqheight()+
    $window_mute->reqwidth()+
    20;

$toplevel->optionAdd("$Xname.geometry",    ($minwidth+20).'x'.$minheight,20);
my$geometry=$toplevel->optionGet("geometry","");
$toplevel->minsize($minwidth,$minheight);
$toplevel->geometry($geometry);

$toplevel->update();

$window_quit->configure(-command=>[sub{Shutdown();}]);
$window_amute->configure(-command=>[sub{Amute();}]);
$window_vmute->configure(-command=>[sub{Vmute();}]);
$window_active->configure(-command=>[sub{Robot_Active();}]);
$window_timer->configure(-command=>[sub{Robot_Timer();}]);
$window_inactive->configure(-command=>[sub{Robot_Inactive();}]);
$window_setupbar->configure(-command=>[sub{Setup();}]);
$window_timerbar->configure(-command=>[sub{Timer();}]);

# bind socket
BindSocket();

# throw a realplayer process and 
ThrowRealPlayer();

# main loop 
Tk::MainLoop();


sub ThrowRealPlayer{
    $SIG{CHLD}='IGNORE';

    Status("Starting RealPlayer...");
    # set up the environment

    $ENV{"SNATCH_DEBUG"}=1;
    $ENV{"LD_PRELOAD"}=$libsnatch;
    $ENV{"SNATCH_COMM_SOCKET"}=$backchannel_socket;

    my@list=bsd_glob("$CONFIG{'REALPLAYER'}",
		     GLOB_TILDE|GLOB_ERR|GLOB_BRACE);
    if(GLOB_ERROR || $#list<0){
	Status("Failed to find RealPlayer!");
	return;
    }

    die "pipe call failed unexpectedly: $!" unless pipe REAL_STDERR,WRITEH;
    $realpid=open3("STDIN",">&STDOUT",">&WRITEH",@list[0]);
    close WRITEH;

    # a select loop until we have the socket accepted
    my $rin = $win = $ein = '';
    my $rout,$wout,$eout;
    vec($rin,fileno(REAL_STDERR),1)=1;
    vec($rin,fileno(LISTEN_SOCK),1)=1;
    $ein=$rin | $win;

    my $time=20;
    my $stderr_output;

    Status("Waiting for rendevous... [$time]");
    while($time){
	my($nfound,$timeleft)=select($rout=$rin, $wout=$win, $eout=$ein, 1);
	if($nfound==0){
	    $time--;
	    Status("Waiting for rendevous... [$time]");
	}else{
	    $toplevel->update();
	    if(vec($rout,fileno(REAL_STDERR),1)){
		$bytes=sysread REAL_STDERR, my$scalar, 4096;
		$stderr_output.=$scalar;
		
		if($bytes==0){
		    Status("Rendevous failed.");

		    Alert("RealPlayer didn't start successfully.  ".
			  "Here's the complete debugging output of the ".
			  "attempt:",
			  $stderr_output);

		    return;
		}
	    }
	    if(vec($rout,fileno(LISTEN_SOCK),1)){
		# socket has a request
		$status=accept(COMM_SOCK,LISTEN_SOCK);
		$comm_ready=1;
		$time=0;
	    }
	}
    }


    # configure
    Status("RealPlayer started...");
    $window_active->configure(state=>normal);
    $window_timer->configure(state=>normal);
    $window_inactive->configure(state=>normal);

    $window_amute->configure(state=>normal);
    $window_vmute->configure(state=>normal);
    $toplevel->fileevent(REAL_STDERR,'readable'=>[sub{ReadStderr();}]);
    send_string("F",$CONFIG{'OUTPUT_PATH'});
    send_string("D",$CONFIG{'AUDIO_DEVICE'});
    Robot_Active();

}

sub BindSocket{
    Status("Binding socket..");
    die $! unless socket(LISTEN_SOCK, PF_UNIX, SOCK_STREAM,0);
    unlink($backchannel_socket);
    die $! unless bind(LISTEN_SOCK,$uaddr);
    die $! unless listen(LISTEN_SOCK,SOMAXCONN);
}

sub ReleaseSocket{
    $window_amute->configure(state=>disabled);
    $window_vmute->configure(state=>disabled);

    $comm_ready=0;
    unlink($backchannel_socket);
    close(LISTEN_SOCK);
}

sub AcceptSocket{
    Status("Waiting for rendevous...");

    eval{
	local $SIG{ALRM} = sub { Status("Failed to rendevous"); };
	alarm 15;
	$status=accept(COMM_SOCK,LISTEN_SOCK);
	alarm 0;
	if($status){
	    #enable the panel
	    Status("libsnatch contacted");
	    $comm_ready=1;
	    $window_amute->configure(state=>active);
	    $window_vmute->configure(state=>active);

	    
	}
    }
}

sub Shutdown{
# save the config/history
    Robot_Exit();
    ReleaseSocket();

    die $! unless open CFILE, ">$configfile".".tmp";
    foreach $key (keys %CONFIG){
	print CFILE "$key=$CONFIG{$key}\n";
    }
    close CFILE;
    die $! unless rename "$configfile".".tmp", $configfile;
    
    die $! unless open HFILE, ">$historyfile".".tmp";
    foreach $line (@TIMER){
	print HFILE "$line\n";
    }
    close HFILE;
    die $! unless rename "$historyfile".".tmp", $historyfile;
    
  Tk::exit(0);
}


sub send_string{
    my($op,$code)=@_;
    syswrite COMM_SOCK,$op;
    syswrite COMM_SOCK, (pack 'S', length $code);
    syswrite COMM_SOCK, $code;
}

sub Robot_PlayLoc{
    my($loc,$username,$password)=@_;
    my $stopcode=join "",("Ks",pack ("S",4));
    my $loccode=join "",("Kl",pack ("S",4));
    
    syswrite COMM_SOCK,$stopcode;

    send_string("P",$password);
    send_string("U",$username);
    send_string("L",$openloc);

    syswrite COMM_SOCK,$loccode;
 
    # watch for bad password
}

sub Robot_PlayFile{
    my($loc)=@_;
    my $stopcode=join "",("Ks",pack ("S",4));
    my $opencode=join "",("Ko",pack ("S",4));

    syswrite COMM_SOCK,$stopcode;
    send_string("O",$openfile);
    syswrite COMM_SOCK,$opencode;
}

sub Robot_Stop{
    my $stopcode=join "",("Ks",pack ("S",4));
    syswrite COMM_SOCK,$playcode;
}

sub Robot_Exit{
    my $exitcode=join "",("Kx",pack ("S",4));
    syswrite COMM_SOCK,$playcode;
    close COMM_SOCK;
}

sub Robot_Active{
    # clear out robot settings to avoid hopelessly confusing the user
    send_string("U","");
    send_string("P","");
    send_string("O","");
    send_string("L","");
    syswrite COMM_SOCK,'A';
    Robot_Audio($global_audio_setting);
    Robot_Video($global_video_setting);
    Status("Active/Nominal");
}

sub Robot_Inactive{
    send_string("U","");
    send_string("P","");
    send_string("O","");
    send_string("L","");
    Robot_Audio($global_audio_setting);
    Robot_Video($global_video_setting);
    syswrite COMM_SOCK,'I';
    Status("Recording off");
}

sub Robot_Timer{
    send_string("U","");
    send_string("P","");
    send_string("O","");
    send_string("L","");
    Robot_Audio($global_audio_setting);
    Robot_Video($global_video_setting);
    syswrite COMM_SOCK,'T';
    Status("Inactive [Timer]");
}

sub Robot_Audio{
    my($onoff)=@_;

    if($onoff=~m/on/){
	syswrite COMM_SOCK,'S';
    }
    if($onoff=~m/off/){
	syswrite COMM_SOCK,'s';
    }
}

sub Robot_Video{
    my($onoff)=@_;

    if($onoff=~m/on/){
	syswrite COMM_SOCK,'V';
    }
    if($onoff=~m/off/){
	syswrite COMM_SOCK,'v';
    }
}

sub SplitTimerEntry{
    my($line)=@_;

# eg
# 2001 11 05 1 12:25 300000 FAKEA FAKEV length:USERNAME length:PASSWORD length:FILE length:URL
    $line=~/^\s*(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\d+):(\d+)\s+(\d+)\s+(\S+)\s+(\S+)\s+(.*)/;
    my $year=$1;
    my $month=$2;
    my $day=$3;
    my $dayofweek=$4;
    my $hour=$5;
    my $minute=$6;
    my $duration=$7;

    my $audio=$8;
    my $video=$9;

    my $fields=$10;
    
    my $username;
    my $password;
    my $outfile;
    my $url;

    ($username,$fields)=LengthParse($fields);
    ($password,$fields)=LengthParse($fields);
    ($outfile,$fields)=LengthParse($fields);
    ($url,$fields)=LengthParse($fields);

    ($year,$month,$day,$dayofweek,$hour,$minute,$duration,$audio,$video,$username,
     $password,$outfile,$url);
}

sub LengthParse{
    my($line)=@_;
    $line=~/(\d+):(.+)/;
    $length=$1;
    $rest=$2;

    (substr($rest,0,$length),substr($rest,$length));
}	

sub MonthDays{
    my($month,$year)=@_;
    
    if($month==2){
	if($year % 4 !=0){
	    28;
	}elsif ($year % 400 == 0){
	    29;
	}elsif ($year % 100 == 0){
	    28;
	}else{
	    29;
	}
    }else{
	my @trans=(0,31,0,31,30,31,30,31,31,30,31,30,31);
	$trans[$month];
    }
}

sub TimerWhen{
    my($try,$year,$month,$day,$dayofweek,$hour,$minute,$duration)=@_;

    #overguard 
    if($minute ne '*'){while($minute>=60){$minute-=60;
					  $hour++if($hour ne '*');}};
    if($hour ne '*'){while($hour>=24){$hour-=24;
				      $day++ if($day ne '*');}};
    if($day ne '*' && $month ne '*' && $year ne '*'){
	while($month>12){$month-=12;$year++;};
	while($day>MonthDays($month,$year)){
	    $day-=MonthDays($month,$year);$month++;
	    while($month>12){$month-=12;$year++;};
	}
    }
    if($month ne '*'){while($month>12){$month-=12;
				       $year++ if ($year ne '*')}};
    
    my $now=time();
    my($nowsec,$nowmin,$nowhour,$nowday,$nowmonth,$nowyear)=localtime($now);    
    $nowmon++;
    $nowyear+=1900;

    
    # boundary cases in each...  rather than solving it exactly, we'll
    # solve it empirically. Laziness as a virtue.
    if($year eq '*'){
	$try=TimerWhen($try,$nowyear-1,$month,$day,$dayofweek,
		       $hour,$minute,$duration);
	return $try if($try!=-1);
	$try=TimerWhen($try,$nowyear,$month,$day,$dayofweek,
		       $hour,$minute,$duration);
	return $try if($try!=-1);
	$try=TimerWhen($try,$nowyear+1,$month,$day,$dayofweek,
		       $hour,$minute,$duration);
	return $try if($try!=-1);
    }elsif($month eq '*'){
	for(my$i=1;$i<13;$i++){
	    $try=TimerWhen($try,$year,$i,$day,$dayofweek,
			   $hour,$minute,$duration);
	    return $try if($try!=-1);
	}
    }elsif($day eq '*'){
	# important to go for a weekday match */
	for(my$i=1;$i<32;$i++){
	    $try=TimerWhen($try,$year,$month,$i,$dayofweek,
			   $hour,$minute,$duration);
	    return $try if($try!=-1);
	}
    }elsif($hour eq "*"){
	return $try;
    }elsif($hour eq "*"){
	return $try;
    }elsif($duration eq "*"){
	return $try;
    }else{
	my $start=timelocal(0,$minute,$hour,$day,$month-1,$year);
	my $end=$start+$duration;
	
	# make sure day-of-month and day-of-week agree
	if($dayofweek ne '*'){
	    my($tsec,$tmin,$thour,$tday,$tmon,$tyear,$twday)=localtime($start);
	    if($twday != $dayofweek){return $try};
	}
	
	if($start>$now || $end>$now){
	    if($try==-1 || $start<$try){
		return $start;
	    }
	}
    }
    $try;
}

sub max{
    my$val=shift @_;
    while (defined(my$n=shift @_)){
	$val=$n if($n>$val);
    }
    $val;
}

sub sortsub{
    my($a,$b)=@_;
    return $TIMER_TIMES[$a]-$TIMER_TIMES[$b];
}

sub TimerSort{
    $count=0;
    @TIMER_TIMES=(map {TimerWhen(-1,(SplitTimerEntry($_)))} @TIMER);
    @TIMER_SORTED=sort sortsub, (map {$count++} @TIMER);
}    

sub Status{
    $window_status->configure(text=>shift @_);
    $toplevel->update();
}

sub Alert{
    my($message,$detail)=@_;

    if(defined($modal)){$modal->destroy()};

    $modal=new MainWindow(-class=>"$Xname");
    $modal->configure(-background=>$modal->optionGet("background",""));
    
    $modal_shell=$modal->Label(-class=>Alert,Name=>"shell",
			       borderwidth=>1,relief=>raised)->
				   place(-x=>4,-y=>4,-relwidth=>1.0,-relheight=>1.0,
					 -width=>-8,-height=>-8,-anchor=>'nw');

    $modal_exit=$modal_shell->
	Button(-class=>Tab,Name=>"exit",text=>"X",
	       -padx=>1,-pady=>1,relief=>'groove',borderwidth=>2)->
		   place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');
    
    $modal_message=$modal_shell->
	Label(text=>$message,-class=>"AlertText")->
	    place(-x=>5,-y=>10);

    $width=$modal_message->reqwidth();


    $modal_detail=$modal_shell->
	Message(text=>$detail,-class=>"AlertDetail",
		-width=>($width-$modal_exit->reqwidth()))->
	    place(-relx=>0,-y=>5,-rely=>1.0,-anchor=>'nw',
		  -in=>$modal_message);

    $width+=20;
    $height=$modal_message->reqheight()+$modal_detail->reqheight()+25;

    my$xx=$toplevel->rootx();
    my$yy=$toplevel->rooty();
    my$ww=$toplevel->width();
    my$hh=$toplevel->height();

    $x=$xx+$ww/2-$width/2;
    $y=$yy+$hh/2-$height/2;
    
    $modal->geometry($width."x".$height."+".int($x)."+".int($y));
    $modal->resizable(FALSE,FALSE);
    $modal->transient($toplevel);
    $modal_exit->configure(-command=>[sub{$modal->destroy();undef $modal}]);
}

sub ReadStderr{
    $bytes=sysread REAL_STDERR, my$scalar, 4096;
    if($bytes==0){
	Alert("RealPlayer unexpectedly exited!","Attempting to start a new copy...\n");
	$toplevel->fileevent(REAL_STDERR,'readable' => ''); 
	close REAL_STDERR;
	ThrowRealPlayer();
    }

    if($scalar=~/shut down X/){
	$toplevel->fileevent(REAL_STDERR,'readable' => ''); 
	close REAL_STDERR;
	close COMM_SOCK;
      Tk::exit(0);
    }	
    print $scalar;
}




