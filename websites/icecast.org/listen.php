<?
$serverURL = $_GET["serverURL"];

header("Content-type: audio/m3u");
if (preg_match("/MSIE 5.5/", $HTTP_USER_AGENT)) {
	header("Content-Disposition: filename=\"listen.m3u\"");
}
else {
	header("Content-Disposition: inline; filename=\"listen.m3u\"");
}
print $serverURL."\r\n";
?>
