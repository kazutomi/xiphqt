<? include "header.php"; ?>
<? include "db.php"; ?>
<?
	$db = new DB($mysql_host, $mysql_user, $mysql_password, $mysql_db);
?>
<h2>Add User</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<p>
<?

$doAdd = true;
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
	$doAdd = true;
}
else {
	$doAdd = false;
}

if (($confirm == "1") && ($doAdd == false)) {
	echo "<font color=red>Required field missing</font><br>";
}

$displayform = true;

if ($doAdd) {
	if ($timelimit == "") {
		$timelimit = "-1";
	}
	$query = sprintf("insert into ".$table_prefix."users (username, password, mountpoint, email, timeleft) values ('%s', '%s', '%s', '%s', %s)",  
			mysql_real_escape_string($username),
			mysql_real_escape_string($password),
			mysql_real_escape_string($mount),
			mysql_real_escape_string($email),
			$timelimit);
			
	if (mysql_query($query, $db->link)) {
		echo "<i>Added User $username</i><br>";
	}
	else {
		echo "<font color=red>Error adding user $username : ".mysql_error()."</font><br>";
	}
	
}

?>
<?
if ($displayform) {
	if ($mount == "") {
		$allselected = "selected";
	}
	else {
		$allselected = "";
	}
?>
<form method=GET action="adduser.php">
<input type=hidden name="confirm" value="1">
<table border=0>
<tr>
<td>Mountpoint:</td>
<td> 
<select name="mount">
<option value='ALL' <? echo $allselected; ?>>All Mountpoints
<?
$query = "select mountpoint_name from ".$table_prefix."mountpoints";
$result = mysql_query($query, $db->link);
while ($row = mysql_fetch_assoc($result)) {
	$availablemount = $row['mountpoint_name'];
	if ($availablemount == $mount) {
   		$selected = "selected";
   	}
   	else {
   		$selected = "";
   	}
	echo "<option value='$availablemount' $selected >$availablemount";
}
mysql_free_result($result);
?>
</select>
</td></tr>
<tr><td>Username</td><td><input type="text" name="username" size="15"></td></tr>
<tr><td>Password</td><td><input type="text" name="password" size="15"></td></tr>
<tr><td>E-mail</td><td><input type="text" name="email" size="15"></td></tr>
<tr><td>Time Restriction (in seconds)</td><td><input type="text" name="timelimit" size="15"></td></tr>
</table>
<br>
<br>
<input type="Submit" value="Add User">
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
