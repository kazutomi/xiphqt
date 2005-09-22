<?
include "config.php";

$dologin = 0;
$username = "";
$password = "";
if (isset($_REQUEST["dologin"]))
	$dologin=$_REQUEST["dologin"];
if (isset($_REQUEST["username"]))
	$username=$_REQUEST["username"];
if (isset($_REQUEST["password"]))
	$password=$_REQUEST["password"];

if ($dologin == 1) {
	if (($adminPassword == $password) && ($adminUser == $username)) {
		session_start();
		$_SESSION['loggedin'] = 1;
		header("Location: index.php");
	}
	else {
		session_start();
		session_destroy();
		header("Location: error.php?errorCode=1");
	}
}
include "header.php";
?>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">

<p>
	<form method="post" action="login.php">
		<input type="hidden" name="dologin" value="1">
	<center>
	<table border=1 width=50% bgcolor="#222222">
		<tr><td><center>Login</center></td></tr>
	</table>
		<br>
	<table border=0>
		<tr><td>UserName</td><td><input type="text" name="username" size=20></tr>
		<tr><td>Password</td><td><input type="password" name="password" size=20></tr>
	</table>
		<br><br>
		<input type="submit" value="Login">
   </center>
	</form>
	<br>
	<br>
</p>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

