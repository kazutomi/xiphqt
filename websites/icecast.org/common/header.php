<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
   "http://www.w3.org/TR/html4/loose.dtd">
<?
	$iceroot = "/";
	$dirroot = "http://dir.xiph.org/";
?>
<html>
<head>
<style>
#xiphbar {
	background-color: #666666;
	min-height: 29px;
}

#xiphbar div {
	width: 50em;
	margin-left: auto;
	margin-right: auto;
	min-width: 800px;
}


#xiphbar img {
	margin: 0 0 0 0;
	border: 0;
	padding: .4em 0 0 12px;
	float: left;
	text-transform: lowercase;
	width: 231;
	height: 17px;
}

#xiphbar ul {
	margin: 0;
	border: 0;
	padding: .23em 12px .45em 0;
	text-align: right;
	text-transform: uppercase;
}

#xiphbar li {
	margin: 0;
	border: 0;
	padding: 0 10px 0 0;
	display: inline;
	list-style: none;
}

#xiphbar li a {
	color: #FFCC66;
	text-decoration: none;
	font-family: Arial, Helvetica, sans-serif;
	font-size: 10px;
}

#xiphbar li a:hover {
	color: #FFE6B3;
}
</style>
<title>Icecast.org</title>
<link rel="stylesheet" type="text/css" href="common/style.css" />
</head>
<body bgcolor="black">
<div id="xiphbar_outer">
<table id="xiphbar" border="0" cellpadding="0" cellspacing="0">
<tr>

  <td>
    <img id="xiphbanner" src="images/xiphbar.png" alt="Xiph logo" width="231" height="17"/>
  </td>

  <td id="xiphlinks" align="right">

    <a href="http://www.xiph.org/">Xiph.org</a>
    <a href="http://www.vorbis.com/">Vorbis</a>
    <a href="http://www.theora.org/">Theora</a>
    <a href="http://www.icecast.org/">Icecast</a>
    <a href="http://www.speex.org/">Speex</a>
    <a href="http://flac.sourceforge.net/">FLAC</a>

  </td>

</tr>
</table>
</div>
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
	    <a class="nav" href="http://forum.icecast.org/">Forums</a>&nbsp;&nbsp;|&nbsp;&nbsp;
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

