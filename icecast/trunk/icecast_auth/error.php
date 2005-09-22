<? include "header.php"; ?>
<h2>Error</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<p>
	<br><font color="red">
<?
	$errorCode = $_REQUEST['errorCode'];
	if ($errorCode == 1) {
		$errorMessage = "Invalid username or password";
	}
	echo "$errorMessage";
?>
	</font>
	<br>
	<br>
</p>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<? include "footer.php"; ?>