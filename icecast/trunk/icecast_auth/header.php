<?
  include "config.php";
  session_start();

  $currentPage = basename($_SERVER['PHP_SELF']);
  if (($currentPage != "login.php") && ($currentPage != "error.php")) {
	  if (!isset($_SESSION['loggedin'])) {
		  header("Location: login.php");
	  }
  }

?>
  <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
   "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Icecast Authentication Manager</title>
<link rel="stylesheet" type="text/css" href="style.css" />
</head>
<body bgcolor="black">
<img src="images/icecast_logo.gif" align="middle"><font face="verdana" size=5 color="white">Iceast Authentication Manager</font>
<table width="100%" border=0 cellpadding=0 cellspacing=0 bgcolor=black>
<tr><td colspan=3 bgcolor="#7B96C6" height=3><img alt="Icecast Logo" src="images/blue.png" height=3></td></tr>
<tr>
<td colspan=3 bgcolor="black">
	<table border=0 cellpadding=1 cellspacing=3 width="90%">
	<tr>
	    <td align=left >
	    <table width=100% border=0 cellpadding=0 cellspacing=0>
	    <tr><td><a class="nav" href="index.php">Home</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="mountpoints.php">Mountpoints</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="adduser.php">Add User</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="reports.php">Reports</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="info.php">Info</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="logout.php">Logout</a>
	    </td>
	    <td align=right>
	    <form method=GET action="listusers.php">Search for a user : <input type="text" name="username" size="15"><input type="Submit" value="Search"></form>
	    </td></tr>
	    </table>
	    </td></tr>
	</table>
</td></tr>
<tr><td colspan=3 bgcolor="#7B96C6" height=3><img alt="Icecast Logo" src="images/blue.png" height=3></td></tr>
<tr><td colspan=3><br><br></td></tr>
<tr>
<td width=50><br></td>
<td>

