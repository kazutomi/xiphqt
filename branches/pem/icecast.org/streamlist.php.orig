<? $nopad = 1; ?>
<? include "header.php"; ?>
<?
$username = "oddsock";
$password = "";
$database = "oddsock";
$max_per_page = 10;
$start = $_GET["start"];
$num = $_GET["num"];
$sgenre = $_GET["sgenre"];
$search = $_GET["search"];

$link = mysql_connect("localhost", $username, $password);
mysql_select_db($database);

if ($num) {
	$pagesize = $num;
}
else {
	$pagesize = $max_per_page;
}

function getPrevLink($current, $pagesize, $rowcounter) {
	global $search;
	global $sgenre;

	if (($current - $pagesize) < 0) {
		$prev = -1;
	}
	else {
		$prev = $current - $pagesize;
	}
	$extra = "";
	if ($search) {
		$extra .= "&search=".urlencode($search);
	}
	if ($sgenre) {
		$extra .= "&sgenre=".urlencode($sgenre);
	}
	if ($prev >= 0) {
		return "<a class=\"nav\" href=\"?start=$prev&num=$pagesize$extra\"><font size=3>Prev</font></a>";
	}
	else {
		return "";
	}
}
function getNextLink($current, $pagesize, $rowcounter) {
	global $search;
	global $sgenre;

	if ($rowcounter < $pagesize) {
		return "";
	}
	$next = $current + $pagesize;
	$extra = "";
	if ($search) {
		$extra .= "&search=".urlencode($search);
	}
	if ($sgenre) {
		$extra .= "&sgenre=".urlencode($sgenre);
	}
	return "<a class=\"nav\" href=\"?start=$next&num=$pagesize$extra\"><font size=3>Next</font></a>";
}
function getTotalListeners() {
	$query = "select sum(listeners) from servers";
	$result = mysql_query($query);
	if (!$result) {
		print "<center><font color=red>Mysql Error: ".mysql_error()."!</center><br>";
	}
	 
	$row = mysql_fetch_row($result);
	if ($row) {
		return $row[0];
	}
}
function getTotalServers() {
	$query = "select count(*) from server_details";
	$result = mysql_query($query);
	if (!$result) {
		print "<center><font color=red>Mysql Error: ".mysql_error()."!</center><br>";
	}
	 
	$row = mysql_fetch_row($result);
	if ($row) {
		return $row[0];
	}
}
function buildStreamDescription($listeners, $url, $server_name, $bgcolor) {
	$urllink = "";
	if ($url != "") {
		if (substr($url, 0, 7) != "http://") {
			$url = "http://".$url;
		}
		$urllink = '<a class="nav" href="'.$url.'"><b>'.substr($server_name, 0, 60).'</b></a>';
	}
	else {
		$urllink = '<b>'.substr($server_name, 0, 60).'</b>';
	}
	$streamLine = '
	<tr>
		<td colspan=5 height=3 bgcolor="#333333"></td>
	</tr>
	<tr>
		<td class="streamtd_alt" bgcolor="'.$bgcolor.'"></td><td class="streamtd_alt" colspan=1 bgcolor="'.$bgcolor.'" >'.$urllink.'</td><td class="streamtd_alt" colspan=3 bgcolor="'.$bgcolor.'"></td>
	</tr>
';
	return $streamLine;
}
function buildStreamLine($server_name,$listen_url, $genre, $listeners, $description, $current_song, $server_type, $bitrate, $samplerate, $channels, $stream_id, $bgcolor) {
	if ($channels == "1") {
		$channels = "Mono";
	}
	if ($channels == "2") {
		$channels = "Stereo";
	}
	$mountpoint = preg_replace("/^http:\/\/.*:.*\//", "/", $listen_url);
	$type_link = "$server_type";
	if ($server_type == "Ogg Vorbis") {
		$type_link = "<img title='Ogg Vorbis - Listen' alt='Ogg Vorbis - Listen' src='images/vorbis.gif' border=0>";
		if ($bitrate == "Quality 1@00") {
			$bitrate = "";
		}
		if ($bitrate > 1000) {
			$bitrate = "";
		}
		if ($bitrate == "0") {
			$bitrate = "";
		}
		if ($samplerate == "") {
			$samplerate = "";
		}
		else {
			$samplerate = $samplerate."Hz";
		}
		if ($channels == "") {
			$channels = "";
		}
		if (substr($bitrate, 0, 5) == "Quali") {
			$audio_info = $bitrate."<br>".$samplerate." ".$channels;

		}
		else {
			$audio_info = $bitrate."kbps<br>".$samplerate." ".$channels;
		}
	}
	if ($server_type == "MP3 audio") {
		$type_link = "<img title='MP3 - Listen' alt='MP3 - Listen' src='images/mp3.gif' border=0>";
		if ($bitrate == "Quality 0") {
			$bitrate = "";
		}
		if ($bitrate == "0") {
			$bitrate = "";
		}
		if ($samplerate == "") {
			$samplerate = "";
		}
		else {
			$samplerate = $samplerate."Hz";
		}
		if ($channels == "") {
			$channels = "";
		}
		$audio_info = $bitrate."kbps<br>".$samplerate." ".$channels;
	}
	if (($samplerate == "0") || ($channels == "0")) {
		$audio_info = "Unknown";
	}
	if ($server_name != $description) {
		$description_tag = '<font color="#AAAAAA">'.$description.'</font><br>';
	}
	$streamLine = '
	<tr>
		<td class="streamtd_alt" valign="middle" bgcolor="'.$bgcolor.'" ><a href="listen.php?serverURL='.urlencode($listen_url).'&file=listen.m3u">'.$type_link.'</a></td>
		<td class="streamtd_alt" bgcolor="'.$bgcolor.'">'.$description_tag.'Now playing on <a class="streamtd_alt_2" href="listen.php?serverURL='.urlencode($listen_url).'&file=listen.m3u">'.$mountpoint.'</a> :&nbsp;&nbsp;<font color=yellow>'.substr($current_song, 0, 100).'</font></td>
		<td class="streamtd_alt" width=75 bgcolor="'.$bgcolor.'"> <center><font color=white>'.substr($genre, 0, 30).'</font></td>
		<td class="streamtd_alt" width=150 bgcolor="'.$bgcolor.'"><center>'.$audio_info.'</td>
	</tr>
';
	return $streamLine;
}
?>
<td valign=top width=100%>
<?
$directory_data = '
<table border=0 cellpadding=2 cellspacing=0 width="100%">
<tr>
		<td colspan=5 bgcolor="#000000" height=2></td>
