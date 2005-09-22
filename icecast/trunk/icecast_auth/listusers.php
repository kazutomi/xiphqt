<? include "header.php"; ?>
<? include "db.php"; ?>
<?
	$db = new DB($mysql_host, $mysql_user, $mysql_password, $mysql_db);
?>
<?

$mount = "";

if (isset($_REQUEST["mount"])) {
	$mount=$_REQUEST["mount"];
}
$username = "";

if (isset($_REQUEST["username"])) {
	$username=$_REQUEST["username"];
}

?>
<h2>Mountpoints</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Mountpoint Listing (<? echo $mount; ?>)</h3>
<p>
<table border=0 width=100%>
<tr><td>Username</td><td>Email<td><td>Time Restrictions</td></tr>
<?
$query = "select username, email, timeleft, mountpoint from ".$table_prefix."users where mountpoint = '$mount' or mountpoint = 'ALL'";
if ($username != "") {
	$query = "select username, email, timeleft, mountpoint from ".$table_prefix."users where username like '%$username%'";
}
$result = mysql_query($query, $db->link);
while ($row = mysql_fetch_assoc($result)) {
	$username = $row['username'];
	$email = $row['email'];
	$timeleft = $row['timeleft'];
	$mountpoint = $row['mountpoint'];
	$timerestriction = "None";
	if ($timeleft != "-1") {
		$timerestriction = "$timeleft seconds";
	}
	$mountall = " ($mountpoint) ";
	if ($mountpoint == "ALL") {
		$mountall = " (All) ";
	}
	echo "<tr><td><a href='edituser.php?username=".urlencode($username)."&mount=".urlencode($mountpoint)."'>$username</a>$mountall</td><td>$email<td><td>$timerestriction</td></tr>";
}
mysql_free_result($result);
?>
</table>
	<br>
</p>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>	
</div>
<? include "footer.php"; ?>
