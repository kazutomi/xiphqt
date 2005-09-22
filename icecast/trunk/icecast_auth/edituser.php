<? include "header.php"; ?>
<? include "db.php"; ?>
<?
	$db = new DB($mysql_host, $mysql_user, $mysql_password, $mysql_db);
?>
<h2>Edit User</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<p>
<?

$doEdit = true;
$username = "";
if (isset($_REQUEST["username"])) {
	$username=$_REQUEST["username"];
}
$password = "";
if (isset($_REQUEST["password"])) {
	$password=$_REQUEST["password"];
}
$email = "";
if (isset($_REQUEST["email"])) {
	$email=$_REQUEST["email"];
}
$timelimit = "";
if (isset($_REQUEST["timelimit"])) {
	$timelimit=$_REQUEST["timelimit"];
}
$mount = "";
if (isset($_REQUEST["mount"])) {
	$mount=$_REQUEST["mount"];
}
$confirm = "";
if (isset($_REQUEST["confirm"])) {
	$confirm=$_REQUEST["confirm"];
}

if (($username != "") && ($password != "") && ($mount != "")) {
	$doEdit = true;
}
else {
	$doEdit = false;
}

if (($confirm == "1") && ($doEdit == false)) {
	echo "<font color=red>Required field missing</font><br>";
}

$displayform = true;

if ($doEdit) {
	if ($timelimit == "") {
		$timelimit = "-1";
	}
	$query = sprintf("update ".$table_prefix."users set password = '%s', email = '%s', timeleft = %s where username = '%s' and mountpoint = '%s'",  
			mysql_real_escape_string($password),
			mysql_real_escape_string($email),
			mysql_real_escape_string($timelimit),
			mysql_real_escape_string($username),
			mysql_real_escape_string($mount));
			
	if (mysql_query($query, $db->link)) {
		echo "<i>User $username updated</i><br>";
		$displayform = false;
	}
	else {
		echo "<font color=red>Error updating user $username : ".mysql_error()."</font><br>";
		$displayform = false;
	}
	
}

?>
<?
if ($displayform) {
?>
<form method=GET action="edituser.php">
<input type=hidden name="confirm" value="1">
<table border=0>
<?
$query = "select username, password, email, timeleft, mountpoint from ".$table_prefix."users where mountpoint = '".mysql_real_escape_string($mount)."' and username = '".mysql_real_escape_string($username)."'";
$result = mysql_query($query, $db->link);
while ($row = mysql_fetch_assoc($result)) {
	$username = $row['username'];
	$password = $row['password'];
	$email = $row['email'];
	$timeleft = $row['timeleft'];
	$mountpoint = $row['mountpoint'];
	echo '<tr><td>Username</td><td><input type="hidden" name="username" size="15" value="'.$username.'">'.$username.' - Check <a href="reports.php?username='.urlencode($username).'">User Report</a></td></tr>';
	echo '<tr><td>Mountpoint</td><td><input type="hidden" name="mount" size="15" value="'.$mount.'">'.$mount.'</td></tr>';
	echo '<tr><td>Password</td><td><input type="text" name="password" size="15" value="'.$password.'"></td></tr>';
	echo '<tr><td>E-mail</td><td><input type="text" name="email" size="25" value="'.$email.'"></td></tr>';
	echo '<tr><td>Time Restriction (in seconds)</td><td><input type="text" name="timelimit" size="15" value="'.$timeleft.'"> ***Enter -1 for no time restrictions.</td></tr>';
}
?>	
</table>
<br>
<br>
<input type="hidden" name="mount" value="<? echo $mount; ?>">
<input type="Submit" value="Edit User">
</form>
</p>
<?
}
?>
	<br>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>	
</div>
<? include "footer.php"; ?>