</tr>
<tr>
		<td colspan=5 bgcolor="#000000" height=3></td>
</tr>
<tr>
		<td bgcolor="#222222"></td>
		<td valign="middle" bgcolor="#222222" ><font size=2><b>Streams</td>
		<td width=75 bgcolor="#222222"> <font size=2><b><center>Genre</td>
		<td width=150 bgcolor="#222222"><font size=2><b><center>Audio Info</td>
</tr>
<tr>
		<td colspan=5 bgcolor="#222222" height=3></td>
</tr>
<tr>
		<td colspan=5 bgcolor="#222222" height=2></td>
</tr>
<tr>
		<td colspan=5 bgcolor="#222222" height=5></td>
</tr>
';
 
$query = "select id, server_name, listeners from servers order by rank desc";

if ($sgenre) {
	$query = "select a.id, a.server_name, a.listeners from servers a, server_details b where a.id = b.parent_id and b.genre like '%$sgenre%' group by a.id, a.server_name, a.listeners order by a.rank desc";
}
if ($search) {
	$query = "select a.id, a.server_name, a.listeners from servers a, server_details b where a.id = b.parent_id and (a.server_name like '%$search%' or b.description like '%$search%' or b.current_song like '%$search%' or b.genre like '%$search%') group by a.id, a.server_name, a.listeners order by a.rank desc";
}
$result = mysql_query($query);
if (!$result) {
        print "<center><font color=red>Mysql Error: ".mysql_error()."!</center><br>";
}
 
