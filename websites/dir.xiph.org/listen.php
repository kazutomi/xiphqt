<?
$username = "oddsock";
$password = "";
$database = "oddsock";
$pid = $_GET["pid"];

$link = mysql_connect("localhost", $username, $password);
mysql_select_db($database);

header("Content-type: audio/m3u");
if (preg_match("/MSIE 5.5/", $HTTP_USER_AGENT)) {
	header("Content-Disposition: filename=\"listen.m3u\"");
}
else {
	header("Content-Disposition: inline; filename=\"listen.m3u\"");
}
$query = "select listen_url from playlists where id = $pid";
$result = mysql_query($query);
if (!$result) {
        print mysql_error();
}

while ($row = mysql_fetch_row($result)) {
	print $row[0]."\r\n";
}
?>
