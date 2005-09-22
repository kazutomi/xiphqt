<? include "header.php"; ?>
<? include "db.php"; ?>
<?
	$db = new DB($mysql_host, $mysql_user, $mysql_password, $mysql_db);
?>
<h2>Mountpoints</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Mountpoint Listing</h3>
<p>
<?

if (isset($_REQUEST["action"])) {
	$action=$_REQUEST["action"];
	if ($action == "add") {
		if (isset($_REQUEST["mount"])) {
			$mount=$_REQUEST["mount"];
			if (!ereg("^/", $mount)) {
				echo "<font color=red>Error adding mountpoint $mount : mountpoint must begin with a slash (/)</font><br>";
			}
			else {
				$query = sprintf("insert into ".$table_prefix."mountpoints values ('%s')",  mysql_real_escape_string($mount));
				if (mysql_query($query, $db->link)) {
					echo "<i>Added mountpoint $mount</i><br>";
				}
				else {
					echo "<font color=red>Error adding mountpoint $mount : ".mysql_error()."</font><br>";
				}
			}
		}
	}
}

?>
<table border=0 width=100%>
<?
$query = "select mountpoint_name from ".$table_prefix."mountpoints";
$result = mysql_query($query, $db->link);
while ($row = mysql_fetch_assoc($result)) {
   $mount = $row['mountpoint_name'];
   $subquery = "select count(*) from ".$table_prefix."users where mountpoint = '$mount' or mountpoint = 'ALL'";
   $subresult = mysql_query($subquery, $db->link);
   $subrow = mysql_fetch_row($subresult);
   mysql_free_result($subresult);
   $usercount = $subrow[0];
   
   echo "<tr><td>$mount</td><td><a href='adduser.php?mount=".urlencode($mount)."'>Add user</a>&nbsp;&nbsp;&nbsp;<a href='listusers.php?mount=".urlencode($mount)."'>List users</a>&nbsp;<a href='reports.php?mount=".urlencode($mount)."'>Get Report</a></td><td>Users: $usercount<td><td><a href='deletemount.php?mount=$mount'>Delete</a></td></tr>";
}
mysql_free_result($result);
?>
</table>
	<br>
<form method=GET action="mountpoints.php">
<input type=hidden name="action" value="add">
Add new mountpoint <input type=text name="mount" size=20>&nbsp;&nbsp;<input type="submit" value="Add">
</form>
	<br>
	<br>
</p>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>	
</div>
<? include "footer.php"; ?>
