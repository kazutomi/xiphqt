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

my $backchannel_socket="/tmp/snatch.$PID";
my $uaddr=sockaddr_un($backchannel_socket);
my $proto=getprotobyname('tcp');
my $comm_ready=0;
my $mode='active';

# default config
$CONFIG{'REALPLAYER'}='{realplay,~/RealPlayer8/realplay}';
$CONFIG{'LIBSNATCH'}='{~/snatch/libsnatch.so}';
$CONFIG{'OUTPUT_PATH'}=$HOME;
$CONFIG{'AUDIO_DEVICE'}="/dev/dsp*";
$CONFIG{'AUDIO_MUTE'}='no';
$CONFIG{'VIDEO_MUTE'}='no';
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
$toplevel->optionAdd("$Xname*Tab*disabledForeground",  "#ffffff",20);
$toplevel->optionAdd("$Xname*Tab*relief",  "raised",20);
$toplevel->optionAdd("$Xname*Tab*borderWidth",  1,20);

$toplevel->optionAdd("$Xname*Button*background",  "#f0d0b0",20);
$toplevel->optionAdd("$Xname*Button*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*Button*borderWidth",  '2',20);
$toplevel->optionAdd("$Xname*Button*relief",  'groove',20);

$toplevel->optionAdd("$Xname*activeBackground",  "#ffffff",20);
$toplevel->optionAdd("$Xname*activeForeground",  '#0000a0',20);
$toplevel->optionAdd("$Xname*borderWidth",         0,20);
$toplevel->optionAdd("$Xname*relief",         'flat',20);
$toplevel->optionAdd("$Xname*activeBorderWidth",         1,20);
$toplevel->optionAdd("$Xname*highlightThickness",         0,20);
$toplevel->optionAdd("$Xname*padX",         2,20);
$toplevel->optionAdd("$Xname*padY",         2,20);
$toplevel->optionAdd("$Xname*font",    
                     '-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Entry.font",    
                     '-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*',20);

