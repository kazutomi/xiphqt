<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
   "http://www.w3.org/TR/html4/loose.dtd">
<?
	$iceroot = "/";
	$dirroot = "http://dir.xiph.org/";
?>
<html>
<head>
<title>Icecast.org</title>
<link rel="stylesheet" type="text/css" href="common/style.css" />
</head>
<body bgcolor="black">
<table width="100%" border=0 cellpadding=0 cellspacing=0 >
<tr>
<td class="xiphnav"><a class="xiphnav_a" href="http://www.xiph.org/">XIPH.ORG</a></td>
<td class="xiphnav"><a class="xiphnav_a" href="http://www.vorbis.com/">VORBIS.COM</a></td>
<td class="xiphnav"><a class="xiphnav_a" href="http://www.icecast.org/">ICECAST.ORG</a></td>
<td class="xiphnav"><a class="xiphnav_a" href="http://www.speex.org/">SPEEX.ORG</a></td>
<td class="xiphnav" align="right">Open Standards for Internet Multimedia</td>
</tr>
</table>
<table width="100%" border=0 cellpadding=0 cellspacing=0 bgcolor=black>
<tr><td colspan=3><img alt="Icecast Logo" src="images/icecast.png"></td></tr>
<tr><td colspan=3 bgcolor="#7B96C6" height=3><img alt="Icecast Logo" src="images/blue.png" height=3></td></tr>
<tr>
<td colspan=3 bgcolor="black">
	<center>
	<table border=0 cellpadding=1 cellspacing=3 width="90%">
	<tr>
	    <td align=center >
	    <a class="nav" href="<? print $iceroot; ?>index.php">Home</a>&nbsp;&nbsp;|&nbsp;&nbsp;  
	    <a class="nav" href="<? print $iceroot; ?>download.php">Download</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $iceroot; ?>svn.php">Subversion</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $dirroot; ?>index.php">Stream Directory</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $iceroot; ?>docs.php">Docs</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $iceroot; ?>3rdparty.php">3rd Party Apps</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $iceroot; ?>ices.php">Ices</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $iceroot; ?>mailinglist.php">Mailing List</a>&nbsp;&nbsp;|&nbsp;&nbsp;
	    <a class="nav" href="<? print $iceroot; ?>contact.php">Contact</a>
	    </td></tr>
	</table>
	</center>
</td></tr>
<tr><td colspan=3 bgcolor="#7B96C6" height=3><img alt="Icecast Logo" src="images/blue.png" height=3></td></tr>
<tr><td colspan=3><br><br></td></tr>
<tr>
<?
if ($nopad == 1) {
}
else {
?>
<td width=50><br></td>
<?
}
?>
<td>

