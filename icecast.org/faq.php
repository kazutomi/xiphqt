<? include "common/header.php"; ?>
<h2>FAQ</h2>
<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast FAQ</h3>
<h2>General Questions</h2>
<p>What is Icecast? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
Icecast, the project, is a collection of programs and libraries for streaming audio over the Internet. This includes: 

<li>icecast, a program that streams audio data to listeners 
<li>libshout, a library for communicating with Icecast servers 
<li>IceS, a program that sends audio data to Icecast servers 
</td></tr></table>

<p>What is icecast, the program? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
icecast streams audio to listeners, and is compatible with Nullsoft's Shoutcast.
</td></tr></table>

<p>What is libshout? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
From the README: <br><br>

libshout is a library for communicating with and sending data to an icecast server. It handles the socket connection, the timing of the data, and prevents bad data from getting to the icecast server. 
</td></tr></table>

<p>What is IceS? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
IceS is a program that sends audio data to an icecast server to broadcast to clients. IceS can either read audio data from disk, such as from Ogg Vorbis files, or sample live audio from a sound card and encode it on the fly.
</td></tr></table>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br>
<br>

<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast FAQ</h3>
<h2>Setup</h2>
<p>What platforms are supported? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
Icecast is being developed on Linux and Windows, and is being tested with major Unices. The TESTED file contains a listing of platforms tested. 
</td></tr></table>

<p>How do I set up Icecast? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
Go to the Icecast Download Page and follow the instructions there. 
</td></tr></table>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br>
<br>

<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast FAQ</h3>
<h2>Administration</h2>
<p>How can I view the stream status page? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
Check your icecast configuration file for an element called <webroot>. This directory contains web stuff. In it, place a file called <i>status.xsl</i> that transforms an XML file containing stream data into a web page (either XHTML or HTML). 

There are sample XSL stylesheets available in icecast/web/ in the CVS distribution of icecast. 

In addition, the web directory can hold multiple status transforms, if you can't decide which one you want. 
</td></tr></table>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br>
<br>

<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast FAQ</h3>
<h2>Interoperability</h2>
<p>What can I use to listen to an Icecast stream? </p>
<table border=0 width=90% cellpadding=10>
<tr><td width=25></td><td bgcolor="#222222">
We maintain a list of Icecast-compatible audio players <a href="3rdparty.php">here</a>. 
</td></tr></table>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br>
<br>
<? include "common/footer.php"; ?>