$toplevel->optionAdd("$Xname*Exit.font",    
                     '-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Exit.relief",          'groove',20);
$toplevel->optionAdd("$Xname*Exit.padX",          1,20);
$toplevel->optionAdd("$Xname*Exit.padY",          1,20);
$toplevel->optionAdd("$Xname*Exit.borderWidth",          2,20);
$toplevel->optionAdd("$Xname*Exit*background",  "#a0a0a0",20);
$toplevel->optionAdd("$Xname*Exit*disabledForeground",  "#ffffff",20);

$toplevel->optionAdd("$Xname*Entry.background",  "#ffffff",20);
$toplevel->optionAdd("$Xname*Entry.disabledForeground",  "#c0c0c0",20);
$toplevel->optionAdd("$Xname*Entry.relief",  "sunken",20);
$toplevel->optionAdd("$Xname*Entry.borderWidth",  2,20);

$toplevel->optionAdd("$Xname*ListFrame.background",  "#ffffff",20);
$toplevel->optionAdd("$Xname*ListRowOdd.background",  "#dfffe7",20);
$toplevel->optionAdd("$Xname*ListRowEven.background",  "#ffffff",20);

$toplevel->optionAdd("$Xname*Scrollbar*background",  "#f0d0b0",20);
$toplevel->optionAdd("$Xname*Scrollbar*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*Scrollbar*borderWidth",  '2',20);
$toplevel->optionAdd("$Xname*Scrollbar*relief",  'sunken',20);



$toplevel->configure(-background=>$toplevel->optionGet("background",""));

#$toplevel->resizable(FALSE,FALSE);
my $xpm_snatch=$toplevel->Pixmap("_snatchlogo_xpm",-file=>$logofile);

$window_shell=$toplevel->Label(Name=>"shell",borderwidth=>1,relief=>raised)->
    place(-x=>10,-y=>36,-relwidth=>1.0,-relheight=>1.0,
	  -width=>-20,-height=>-46,-anchor=>'nw');

$window_setupbar=$toplevel->Button(-class=>Tab,Name=>"setup",text=>"configuration")->
    place(-relx=>1.0,-anchor=>'se',-in=>$window_shell,-bordermode=>outside);
$window_timerbar=$toplevel->Button(-class=>Tab,Name=>"timer",text=>"timer setup")->
    place(-bordermode=>outside,-anchor=>'ne',-in=>$window_setupbar);

$window_quit=$window_shell->Button(-class=>"Exit",text=>"quit")->
    place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');

$window_logo=$toplevel->
    Label(Name=>"logo",-class=>"Panel",image=>$xpm_snatch)->
    place(-x=>5,-y=>5,-anchor=>'nw');

$window_version=$toplevel->
    Label(Name=>"logo text",-class=>"Panel",text=>$version)->
    place(-x=>5,-relx=>1.0,-rely=>1.0,-anchor=>'sw',-in=>$window_logo);


$window_statuslabel=$window_shell->
    Label(Name=>"statuslabel",-class=>"Statuslabel",text=>"Status: ")->
    place(-x=>5,-y=>0,-rely=>.2,-relheight=>.4,-anchor=>'w');

$window_status=$window_shell->
    Label(Name=>"status",-class=>"Status",text=>"Starting...")->
    place(-x=>5,-y=>0,-relx=>1.0,-relheight=>1.0,
	  -anchor=>'nw',-in=>$window_statuslabel);

$window_active=$window_shell->Button(Name=>"active",text=>"capture all",
				    state=>disabled)->
    place(-x=>5,-y=>0,-relx=>0.,-rely=>.55,-relwidth=>.33,
	  -width=>-5,-anchor=>'w',-in=>$window_shell);

$window_timer=$window_shell->Button(Name=>"timer",text=>"timed record",
				    state=>disabled)->
    place(-x=>0,-y=>0,-relx=>.333,-rely=>.55,-relwidth=>.33,
	  -width=>-0,-anchor=>'w',-in=>$window_shell);

$window_inactive=$window_shell->Button(Name=>"inactive",text=>"off",
				    state=>disabled)->
    place(-x=>0,-y=>0,-relx=>.667,-rely=>.55,-relwidth=>.33,
	  -width=>-5,-anchor=>'w',-in=>$window_shell);


$window_mute=$window_shell->Label(Name=>"mute",text=>"silent capture: ")->
    place(-x=>5,-y=>0,-relx=>0.,-rely=>.85,
	  -anchor=>'w',-in=>$window_shell);

$window_amute=$window_shell->Button(Name=>"audio",text=>"audio",
				    state=>disabled)->
    place(-x=>2,-relx=>1.0,-relheight=>1.0,-anchor=>'nw',-in=>$window_mute,
	  -bordermode=>outside);

$window_vmute=$window_shell->Button(Name=>"video",text=>"video",
				    state=>disabled)->
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
    $window_active->reqheight()+
    max($window_mute->reqheight(),$window_quit->reqheight())+
    30;

$toplevel->minsize($minwidth,$minheight);

my$geometry=$toplevel->optionGet("geometry","");
if(defined($geometry)){
    $toplevel->geometry($geometry);
}else{
    $toplevel->geometry(($minwidth+20).'x'.$minheight);
}
$toplevel->update();

$window_quit->configure(-command=>[sub{Shutdown();}]);
$window_amute->configure(-command=>[sub{Robot_Audio();}]);
$window_vmute->configure(-command=>[sub{Robot_Video();}]);
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
    my@list=bsd_glob("$CONFIG{'LIBSNATCH'}",
		     GLOB_TILDE|GLOB_ERR|GLOB_BRACE);
    if(GLOB_ERROR || $#list<0){
	Status("Failed to find libsnatch.so!");
	Alert("Failed to find libsnatch.so!",
	      "Please verify that libsnatch.so is built,".
	      " installed, and its location is set correctly ".
	      "on the configuration panel.\n");
	return;
    }
    
    $ENV{"SNATCH_DEBUG"}=1;
    $ENV{"LD_PRELOAD"}=@list[0];
    $ENV{"SNATCH_COMM_SOCKET"}=$backchannel_socket;

    @list=bsd_glob("$CONFIG{'REALPLAYER'}",
		     GLOB_TILDE|GLOB_ERR|GLOB_BRACE);
    if(GLOB_ERROR || $#list<0){
	Status("Failed to find RealPlayer!");
	Alert("Failed to find RealPlayer!",
	      "Please verify that RealPlayer is installed,".
	      " executable, and its location is set correctly".
	      "on the configuration panel.\n");
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
    while($time>0){
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
		$time=-1;
	    }
	}
    }

    if($time==0){
	Alert("Rendevous failed!",
	      "RealPlayer appears to have started, but the Snatch ".
	      "robot could not connect to it via libsnatch.  Most likely,".
	      " this is a result of having a blank or mis-set ".
	      "'libsntach.so location' setting on the configuration ".
	      "menu.  Please verify this setting before continuing.\n");
	Status("Rendevous failed!");
	return;
    }

    # configure
    Status("RealPlayer started...");
    $toplevel->fileevent(REAL_STDERR,'readable'=>[sub{ReadStderr();}]);
    ButtonConfig();
    send_string("F",$CONFIG{'OUTPUT_PATH'});
    send_string("D",$CONFIG{'AUDIO_DEVICE'});
    Robot_Active() if($mode eq 'active');
    Robot_Timer() if($mode eq 'timer');
    Robot_Inactive() if($mode eq 'inactive');

}

