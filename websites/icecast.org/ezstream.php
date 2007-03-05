<? include "common/header.php"; ?>

<style type="text/css">
 <!--
  code {
      font-size: 120%;
  }
  .ez_nav {
      margin: 10px 0px;
      font-family: Verdana, sans-serif;
      text-decoration: none;
      text-align: right;
      font-weight: bold;
      font-size: 100%;
      color: #F8EF64;
      border-top: 3px dashed #000000;
  }
  -->
</style>

<h2>Ezstream</h2>

<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent" id="ez_about" name="ez_about">

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

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a>
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
<div class="newscontent" id="ez_download" name="ez_download">

<h3>Download</h3>

<p style="font-size: larger;">
 Latest version: <b>0.3.0</b>
 [ <a href="#ez_relnotes_chgs">Changes</a> ]
<p>

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
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.3.0.tar.gz">ezstream-0.3.0.tar.gz</a><br/>
    MD5: 7704b0a17799f3e8df557cd35a5f897a
   </td>
  </tr>
  <tr>
   <td>
    Windows NT/2000/XP binary
   </td>
   <td>
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.3.0-win32.zip">ezstream-0.3.0-win32.zip</a><br/>
    MD5: 9c18f8d95fbc032fd3fb26db53b0a28b
   </td>
  </tr>
 </table>
</center>

<p>
 Older releases can be found at
 <a href="http://downloads.xiph.org/releases/ezstream/">http://downloads.xiph.org/releases/ezstream/</a>.
</p>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a>
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
<div class="newscontent" id="ez_more" name="ez_more">

<h3>Further Information</h3>

<h4>Dependencies</h4>

<ul style="font-size: smaller; margin: 1em 3em">
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

<h4>UNIX (Linux, *BSD, Solaris, ...)</h4>

<p>
 On UNIX and UNIX-like systems, such as Linux and *BSD, binary packages for the
 required libraries are usually available.
 To compile ezstream, ensure that the respective <em>-devel</em> packages are
 installed as well, if applicable.
 Further steps, and more, are explained in the <em>README</em> and
 <em>INSTALL</em> files inside the distribution package.
</p>

<h4>Windows</h4>

<p>
 Windows users can simply copy the ezstream.exe file from the binary
 distribution archive (.ZIP) to any location of their choosing.
 As of version 0.3.0, the Win32 version of ezstream no longer requires any .DLL
 files and is entirely self-contained.
 Because of this simplicity, an installer is no longer provided.
 Users, who wish to be able to run ezstream anywhere on their system, can add
 the directory that contains ezstream.exe to their PATH environment variable.
</p>

<h4>Even more ...</h4>

<p>
 Comprehensive documentation of ezstream and its configuration is included in
 the man page <code>ezstream(1)</code>.
</p>

<p>
 Those, who are interested in how ezstream development progresses, can stay
 informed by occasionally looking at the
 <a href="http://svn.xiph.org/trunk/ezstream/NEWS">NEWS</a>
 file.
 It is kept up-to-date with important changes in ezstream as they happen.
</p>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a>
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
<div class="newscontent"  id="ez_relnotes" name="ez_relnotes">

<h3>Release Notes</h3>

<p>
 Ezstream 0.3.0 has been released on February 5th 2007.
</p>

<p>
 In this version, ezstream received a complete overhaul, with many new
 features, bug fixes, complete documentation, various other cleanups and
 improvements.
 The result is a better, more stable, more portable and more user-friendly
 ezstream that fits in a smaller distribution tarball.
 The binary distribution for Windows also no longer requires any .DLL files to
 function and comes as a stand-alone .EXE file.
</p>

<h4 id="ez_relnotes_chgs" name="ez_relnotes_chgs">Changes</h4>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  The original author, Ed Zaleski, hands over ezstream maintainership to Moritz
  Grimm.
 </li>
 <li>
  New Features:
  <ul>
   <li>
    Playlist shuffling support, enabled via the new
    <code>&lt;shuffle&gt;</code> configuration option.
   </li>
   <li>
    Playlist scripting support: Indicate that the executable in
    <code>&lt;filename&gt;</code> should be run each time to get a new media
    filename to stream, by setting the new <code>&lt;playlist_program&gt;</code>
    configuration option to 1.
   </li>
   <li>
    New <code>&lt;stream_once&gt;</code> configuration option, which makes
    ezstream play a media file or playlist once and then exit.
   </li>
   <li>
    New <code>&lt;reconnect_tries&gt;</code> option to restrict the number of
    reconnection attempts to a server in case the connection goes down.
   </li>
   <li>
    Add feature to skip the currently streaming track, done by sending the
    SIGUSR1 signal to the ezstream process.
   </li>
   <li>
    New command line option <code>-q</code>: Suppress standard error output
    from external de-/encoders.
   </li>
   <li>
    New command line option <code>-v</code>: Produce more verbose output.
   </li>
   <li>
    New "real-time" status display of the stream when both -q and -v are given
    on the command line.
   </li>
   <li>
    Thorough configuration file checks with helpful error messages.
   </li>
   <li>
    The <code>@M@</code> metadata placeholder is now supported in
    <code>&lt;decode&gt;</code>.
   </li>
   <li>
    Playlists may now have the '.txt' filename extension in addition to '.m3u'.
   </li>
  </ul>
 </li>
 <li>
  Fixes:
  <ul>
   <li>
    At least one stack and one heap overflow have been fixed.
   </li>
   <li>
    Several memory leaks have been fixed.
   </li>
   <li>
    Strict checking of the configuration file prevents unexpected behavior and
    adds another safeguard to prevent crashes.
   </li>
   <li>
    Fixed and more portable signal handling.
   </li>
  </ul>
 </li>
 <li>
  Miscellaneous, user-visible changes:
  <ul>
   <li>
   </li>
   <li>
    Improved documentation. Ezstream now has a comprehensive man page and the
    README file has been rewritten.
   </li>
   <li>
    Consistency improvements in the configuration examples. The examples are
    now also installed.
   </li>
   <li>
    The <code>&lt;format&gt;</code> and <code>&lt;match&gt;</code>
    configuration elements, as well as filename extension matching to detect
    playlists in the <code>&lt;filename&gt;</code> element, are now case
    insensitive.
   </li>
   <li>
    Various improvements in the build system and portability fixes.
   </li>
   <li>
    Consistent and more helpful messages from ezstream. By default, ezstream
    no longer clutters the screen with lots of output (some information that
    used to be available needs to be enabled with the new <code>-v</code>
    command line parameter.)
   </li>
   <li>
    Tweaked metadata string generation: no more lone " - " dashes at the
    beginning or end of the string.
   </li>
   <li>
    New command line option <code>-V</code>: Print the version number and exit.
   </li>
  </ul>
 </li>
</ul>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a>
</p>

</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br/>

<? include "common/footer.php"; ?>
