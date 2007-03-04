<? include "common/header.php"; ?>
<h2>Ezstream</h2>

<div class="roundcont">

<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>

<div class="newscontent">

<h3>About Ezstream</h3>

<p>
 Ezstream is a command line source client for Icecast media streaming servers.
 It began as the successor of the old "shout" utility, and has since gained
 a lot of useful features.
</p>

<p>
 In its basic mode of operation, it streams media files or data from standard
 input without reencoding and thus requires only very little CPU resources.
 It can also use various external decoders and encoders to reencode from one
 format to another, and stream the result to an Icecast server.
 With reencoding enabled, ezstream is a very flexible source client.
</p>

<p>
 Supported media formats for streaming are MP3, Ogg Vorbis and Ogg Theora.
 Metadata support is available for MP3 (ID3v1 only) and Ogg Vorbis.
</p>

<p>
 Ezstream is free software and licensed under the GNU General Public License.
</p>

</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br/>

<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">

<h3>Download</h3>

<center>
 <table border=1 cellpadding=5 cellspacing=0 width=90% bordercolor="#000000">
  <tr>
   <td><b>Package</b></td><td><b>Download Link</b></td>
  </tr>
  <tr>
   <td>
    Source .tar.gz (all platforms)
   </td>
   <td>
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.2.1.tar.gz">ezstream-0.2.1.tar.gz</a>
   </td>
  </tr>
  <tr>
   <td>
    Windows NT/2000/XP binary
   </td>
   <td>
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream_win32_0.2.1_setup.exe">ezstream_win32_0.2.1_setup.exe</a>
   </td>
  </tr>
 </table>
</center>

<p>
 Older releases can be found at
 <a href="http://downloads.xiph.org/releases/ezstream/">http://downloads.xiph.org/releases/ezstream/</a>.
</p>

</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br/>

<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">

<h3>Prerequisites and Installation</h3>

<p>
 Ezstream depends on:
</p>

<ul style="font-size: smaller; margin: 1em 2em">
 <li>
  libshout 2.2.x
  (<a href="http://www.icecast.org/download.php">http://www.icecast.org/download.php</a>)
 </li>
 <li>
  Dependencies of libshout, such as libogg, libvorbis, libtheora, etc.
  (<a href="http://www.vorbis.com/">http://www.vorbis.com/</a> and
  <a href="http://www.theora.org/">http://www.theora.org/</a>)
 </li>
 <li>
  libxml 2.x
  (<a href="http://xmlsoft.org">http://xmlsoft.org</a>)
 </li>
</ul>

<p>
 On UNIX and UNIX-like systems, such as Linux and *BSD, binary packages for the
 required libraries are usually available.
 To compile ezstream, ensure that the respective <em>-devel</em> packages are
 installed as well, if applicable.
 Further steps are explained in the <em>README</em> and <em>INSTALL</em> files
 inside the distribution package.
</p>

</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br/>

<? include "common/footer.php"; ?>