sub BindSocket{
    Status("Binding socket..");
    die $! unless socket(LISTEN_SOCK, PF_UNIX, SOCK_STREAM,0);
    unlink($backchannel_socket);
    die $! unless bind(LISTEN_SOCK,$uaddr);
    die $! unless listen(LISTEN_SOCK,SOMAXCONN);
}

sub Disconnect{
    $comm_ready=0;
    ButtonConfig();
    close COMM_SOCKET if($comm_ready);
    close REAL_STDERR if($comm_ready);
}


sub ReleaseSocket{
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
	    Status("RealPlayer contacted");
	    $comm_ready=1;
	    ButtonConfig();
	}
    }
}

sub SaveConfig{
    die $! unless open CFILE, ">$configfile".".tmp";
    foreach $key (keys %CONFIG){
	print CFILE "$key=$CONFIG{$key}\n";
    }
    close CFILE;
    die $! unless rename "$configfile".".tmp", $configfile;
}

sub SaveHistory{
    die $! unless open HFILE, ">$historyfile".".tmp";
    foreach $line (@TIMER){
	print HFILE "$line\n";
    }
    close HFILE;
    die $! unless rename "$historyfile".".tmp", $historyfile;
}

sub Shutdown{
# save the config/history
    Robot_Exit();
    ReleaseSocket();
    SaveConfig();
    SaveHistory();

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
    Disconnect();
}

sub Robot_Active{
    # clear out robot settings to avoid hopelessly confusing the user
    send_string("U","");
    send_string("P","");
    send_string("O","");
    send_string("L","");
    syswrite COMM_SOCK,'A';
    Robot_Audio($CONFIG{"AUDIO_MUTE"});
    Robot_Video($CONFIG{"VIDEO_MUTE"});
    Status("Ready/waiting to record");
    $mode='active';
    ButtonPressConfig();
}

sub Robot_Inactive{
    send_string("U","");
    send_string("P","");
    send_string("O","");
    send_string("L","");
    syswrite COMM_SOCK,'I';
    Robot_Audio($CONFIG{"AUDIO_MUTE"});
    Robot_Video($CONFIG{"VIDEO_MUTE"});
    Status("Recording off");
    $mode='inactive';
    ButtonPressConfig();
}

