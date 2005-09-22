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

$showall = "";
if (isset($_REQUEST["showall"])) {
	$showall=$_REQUEST["showall"];
}
$showusers = "";
if (isset($_REQUEST["showusers"])) {
	$showusers=$_REQUEST["showusers"];
}

?>
<h2>Reports</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<?
if (($mount == "") && ($username == "")) {
?>
Pick a report :<br>
<form method=GET action="reports.php">
<h3>By Mountpoint</h3>
<p>
Select Mountpoint : 
<select name="mount">
<?
$query = "select mountpoint_name from ".$table_prefix."mountpoints";
$result = mysql_query($query, $db->link);
while ($row = mysql_fetch_assoc($result)) {
    $availablemount = $row['mountpoint_name'];
    echo "<option value='$availablemount'>$availablemount";
}
mysql_free_result($result);
?>
</select>
<input type="Submit">
</p>
</form>
<form method=GET action="reports.php">
<h3>By Mountpoint</h3>
<p>
Select User : <input type="text" name="username" size=30>
<input type="Submit">
</p>
</form>
<?
}
else {
	if ($username != "") {
		// Username Report
		echo "<h3>User Report for $username</h3>";
		echo "<b>Registered Mountpoints</b><br><br>";
		echo "<table border=1 cellpadding=10 cellspacing=0>";
		echo "<tr><td bgcolor=black>Mountpoint</td><td bgcolor=black>Time Restrictions</td></tr>";
		$query = "select mountpoint, timeleft from ".$table_prefix."users where username = '".mysql_real_escape_string($username)."'";
		$result = mysql_query($query, $db->link);
		while ($row = mysql_fetch_assoc($result)) {
			$mountpoint = $row['mountpoint'];
			$timeleft = 0;
			$timeleft = $row['timeleft'];
			if ($timeleft != "-1") {
				$timerestriction = "$timeleft seconds left";
			}
			else {
				$timerestriction = "No time restrictions";
			}
			echo "<tr><td>$mountpoint</td><td>$timerestriction</td></tr>";
		}
		echo "</table>";
		mysql_free_result($result);

		echo "<br><br>";
		echo "<b>Audit Entries (last 20) Click <a href='reports.php?username=".urlencode($username)."&showall=1'>here</a> for a full report</b><br><br>";
		$limit = "";

		if ($showall == "") {
			$limit = "limit 20";
		}
		$query = "select count(*) numentries from ".$table_prefix."audit where username = '".mysql_real_escape_string($username)."'";
		$result = mysql_query($query, $db->link);
		$row = mysql_fetch_assoc($result);
		$entrycount = $row['numentries'];
		mysql_free_result($result);

		echo "<font color=yellow>$entrycount</font> total audit entries<br><br>";
		echo "<table width=100% border=1 cellpadding=10 cellspacing=0>";
		echo "<tr><td bgcolor=black>Date</td><td bgcolor=black>Server/Mount</td><td bgcolor=black>Action</td></tr>";
		$query = "select datestamp, server, mountpoint, action from ".$table_prefix."audit where username = '".mysql_real_escape_string($username)."' order by datestamp desc $limit";
		$result = mysql_query($query, $db->link);
		while ($row = mysql_fetch_assoc($result)) {
			$datestamp = $row['datestamp'];
			$server = $row['server'];
			$mountpoint = $row['mountpoint'];
			$action = $row['action'];
			echo "<tr><td>$datestamp</td><td>$server$mountpoint</td><td>$action</td></tr>";
		}
		echo "</table>";
		mysql_free_result($result);

		echo "</p>";
	}
	if ($mount != "") {
		// Mountpoint report
		echo "<h3>Mountpoint Report for $mount</h3>";
		$query = "select count(*) entrycount from ".$table_prefix."users where mountpoint = '".mysql_real_escape_string($mount)."' or mountpoint = 'ALL'";
		$result = mysql_query($query, $db->link);
		$row = mysql_fetch_assoc($result);
		$entrycount = $row['entrycount'];
		mysql_free_result($result);
		echo "<b>Registered Listeners : <a href='listusers.php?mount=".urlencode($mount)."'><font color=yellow>$entrycount</font></a></b><br><br>";

		echo "<br>";
		if ($showusers == "") {
			$query = "select count(*) entrycount from ".$table_prefix."audit where username = '' and mountpoint = '".mysql_real_escape_string($mount)."'";
			$result = mysql_query($query, $db->link);
			$row = mysql_fetch_assoc($result);
			$entrycount = $row['entrycount'];
			mysql_free_result($result);

			echo "Click <a href='reports.php?showusers=1&mount=".urlencode($mount)."'>here</a> for a listener audit report<br><br>";
			echo "<b>Mount Connect/Disconnect Audit Entries (last 20) Click <a href='reports.php?mount=".urlencode($mount)."&showall=1'>here</a> for a full report</b><br>";
			$limit = "";

			if ($showall == "") {
				$limit = "limit 20";
			}

			echo "<font color=yellow>$entrycount</font> total audit entries<br><br>";
			echo "<table width=100% border=1 cellpadding=10 cellspacing=0>";
			echo "<tr><td bgcolor=black>Date</td><td bgcolor=black>Server/Mount</td><td bgcolor=black>Action</td></tr>";
			$query = "select datestamp, server, mountpoint, action from ".$table_prefix."audit where username = '' and mountpoint = '".mysql_real_escape_string($mount)."' order by datestamp desc $limit";
			$result = mysql_query($query, $db->link);
			while ($row = mysql_fetch_assoc($result)) {
				$datestamp = $row['datestamp'];
				$server = $row['server'];
				$mountpoint = $row['mountpoint'];
				$action = $row['action'];
				echo "<tr><td>$datestamp</td><td>$server$mountpoint</td><td>$action</td></tr>";
			}
			echo "</table>";
			mysql_free_result($result);
		}
		else {
			$query = "select count(*) entrycount from ".$table_prefix."audit where username != '' and mountpoint = '".mysql_real_escape_string($mount)."'";
			$result = mysql_query($query, $db->link);
			$row = mysql_fetch_assoc($result);
			$entrycount = $row['entrycount'];

			mysql_free_result($result);
			echo "Click <a href='reports.php?mount=".urlencode($mount)."'>here</a> for a Mountpoint connect/disconnect audit report<br><br>";
			echo "<b>Listener Audit Entries (last 20) Click <a href='reports.php?showusers=1&mount=".urlencode($mount)."&showall=1'>here</a> for a full report</b><br>";
			$limit = "";

			if ($showall == "") {
				$limit = "limit 20";
			}

			echo "<font color=yellow>$entrycount</font> total audit entries<br><br>";
			echo "<table width=100% border=1 cellpadding=10 cellspacing=0>";
			echo "<tr><td bgcolor=black>Date</td><td bgcolor=black>Server/Mount</td><td bgcolor=black>User</td><td bgcolor=black>Action</td></tr>";
			$query = "select datestamp, username, server, mountpoint, action from ".$table_prefix."audit where username != '' and mountpoint = '".mysql_real_escape_string($mount)."' order by datestamp desc $limit";
			$result = mysql_query($query, $db->link);
			while ($row = mysql_fetch_assoc($result)) {
				$datestamp = $row['datestamp'];
				$server = $row['server'];
				$mountpoint = $row['mountpoint'];
				$action = $row['action'];
				$username = $row['username'];
				echo "<tr><td>$datestamp</td><td>$server$mountpoint</td><td>$username</td><td>$action</td></tr>";
			}
			echo "</table>";
			mysql_free_result($result);
		}

		echo "</p>";
	}
}
?>
	<br>
</p>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>	
</div>
<? include "footer.php"; ?>
