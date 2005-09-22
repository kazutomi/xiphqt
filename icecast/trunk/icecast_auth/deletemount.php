<? include "header.php"; ?>
<? include "db.php"; ?>
<?
	$db = new DB($mysql_host, $mysql_user, $mysql_password, $mysql_db);
?>
<h2>Delete Mountpoint</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<p>
<?

$reassign = "";
if (isset($_REQUEST["reassign"])) {
	$reassign=$_REQUEST["reassign"];
}

$displayform = true;
if (isset($_REQUEST["mount"])) {
	$mount=$_REQUEST["mount"];
	if (isset($_REQUEST["confirm"])) {
		$confirm=$_REQUEST["confirm"];
		if ($confirm == "1") {
			$displayform = false;
			
			$query = sprintf("delete from ".$table_prefix."mountpoints where mountpoint_name = '%s'",  mysql_real_escape_string($mount));
			if (mysql_query($query, $db->link)) {
				echo "<i>Deleted mountpoint $mount</i><br>";
			}
			else {
				echo "<font color=red>Error deleting mountpoint $mount : ".mysql_error()."</font><br>";
			}
			if ($reassign != '') {
				$query = sprintf("update users set ".$table_prefix."mountpoint = '%s' where mountpoint = '%s'",  mysql_real_escape_string($reassign), mysql_real_escape_string($mount));
				$result = mysql_query($query, $db->link);
				if (!$result) {
					echo "<font color=red>Error reassigning users from $mount to $reassign : ".mysql_error()."</font><br>";
				}
				else {
					$users_updated = mysql_affected_rows();
					echo "<i> $users_updated users re-assigned from $mount to mountpoint $reassign</i><br>";
				}
			}			
			else {
				echo "<i> No users re-assigned.</i><br>";
			}
		}
	}
}
?>
<?
if ($displayform) {
?>
This will delete the mountpoint <? echo $mount; ?> and optionally re-assign all users subscribed to this mountpoint to another mountpoint
</p>
<p>
<font color=red>THIS WILL REMOVE THE MOUNTPOINT. PLEASE MAKE SURE YOU WANT TO DO THIS.</font>
</p>
<p>
Delete <? echo $mount; ?><br>
<form method=GET action="deletemount.php">
<input type=hidden name="confirm" value="1">
<input type=hidden name="mount" value="<? echo "$mount"; ?>">
Re-assign to mountpoint: 
<select name="reassign">
<option value='' selected>Do not reassign
<?
$query = "select ".$table_prefix."mountpoint_name from mountpoints";
$result = mysql_query($query, $db->link);
while ($row = mysql_fetch_assoc($result)) {
   $mount = $row['mountpoint_name'];
	echo "<option value='$mount'>$mount";
}
mysql_free_result($result);
?>
</select>
<br>
<br>
<input type="Submit" value="Delete">
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