sub Robot_Timer{
    send_string("U","");
    send_string("P","");
    send_string("O","");
    send_string("L","");
    syswrite COMM_SOCK,'T';
    Robot_Audio($CONFIG{"AUDIO_MUTE"});
    Robot_Video($CONFIG{"VIDEO_MUTE"});
    Status("Timed recording only");
    $mode='timer';
    ButtonPressConfig();
}

sub Robot_Audio{
    my($onoff)=@_;

    if($onoff=~m/yes/){
	syswrite COMM_SOCK,'s';
	$CONFIG{"AUDIO_MUTE"}='yes';
    }elsif($onoff=~m/no/){
	syswrite COMM_SOCK,'S';
	$CONFIG{"AUDIO_MUTE"}='no';
    }else{
	if($CONFIG{"AUDIO_MUTE"}=~/yes/){
	    syswrite COMM_SOCK,'S';
	    $CONFIG{"AUDIO_MUTE"}='no';
	}else{
	    syswrite COMM_SOCK,'s';
	    $CONFIG{"AUDIO_MUTE"}='yes';
	}
    }
    ButtonPressConfig();

}

sub Robot_Video{
    my($onoff)=@_;

    if($onoff=~m/yes/){
	syswrite COMM_SOCK,'v';
	$CONFIG{"VIDEO_MUTE"}='yes';
    }elsif($onoff=~m/no/){
	syswrite COMM_SOCK,'V';
	$CONFIG{"VIDEO_MUTE"}='no';
    }else{
	if($CONFIG{"VIDEO_MUTE"}=~/yes/){
	    syswrite COMM_SOCK,'V';
	    $CONFIG{"VIDEO_MUTE"}='no';
	}else{
	    syswrite COMM_SOCK,'v';
	    $CONFIG{"VIDEO_MUTE"}='yes';
	}
    }
    ButtonPressConfig();
}

sub SplitTimerEntry{
    my($line)=@_;

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
	Button(-class=>"Exit",text=>"X")->
		   place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');
    
    $modal_message=$modal_shell->
	Label(text=>$message,-class=>"AlertText")->
	    place(-x=>5,-y=>10);

    $width=$modal_message->reqwidth();
    $width=300 if($width<300);

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
	Disconnect();
	Alert("RealPlayer unexpectedly exited!","Attempting to start a new copy...\n");
	$toplevel->fileevent(REAL_STDERR,'readable' => ''); 
	ThrowRealPlayer();
    }

    if($scalar=~/shut down X/){
	Disconnect();
	$toplevel->fileevent(REAL_STDERR,'readable' => ''); 
      Tk::exit(0);
    }	
    print $scalar if($CONFIG{DEBUG} eq 'yes');
}

sub ButtonPressConfig(){
    $window_timer->configure(-relief=>'groove') if ($mode ne 'timer');
    $window_timer->configure(-relief=>'sunken') if ($mode eq 'timer');
    $window_active->configure(-relief=>'groove') if ($mode ne 'active');
    $window_active->configure(-relief=>'sunken') if ($mode eq 'active');
    $window_inactive->configure(-relief=>'groove') if ($mode ne 'inactive');
    $window_inactive->configure(-relief=>'sunken') if ($mode eq 'inactive');

    $window_amute->configure(-relief=>'groove') if ($CONFIG{AUDIO_MUTE} eq 'no');
    $window_amute->configure(-relief=>'sunken') if ($CONFIG{AUDIO_MUTE} eq 'yes');
    $window_vmute->configure(-relief=>'groove') if ($CONFIG{VIDEO_MUTE} eq 'no');
    $window_vmute->configure(-relief=>'sunken') if ($CONFIG{VIDEO_MUTE} eq 'yes');

}