$row_counter = 0;
$printed = 0;
while ($row = mysql_fetch_row($result)) {
	$row_counter++;
	if ($start) {
		if ($row_counter < $start) {
			continue;
		}
	}
	if ($pagesize) {
		if ($printed >= $pagesize) {
			break;
		}
	}
        $id = $row[0];
        $server_name = htmlentities($row[1]);
        $listeners = $row[2];
	$cluster_flag = 0;
	if ($cluster_id != "") {
		$cluster_flag = 1;
	}
	if ($swap) {
		$swap = 0;
		$bgcolor = "#222222";
	}
	else {
		$swap = 1;
		$bgcolor = "#444444";
	}
	if ($cluster_flag) {
		$cluster_text = "cluster";
	}
	else {
		$cluster_text = "";
	}
	if ($server_name != "") {
		$all_listeners = $listeners;
		$query = "select description, genre, url, current_song, listen_url, server_type, bitrate, samplerate, channels, listeners+0, parent_id, id from server_details where parent_id = $id";
		
		$streamDetailsLine = "";
		$result2 = mysql_query($query);
		if (!$result2) {
			print "<center><font color=red>Mysql Error: ".mysql_error()."!</center><br>";
		}
		 
		while ($row2 = mysql_fetch_row($result2)) {
			$description = htmlentities($row2[0]);
			$genre = htmlentities($row2[1]);
			$url = $row2[2];
			$current_song = htmlentities($row2[3]);
			$listen_url = urlencode($row2[4]);
			$listen_url_raw = $row2[4];
			$server_type = $row2[5];
			$bitrate = $row2[6];
			$samplerate = $row2[7];
			$channels = $row2[8];
			$listeners = $row2[9];
			$cluster_id = $row2[10];
			$stream_id = $row2[11];
			$total_listeners = $total_listeners + $listeners;
			$streamDetailsLine .= buildStreamLine($server_name, $listen_url_raw, $genre, $listeners, $description, $current_song, $server_type, $bitrate, $samplerate, $channels, $stream_id, $bgcolor);
		}
		$streamDescriptionLine = buildStreamDescription($all_listeners, $url, $server_name, $bgcolor);
		$directory_data .= '
'.$streamDescriptionLine.'
'.$streamDetailsLine.'
';
		$printed++;
	}
}
$directory_data .= '
	<tr>
		<td colspan=5 height=3 bgcolor="#333333"></td>
	</tr>
	<tr>
		<td colspan=5 height=10 bgcolor="black"></td>
	</tr>
</table>';
$nav_text = '
<table border=0 bordercolor="#333399" cellpadding=0 cellspacing=0 width="100%">
<tr>
<td bgcolor="#000000"><font color=white face=Verdana size=1>'.getPrevLink($start, $pagesize, $printed).'</td>
<td bgcolor="#000000" width=100%>&nbsp;</td>
<td bgcolor="#000000"><font color=white face=Verdana size=1>'.getNextLink($start, $pagesize, $printed).'</td>
</tr>
</table>';
print '<center>Click <img src="images/vorbis.gif"> or <img src="images/mp3.gif"> to listen.</center>';
print $nav_text;
print $directory_data;
print $nav_text;
?>
</td>
<td class="streamtd" valign=top align=right>
<b>Search</b><br>
<form method=GET action=streamlist.php>
Genre<br><select style="font-size:8pt" name=sgenre>
<option>
<option value="Alternative">Alternative
<option value="Classical">Classical
<option value="Comedy">Comedy
<option value="Country">Country
<option value="Dance">Dance
<option value="Funk">Funk
<option value="Jazz">Jazz
<option value="Metal">Metal
<option value="Mixed">Mixed
<option value="Pop">Pop
<option value="Rap">Rap
<option value="RnB">RnB
<option value="Rock">Rock
<option value="Talk">Talk
<option value="Techno">Techno
<option value="80s">80s
<option value="70s">70s
</select>
<br>
Freeform Search<br><input name=search style="font-size:8pt">
<input type=Submit value=Search>
</form>
<br>
<br>
<b>Statistics</b><br>
Total Servers: <? print getTotalServers(); ?>
</td>
</tr>
</table>
</body>
</html>

