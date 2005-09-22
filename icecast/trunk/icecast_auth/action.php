<? 
include "config.php";
include "db.php";
?>
<?
	$db = new DB($mysql_host, $mysql_user, $mysql_password, $mysql_db);
?>
<?

$mount = "";
$username = "";
$password = "";
$action = "";
$server = "";
$port = "";
$duration = "";

$listener_add = "listener_add";
$listener_remove = "listener_remove";
$mount_add = "mount_add";
$mount_remove = "mount_remove";

function insertAudit($db, $user, $server, $mount, $action) {
	global $table_prefix;
	$query = "insert into ".$table_prefix."audit (datestamp, username, server, mountpoint, action) values (NOW(), '".mysql_real_escape_string($user)."', '".mysql_real_escape_string($server)."', '".mysql_real_escape_string($mount)."', '".mysql_real_escape_string($action)."')";
	mysql_query($query, $db->link);
}

if (isset($_REQUEST["server"])) {
	$server=$_REQUEST["server"];
}
if (isset($_REQUEST["port"])) {
	$port=$_REQUEST["port"];
}

$servertext = "";
if (($server != "") && ($port != "")) {
	$servertext = "$server:$port";
}
if (isset($_REQUEST["duration"])) {
	$duration=$_REQUEST["duration"];
}
if (isset($_REQUEST["action"])) {
	$action=$_REQUEST["action"];
}
else {
	header('icecast-auth-user: 0');
	header('icecast-auth-message: action not supplied');
	exit;
}

if (isset($_REQUEST["mount"])) {
	$mount=$_REQUEST["mount"];
}
else {
	header('Response: 0');
	header('ResponseMessage: mount not supplied');
	exit;
}

if (isset($_REQUEST["user"])) {
	$username=$_REQUEST["user"];
}
else {
	if (($action == $listener_add) || ($action == $listener_remove)) {
		header('icecast-auth-user: 0');
		header('icecast-auth-message: username not supplied');
		exit;
	}
}

if (isset($_REQUEST["pass"])) {
	$password=$_REQUEST["pass"];
}
else {
	if (($action == $listener_add) || ($action == $listener_remove)) {
		header('icecast-auth-user: 0');
		header('icecast-auth-message: password not supplied');
		exit;
	}
}


if ($action == $listener_add) {
	$query = "select password, timeleft from ".$table_prefix."users where mountpoint = '".mysql_real_escape_string($mount)."' and username = '".mysql_real_escape_string($username)."'";
	$result = mysql_query($query, $db->link);
	$row = mysql_fetch_assoc($result);
	if ($row) {
		$pass = $row['password'];
		$time = (int)$row['timeleft'];
		if ($pass == $password) {
			if ($time != -1) {
				if ($time == 0) {
					header('icecast-auth-user: 0');
					header('icecast-auth-message: all time expired');
					//insertAudit($db, $username, $servertext, $mount, "Failed Auth - time expired");
					exit;
				}
				else {
					header('icecast-auth-user: 1');
					header('icecast-auth-message: OK');
					header("icecast-auth-timelimit: $time");
					insertAudit($db, $username, $servertext, $mount, "Listen Start - timelimit $time");
					exit;
				}
			}
			else {
				header('icecast-auth-user: 1');
				header('icecast-auth-message: OK');
				insertAudit($db, $username, $servertext, $mount, "Listen Start");
				exit;
			}
		}
		else {
			header('icecast-auth-user: 0');
			header('icecast-auth-message: invalid password');
			//insertAudit($db, $username, $servertext, $mount, "Failed Auth - invalid password");
			exit;
		}
	}
	else {
		$query = "select password, timeleft from ".$table_prefix."users where mountpoint = 'ALL' and username = '".mysql_real_escape_string($username)."'";
		$result = mysql_query($query, $db->link);
		$row = mysql_fetch_assoc($result);
		if ($row) {
			$pass = $row['password'];
			$time = 0;
			$time = $row['timeleft'];
			if ($pass == $password) {
				if ($time != -1) {
					if ($time == 0) {
						header('icecast-auth-user: 0');
						header('icecast-auth-message: all time expired');
						//insertAudit($db, $username, $servertext, $mount, "Failed Auth - time expired");
						exit;
					}
					else {
						header('icecast-auth-user: 1');
						header('icecast-auth-message: OK');
						header("icecast-auth-timelimit: $time");
						insertAudit($db, $username, $servertext, $mount, "Listen Start - timelimit $time");
						exit;
					}
				}
				else {
					header('icecast-auth-user: 1');
					header('icecast-auth-message: OK');
					insertAudit($db, $username, $servertext, $mount, "Listen Start");
					exit;
				}
			}
			else {
				header('icecast-auth-user: 0');
				header('icecast-auth-message: invalid password');
				//insertAudit($db, $username, $servertext, $mount, "Failed Auth - invalid password");
				exit;
			}
		}
		else {
			header('icecast-auth-user: 0');
			header('icecast-auth-message: invalid user');
			//insertAudit($db, $username, $servertext, $mount, "Failed Auth - invalid user");
			exit;
		}
	}
}
if ($action == $listener_remove) {

	if ($duration == "") {
		header('icecast-auth-user: 0');
		header('icecast-auth-message: missing duration');
		insertAudit($db, $username, $servertext, $mount, "Listener Remove");
		exit;
	}
	$timespentlistening = 0;
	$timespentlistening = (int)$duration;

	$query = "select timeleft from ".$table_prefix."users where mountpoint = '".mysql_real_escape_string($mount)."' and username = '".mysql_real_escape_string($username)."'";
	$result = mysql_query($query, $db->link);
	$row = mysql_fetch_assoc($result);
	if ($row) {
		$time = 0;
		$time = (int)$row['timeleft'];
		$timeremaining = $time - $timespentlistening;
		if ($timeremaining < 0) {
			$timeremaining = 0;
		}
		if ($time == -1) {
			$timeremaining = -1;
		}
		$query2 = "update ".$table_prefix."users set timeleft = $timeremaining where mountpoint = '".mysql_real_escape_string($mount)."' and username = '".mysql_real_escape_string($username)."'";
		$result2 = mysql_query($query2, $db->link);
		insertAudit($db, $username, $servertext, $mount, "Listener Remove ($duration secs)");
	}
	else {
		$query = "select timeleft from ".$table_prefix."users where mountpoint = 'ALL' and username = '".mysql_real_escape_string($username)."'";
		$result = mysql_query($query, $db->link);
		$row = mysql_fetch_assoc($result);
		if ($row) {
			$time = 0;
			$time = (int)$row['timeleft'];
			$timeremaining = $time - $timespentlistening;
			if ($timeremaining < 0) {
				$timeremaining = 0;
			}
			if ($time == -1) {
				$timeremaining = -1;
			}
			$query2 = "update ".$table_prefix."users set timeleft = $timeremaining where mountpoint = 'ALL' and username = '".mysql_real_escape_string($username)."'";
			$result2 = mysql_query($query2, $db->link);
			insertAudit($db, $username, $servertext, $mount, "Listener Remove ($duration secs)");
		}
		else {
			header('icecast-auth-user: 0');
			header('icecast-auth-message: invalid user');
			//insertAudit($db, $username, $servertext, $mount, "Failed Listener Remove - invalid user");
			exit;
		}
	}
}
if ($action == $mount_add) {
	insertAudit($db, "", $servertext, $mount, "Mount connect");
	header('icecast-auth-message: OK');
}
if ($action == $mount_remove) {
	insertAudit($db, "", $servertext, $mount, "Mount disconnect");
	header('icecast-auth-message: OK');
}
?>