sub ButtonConfig{
    if ($#TIMER<0 || !$comm_ready){
	$window_timer->configure(state=>disabled);
    }else{ 
	$window_timer->configure(state=>normal);
    }
    if (!$comm_ready){
	$window_active->configure(state=>disabled);
	$window_inactive->configure(state=>disabled);
	$window_amute->configure(state=>disabled);
	$window_vmute->configure(state=>disabled);
    }else{ 
	$window_active->configure(state=>normal);
	$window_inactive->configure(state=>normal);
	$window_amute->configure(state=>normal);
	$window_vmute->configure(state=>normal);
    }
    ButtonPressConfig();
}

sub TimerSort{
    $count=0;
    @TIMER_TIMES=(map {TimerWhen(-1,(SplitTimerEntry($_)))} @TIMER);
    @TIMER_SORTED=sort sortsub, (map {$count++} @TIMER);
}    

sub Setup{
    %TEMPCONF=%CONFIG;
    my$tempstdout;
    if($CONFIG{'OUTPUT_PATH'} eq '-'){
	$tempstdout='yes';
	$TEMPCONF{'OUTPUT_PATH'}=`pwd`;
    }

    $window_setupbar->configure(-state=>'disabled');
    $window_setupbar->configure(-relief=>'flat');
    $setup=new MainWindow(-class=>'Snatch');

    my$xpm_snatch=$setup->Pixmap("_snatchlogo_xpm",-file=>$logofile);

    $setup_shell=$setup->Label(Name=>"shell",borderwidth=>1,relief=>raised)->
	place(-x=>10,-y=>36,-relwidth=>1.0,-relheight=>1.0,
	      -width=>-20,-height=>-46,-anchor=>'nw');
    
    $setup_quit=$setup_shell->
	Button(-class=>"Exit",text=>"OK")->
		   place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');
    $setup_apply=$setup_shell->
	Button(-class=>"Exit",text=>"apply")->
		   place(-x=>0,-y=>0,-anchor=>'ne',-in=>$setup_quit,
			 -bordermode=>outside);
    $setup_cancel=$setup_shell->
	Button(-class=>"Exit",text=>"cancel")->
		   place(-x=>1,-y=>-1,-rely=>1.0,-anchor=>'sw');
    

    $setup_logo=$setup->
	Label(Name=>"logo",-class=>"Panel",image=>$xpm_snatch)->
		  place(-x=>5,-y=>5,-anchor=>'nw');

    $setup_title=$setup->
	Label(Name=>"setup text",-class=>"Panel",text=>"Configuration")->
		  place(-x=>5,-relx=>1.0,-rely=>1.0,-anchor=>'sw',
			-in=>$setup_logo);

    # Real location
    $nexty=5;
    $temp=$setup_shell->
	Label(text=>"RealPlayer location:")->
	    place(-x=>5,-y=>$nexty);
    $setup_shell->
	Entry(-textvariable=>\$TEMPCONF{'REALPLAYER'},-width=>"256")->
	    place(-y=>$nexty,-x=>$temp->reqwidth()+10,
		  -anchor=>'nw',-relwidth=>1.0,
		  -height=>$temp->reqheight(),
		  -width=>-$temp->reqwidth()-18);
    $nexty=8+$temp->reqheight();

    #libsnatch location
    $temp=$setup_shell->
	Label(text=>"libsnatch.so location:")->
	    place(-x=>5,-y=>$nexty);
    $setup_shell->
	Entry(-textvariable=>\$TEMPCONF{'LIBSNATCH'},-width=>"256")->
	    place(-y=>$nexty,-x=>$temp->reqwidth()+10,
		  -anchor=>'nw',-relwidth=>1.0,
		  -height=>$temp->reqheight(),
		  -width=>-$temp->reqwidth()-18);
    $nexty+=8+$temp->reqheight();

    #audio device
    $temp=$setup_shell->
	Label(text=>"audio device (OSS only):")->
	    place(-x=>5,-y=>$nexty);
    $setup_shell->
	Entry(-textvariable=>\$TEMPCONF{'AUDIO_DEVICE'},-width=>"256")->
	    place(-y=>$nexty,-x=>$temp->reqwidth()+10,
		  -anchor=>'nw',-relwidth=>1.0,
		  -height=>$temp->reqheight(),
		  -width=>-$temp->reqwidth()-18);
    $nexty+=15+$temp->reqheight();
    
    #debug
    if($TEMPCONF{'DEBUG'} eq 'yes'){
	$temp=$setup_debug=$setup_shell->
	    Button(text=>"full debug output",-relief=>'sunken',-pady=>1)->
		place(-x=>5,-y=>$nexty);
    }else{
	$temp=$setup_debug=$setup_shell->
	    Button(text=>"full debug output",-pady=>1)->
		   place(-x=>5,-y=>$nexty);
    }
    $setup_debug->configure(-command=>[sub{Setup_Debug();}]);
    $nexty+=15+$temp->reqheight();

    #output path
    $temp=$setup_shell->
	Label(text=>"capture output:")->
	    place(-x=>5,-y=>$nexty);
    if($tempstdout eq 'yes'){
	$setup_stdout=$setup_shell->
	    Button(text=>"stdout",-relief=>'sunken',-pady=>1)->
		place(-x=>$temp->reqwidth()+5,-y=>$nexty,
		      -height=>$temp->reqheight());
	$setup_path=$setup_shell->
	    Entry(-textvariable=>\$TEMPCONF{'OUTPUT_PATH'},-width=>256,
		  -state=>disabled,relief=>groove)->
		      place(-x=>$temp->reqwidth()+10+$setup_stdout->reqwidth(),
			    -y=>$nexty,-height=>$temp->reqheight(),
			    -width=>-$setup_stdout->reqwidth()-$temp->reqwidth()-18,
			    -relwidth=>1.0);
    }else{
	$setup_stdout=$setup_shell->
	    Button(text=>"stdout",-pady=>1)->
		place(-x=>$temp->reqwidth()+5,-y=>$nexty,
		      -height=>$temp->reqheight());
	$setup_path=$setup_shell->
	    Entry(-textvariable=>\$TEMPCONF{'OUTPUT_PATH'},-width=>256)->
		      place(-x=>$temp->reqwidth()+10+$setup_stdout->reqwidth(),
			    -y=>$nexty,-height=>$temp->reqheight(),
			    -width=>-$setup_stdout->reqwidth()-$temp->reqwidth()-18,
			    -relwidth=>1.0);
    }

    $nexty+=15+$temp->reqheight();


    $minwidth=400;
    $minheight=$nexty+28+$setup_logo->reqheight()+$setup_cancel->reqheight();
    
    $setup->minsize($minwidth,$minheight);
    $setup->geometry(($minwidth+20)."x".$minheight);

    $setup_quit->configure(-command=>[sub{
	$TEMPCONF{"OUTPUT_PATH"}='-' if($tempstdout eq 'yes');
	$setup->destroy();undef $setup;%CONFIG=%TEMPCONF;
	$window_setupbar->configure(state=>'normal');
	$window_setupbar->configure(relief=>'raised');
	SaveConfig();
	
	ThrowRealPlayer() if(!$comm_ready);
    }]);

    $setup_apply->configure(-command=>[sub{
	$TEMPCONF{"OUTPUT_PATH"}='-' if($tempstdout eq 'yes');
	%CONFIG=%TEMPCONF;
	SaveConfig();
	
	ThrowRealPlayer() if(!$comm_ready);
    }]);
    
    $setup_cancel->configure(-command=>[sub{
	$setup->destroy();undef $setup;
	$window_setupbar->configure(state=>'normal');
	$window_setupbar->configure(relief=>'raised');
    }]);

    $setup_stdout->configure(-command=>[sub{Setup_Stdout();}]);

}

