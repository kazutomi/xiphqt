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
 It began as the successor of the old &quot;shout&quot; utility, and has since gained
 a lot of useful features.
</p>

<p>
 In its basic mode of operation, it streams media files or data from standard
 input without reencoding and thus requires only very little CPU resources.
 It can also use various external decoders and encoders to reencode from one
 format to another, and stream the result to an Icecast server.
 Additional features include scriptable playlist and metadata handling.
 All of its features make ezstream a very flexible source client.
</p>

<p>
 Supported media formats for streaming are MP3, Ogg Vorbis and Ogg Theora.
 Native metadata support includes MP3 (ID3v1 only) and Ogg Vorbis, and many
 more formats when the optional TagLib support has been compiled in.
</p>

<p>
 Ezstream is free software and licensed under the GNU General Public License.
</p>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a> |
 <a href="#ez_support">Support</a>
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
 Latest version: <b>0.5.6</b>
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
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.5.6.tar.gz">ezstream-0.5.6.tar.gz</a><br/>
    MD5: 1be68119d44fbe71454a901fa650a359
   </td>
  </tr>
  <tr>
   <td>
    MS Windows binary (32bit)
   </td>
   <td>
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.5.6-win32.zip">ezstream-0.5.6-win32.zip</a><br/>
    MD5: 19c6f28c2de81e6e2fa3e0ed914e6696
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
 <a href="#ez_relnotes">Release Notes</a> |
 <a href="#ez_support">Support</a>
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

<p>
 Ezstream depends on:
</p>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  libshout 2.2.x or later + libshout dependencies<br/>
  <a href="http://www.icecast.org/download.php">http://www.icecast.org/download.php</a>
 </li>
 <li>
  libxml 2.x<br/>
  <a href="http://xmlsoft.org">http://xmlsoft.org</a>
 </li>
</ul>

<p>
 Ezstream optionally uses:
</p>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  For reading metadata from Ogg Vorbis files:
  <ul>
   <li>
    TagLib 1.x (1.4 or newer recommended)<br/>
    <a href="http://developer.kde.org/~wheeler/taglib.html">http://developer.kde.org/~wheeler/taglib.html</a>
   </li>
   <b>or</b>:
   <li>
    libvorbis 1.x<br/>
    <a href="http://www.vorbis.com/">http://www.vorbis.com/</a>
   </li>
  </ul>
 </li>
 <li>
  For basic non-ASCII charset support in metadata and filenames:
  <ul>
   <li>
    GNU libiconv, if <code>iconv()</code> is not available in the system C
    library.<br/>
    <a href="http://www.gnu.org/software/libiconv/">http://www.gnu.org/software/libiconv/</a>
   </li>
  </ul>
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

<p>
 It might be a good idea to check whether the operating system vendor provides
 a binary package of a recent ezstream release and install that one instead of
 building from source.
</p>

<h4>Windows</h4>

<p>
 Windows users can simply copy the <code>ezstream.exe</code> file from the
 binary distribution archive (.ZIP) to any location of their choosing.
 Users, who wish to be able to run ezstream anywhere on their system, can add
 the directory that contains ezstream.exe to their PATH environment variable.
</p>

<p>
 The ezstream binary release depends on MSVC 2008 runtime libraries.
 These libraries can be downloaded from Microsoft as the
 <a href="http://www.microsoft.com/DOWNLOADS/details.aspx?familyid=A5C84275-3B97-4AB7-A40D-3802B2AF5FC2"><code>vcredist_x86.exe</code></a>
 file, at no cost.
</p>

<h4>Even more ...</h4>

<p>
 Comprehensive documentation of ezstream and its configuration is included in
 the man page <code>ezstream(1)</code>.
</p>

<p>
 Those, who are interested in how ezstream development progresses, can stay
 informed by occasionally looking at the
 <a href="http://svn.xiph.org/trunk/ezstream/NEWS">NEWS file</a>.
 It is kept up-to-date with important changes in ezstream as they happen.
</p>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a> |
 <a href="#ez_support">Support</a>
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
<div class="newscontent" id="ez_relnotes" name="ez_relnotes">

<h3>Release Notes</h3>

<p>
 Ezstream 0.5.6 has been released on August 31st 2009.
</p>

<p>
 Version 0.5.6 is a maintenance and feature release.
</p>

<h4 id="ez_relnotes_chgs" name="ez_relnotes_chgs">Changes</h4>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  <code>ezstream-file.sh</code>:
  <ul>
   <li>
     NEW &mdash;
     Add and install ezstream-file.sh, a new convenience shell script that
     generates configuration and playlist from a template and command line,
     respectively.
     (Bourne shell and POSIX utilities required.)
   </li>
  </ul>
 <li>
  <code>src/ezstream.c</code>:
  <ul>
   <li>
    MISC &mdash;
    Be more tolerant towards faulty playlists and similar issues.
    Failure to open a resource (e.g. a media file) is no longer fatal and
    operation will continue until 100 subsequent errors.
    Based on an idea from dhorton.
    (Ticket #<a href="https://trac.xiph.org/ticket/1585"><code>1585</code></a>)
   </li>
   <li>
    NEW &mdash;
    New command line option -s: Make ezstream function as a line-based
    shuffling utility.
   </li>
  </ul>
 </li>
 <li>
  <code>src/playlist.c</code>:
  <ul>
   <li>
    MISC &mdash;
    Consider no output from a playlist program to be equivalent to an empty
    line, indicating that the end of the playlist is reached.
   </li>
   <li>
    FIX &mdash;
    Do not complain when receiving an empty line from a playlist program.
   </li>
  </ul>
 </li>
 <li>
  <code>examples/</code>:
  <ul>
   <li>
    NEW &mdash;
    Add a real-world example playlist script with logging feature.
   </li>
  </ul>
</ul>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a> |
 <a href="#ez_support">Support</a>
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
<div class="newscontent" id="ez_support" name="ez_support">

<h3>Support</h3>

<h4>Reporting Problems</h4>

<p>
 In case of problems with ezstream, please make sure to re-read the
 documentation first and double-check your configuration.
 A lot of effort has been put into the <code>ezstream(1)</code> manual, and it
 should &mdash; at least in theory &mdash; answer all questions.
</p>

<p>
 Of course, ezstream isn't immune to bugs.
 After
 <a href="https://trac.xiph.org/report/21">checking if your bug is already known</a>,
 or a newer ezstream release fixes your problem, it is much appreciated that you
 <a href="https://trac.xiph.org/newticket?component=ezstream">file your bug report</a>
 with the Xiph.org bug tracker.
</p>

<p>
 Please be as specific and verbose as necessary when submitting a problem
 report.
 Also, please include a valid e-Mail address when reporting an issue, in case
 there are follow-up questions.
</p>

<p>
 Your contribution and efforts are very much appreciated.
 Thank you!
</p>

<h4>Contact</h4>

<p>
 The current maintainer of ezstream is Moritz Grimm (mdgrimm at gmx dot net).
</p>

<p class="ez_nav">
 <a href="#ez_about">About</a> |
 <a href="#ez_download">Download</a> |
 <a href="#ez_more">More ...</a> |
 <a href="#ez_relnotes">Release Notes</a> |
 <a href="#ez_support">Support</a>
</p>

</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>

<br/>

<? include "common/footer.php"; ?>
