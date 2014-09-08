<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-15"/>
  <link rel="icon" href="//www.xiph.org/images/logos/xiph.ico" type="image/x-icon"/>
  <link rel="shortcut icon" href="//www.xiph.org/images/logos/xiph.ico" type="image/x-icon"/>
  <link rel="stylesheet" title="Default 50em Width" media="screen" href="//www.xiph.org/css/screen.css" type="text/css"/>
  <link rel="alternate stylesheet" title="Xiph.org 50em Width" media="screen" href="//www.xiph.org/css/screen-2.css" type="text/css"/>

<!-- Enter custom page information and styles here -->
  <title>Xiph.org</title>

<style type="text/css">
<!--
.newslist {
	margin: 0 0 2.64em 0;
	padding: 0;
}

.newslist li {
	margin-bottom: .72em;
	list-style: none;
	font-size: .94em;
	line-height: 1.4;
}

.newslist a {
	color: #3366CC;
}

.xiph {
	color: #FF9900;
	font-weight: bold;
}

.resources ul {
	margin: 0;
	padding: 0;
}

.resources li {
	margin-bottom: .4em;
	list-style: none;
	font-size: .94em;
	line-height: 1.4;
}
-->
</style>
</head>
<body>
<div id="xiphbar_outer" style="@import url(/common/xiphbar.css);">
<table id="xiphbar" border="0" cellpadding="0" cellspacing="0">
<tr>

  <td>
    <img id="xiphbanner" src="//xiph.org/images/logos/xiphbar.png" 
         alt="Xiph logo" width="231" height="17"/>
  </td>

  <td id="xiphlinks" align="right">

    <a href="https://www.xiph.org/">Xiph.org</a>
    <a href="http://www.vorbis.com/">Vorbis</a>
    <a href="http://www.theora.org/">Theora</a>
    <a href="http://www.icecast.org/">Icecast</a>
    <a href="http://www.speex.org/">Speex</a>
    <a href="https://xiph.org/flac/">FLAC</a>

    <a href="http://www.xspf.org/">XSPF</a>
  </td>

</tr>
</table>
</div>


<div id="thepage"><h2>people.xiph.org - personal webspace of the xiphs</h2>
<?php
$home = opendir('/home');
$users = array();
while($user = readdir($home)) {
  if($user[0] != '.') {
    if(is_dir("/home/$user/public_html")) {
    	$users[] = $user;
    }
  }
}
asort($users);
echo "<ul>";
foreach($users as $user) {
  echo '<li><a href="/~'.$user.'">'.$user.'</a></li>'."\n";
}
echo "</ul>";
?>
</div>
<div id="copyright">
  <p>The Xiph Fish Logo and the Vorbis.com many-fish logos are 
  trademarks (&trade;) of Xiph.Org.<br/>

  These pages &copy; 1994 - 2005 Xiph.Org. All rights reserved.</p>

  <p style="color:#FFFFFF;">Last Modified: November 25, 2005</p>
</div></body>