sub Setup_Debug{
    if($TEMPCONF{'DEBUG'} eq 'yes'){
	$TEMPCONF{'DEBUG'}='no';
	$setup_debug->configure(-relief=>groove);
    }else{
	$TEMPCONF{'DEBUG'}='yes';
	$setup_debug->configure(-relief=>sunken);
    }
}    

sub Setup_Stdout{
    if($tempstdout eq 'yes'){
	$tempstdout='no';
	$setup_path->configure(-state=>normal);
	$setup_path->configure(-relief=>sunken);
	$setup_stdout->configure(-relief=>groove);
    }else{
	$tempstdout='yes';
	$setup_path->configure(-state=>disabled);
	$setup_path->configure(-relief=>groove);
	$setup_stdout->configure(-relief=>sunken);
    }
}    


sub Timer{

    $window_timerbar->configure(-state=>'disabled');
    $window_timerbar->configure(-relief=>'flat');
    $timerw=new MainWindow(-class=>'Snatch');

    my$xpm_snatch=$timerw->Pixmap("_snatchlogo_xpm",-file=>$logofile);

    $timerw_shell=$timerw->Label(Name=>"shell",borderwidth=>1,relief=>raised)->
	place(-x=>10,-y=>36,-relwidth=>1.0,-relheight=>1.0,
	      -width=>-20,-height=>-46,-anchor=>'nw');
    
    $timerw_quit=$timerw_shell->
	Button(-class=>"Exit",text=>"X")->
		   place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');
    
    $timerw_quit->configure(-command=>[sub{
	$timerw->destroy();
	$window_timerbar->configure(state=>'normal');
	$window_timerbar->configure(relief=>'raised');
    }]);

    $timerw_logo=$timerw->
	Label(Name=>"logo",-class=>"Panel",image=>$xpm_snatch)->
		  place(-x=>5,-y=>5,-anchor=>'nw');

    $timerw_title=$timerw->
	Label(Name=>"timertext",-class=>"Panel",text=>"Timed Record Setup")->
	    place(-x=>5,-relx=>1.0,-rely=>1.0,-anchor=>'sw',
		  -in=>$timerw_logo);

    $timerw_delete=$timerw_shell->
	Button(Name=>"delete",text=>"delete",-state=>disabled)->
	    place(-x=>-5,-relx=>1.0,-y=>-$timerw_quit->reqheight()-25,
		  -rely=>1.0,-anchor=>'se');

    $timerw_edit=$timerw_shell->
	Button(Name=>"edit",text=>"edit",-state=>disabled)->
	    place(-x=>0,-y=>-25,-relwidth=>1.0,-anchor=>'sw',
		  -in=>$timerw_delete,-bordermode=>outside);
    $timerw_add=$timerw_shell->
	Button(Name=>"add",text=>"add")->
	    place(-x=>0,-y=>-5,-relwidth=>1.0,-anchor=>'sw',
		  -in=>$timerw_edit,-bordermode=>outside);

    $listbox=Snatch::ListBox::new($timerw_shell,7,
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah1",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah2",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah3",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah4",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah5",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah6",
			  "2001","Dec","12","Tues","12:00","2 hours","rtsp://blah7")->
			      place(-x=>5,-y=>5,-relheight=>1.0,-relwidth=>1.0,
				    -width=>-$timerw_delete->reqwidth()-15,
				    -height=>-$timerw_quit->reqheight()-30,
				    -bordermode=>outside);
    
    $minwidth=500;
    $minheight=$timerw_add->reqheight()*3+$timerw_quit->reqheight()+110;
    
    $timerw->minsize($minwidth,$minheight);
    $timerw->geometry(($minwidth+20)."x".$minheight);
    
    $timerw->update();
}

package Snatch::ListBox;

sub new{
    my%listbox;
    my$this=bless \%listbox;

    my$parent=shift @_;
    my$cols=$listbox{cols}=shift @_;
    $listbox{rows}=0;
    my@textrows;
    my@widgetrows;

    $listbox{textrows}=\@textrows;
    $listbox{widgetrows}=\@widgetrows;

    my$frame=$listbox{frame}=$parent->Frame(-class=>'ListBoxFrame');
    my$scrollbar=$listbox{scrollbar}=$frame->Scrollbar(-orient=>"vertical")->
	place(-relx=>1.0,-relheight=>1.0,-anchor=>'ne',-bordermode=>outside);
    my$pane=$listbox{pane}=$frame->Frame(-class=>'ListBox')->
	place(-relwidth=>1.0,-relheight=>1.0,-width=>-$scrollbar->reqwidth());

    $listbox{window}=$pane->Frame(-class=>'ListFrame')->place(-relwidth=>1.0);

    my$maxheight=0;

    # row by row
    my$done=0;
    for($listbox{rows}=0;!$done;$listbox{rows}++){
	my @textrow=();
	my @widgetrow=();
	$textrows[$listbox{rows}]=\@textrow;
	$widgetrows[$listbox{rows}]=\@widgetrow;

	for(my$j=0;$j<$cols;$j++){
	    my$temp=shift;
	    if(defined($temp)){
		$textrow[$j]=$temp;
		if($listbox{rows} % 2){
		    $widgetrow[$j]=$listbox{window}->
			Label(-class=>'ListRowEven',text=>$temp);
		}else{
		    $widgetrow[$j]=$listbox{window}->
			Label(-class=>'ListRowOdd',text=>$temp);
		}
	    }else{
		$done=1;
		last;
	    }
	}
    }
    $listbox{rows}--;
	
    my@maxwidth;
    my$x=0;
    my$y=0;

    # find widths col by col
    for(my$j=0;$j<$listbox{cols};$j++){
	$y=0;
	$maxwidth[$j]=0;
	for(my$i=0;$i<$listbox{rows};$i++){
	    my$width=$widgetrows[$i][$j]->reqwidth();
	    my$height=$widgetrows[$i][$j]->reqheight();
	    $maxwidth[$j]=$width if($width>$maxwidth[$j]);
	    $maxheight=$height if($height>$maxheight);
	}

	if($j+1<$listbox{cols}){
	    for(my$i=0;$i<$listbox{rows};$i++){
		$widgetrows[$i][$j]->
		    place(-height=>$maxheight,-width=>$maxwidth[$j],
			  -x=>$x,-y=>$y);
		$y+=$maxheight+3;
	    }
	    $x+=$maxwidth[$j]+1;
	}else{
	    for(my$i=0;$i<$listbox{rows};$i++){
		$widgetrows[$i][$j]->configure(-anchor=>w);
		$widgetrows[$i][$j]->
		    place(-height=>$maxheight,-relwidth=>1.0,
			  -width=>-$x,-x=>$x,-y=>$y);
		$y+=$maxheight+3;
	    }
	    $x+=$maxwidth[$j]+1;
	}
    }

    #$frame->bind('syncscrollbar','<Configure>',[\$this->resize,Ev('w'),Ev('h')]);
    $pane->bind('<Configure>',[sub{$this->resize();}]);
    $listbox{window}->configure(-height=>$y);
    $scrollbar->configure(-command=>[yview=>$this]);

    $this;
}
   
sub place{
    my$this=shift;
    $this->{frame}->place(@_);
    $this;
}

sub destroy{
    my$this=shift;
    $this->{frame}->destroy();
    undef $$this;
}

sub yview{
    my$this=shift;
    
    my$paneheight=$this->{pane}->height();
    my$listheight=$this->{window}->height();

    my$first=-$this->{window}->y()/$listheight;
    my$second=(-$this->{window}->y()+$paneheight)/$listheight;

    print "$first $second\n";
    ($first,$second);

}

sub resize{
    my$this=shift;

    print "resize\n";
    
    $this->{scrollbar}->set($this->yview());
}
# eg
# 2001 11 05 1 12:25 300000 FAKEA FAKEV length:USERNAME length:PASSWORD length:FILE length:URL




