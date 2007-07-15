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
 Latest version: <b>0.4.0</b>
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
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.4.0.tar.gz">ezstream-0.4.0.tar.gz</a><br/>
    MD5: 6f3699847787a8b0f69b6b5eb5e9c150
   </td>
  </tr>
  <tr>
   <td>
    Windows NT/2000/XP binary
   </td>
   <td>
    <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.4.0-win32.zip">ezstream-0.4.0-win32.zip</a><br/>
    MD5: 0eb552c9bbca600d68559a28ceb97d13
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
 <li>
  <b>Optional</b>: Taglib 1.x
  (1.4 or newer recommended, will be used via the libtag_c wrapper)
  (<a href="http://developer.kde.org/~wheeler/taglib.html">http://developer.kde.org/~wheeler/taglib.html</a>)
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
 Ezstream 0.4.0 has been released on March 11th 2007.
</p>

<p>
 With version 0.4.0, ezstream gained many new features related to metadata.
 It can now use TagLib to read more information from media files. Also an
 external program or script can be used to set a stream's metadata information,
 which can be customized with a format string.
 Metadata updates from an external program can be triggered mid-stream with the
 SIGUSR2 signal. A couple of memory management fixes also went into this
 release.
</p>

<h4 id="ez_relnotes_chgs" name="ez_relnotes_chgs">Changes</h4>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  src/util.*:
  <ul>
   <li>
    FIX --
    The xfree() function did not work as intended due to a programming error.
    Found, and fix suggested, by Karl Heyes.
   </li>
  </ul>
 </li>
 <li>
  src/playlist.*:
  <ul>
   <li>
    FIX --
    An error compareable to the one about xfree() in src/util.* has been fixed.
   </li>
   <li>
    MISC --
    Refuse to execute a group- or world-writeable playlist script or program.
   </li>
  </ul>
 </li>
 <li>
  src/compat.c:
  <ul>
   <li>
    ADD --
    Implement a basename() function for Windows that behaves
    like the ones used on Unix systems.
   </li>
  </ul>
 </li>
 <li>
  win32/:
  <ul>
   <li>
    MISC --
    Remove the rather ugly HTML version of the man page from the distribution,
    and provide a much better PDF version instead.
   </li>
  </ul>
 </li>
 <li>
  various:
  <ul>
   <li>
    ADD --
    Allow ezstream to use TagLib for reading metadata from media files.
    TagLib (libtag_c) is now an optional dependency.
   </li>
   <li>
    ADD --
    When built with TagLib support, include the song length in the "real-time"
    information line, if available.
   </li>
   <li>
    ADD --
    New <code>&lt;metadata_progname&gt;</code> configuration option, which
    causes metadata to be read from the output of an external program or script.
   </li>
   <li>
    ADD --
    New runtime control via the <code>SIGUSR2</code> signal, which triggers
    reading of fresh metadata information from
    <code>&lt;metadata_progname&gt;</code> (metadata is always read at song
    changes.)
   </li>
   <li>
    ADD --
    New <code>&lt;metadata_format&gt;</code> configuration option, to customize
    metadata strings when used with the new
    <code>&lt;metadata_progname&gt;</code> feature.
   </li>
   <li>
    ADD --
    New '<code>@a@</code>' and '<code>@t@</code>' placeholders for separate
    artist and title metadata in de-/encoder commands.
   </li>
  </ul>
 </li>
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
 should -- at least in theory -- answer all questions.
</p>

<p>
 Of course, ezstream isn't immune to bugs.
 After
 <a href="https://trac.xiph.org/report/21">checking whether your bug is already known</a>,
 or a newer ezstream release fixes your problem, it is much appreciated that you
 <a href="https://trac.xiph.org/newticket?component=ezstream">file your <b>complete</b> bug report</a>
 (see below) with the Xiph.org bug tracker.
</p>

<p>
 If you encounter one of the following issues, you most definitely found a
 genuine bug:
</p>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  Ezstream crashes with a segmentation fault (SIGSEGV).
 </li>
 <li>
  Ezstream aborts with an &quot;Internal error&quot;.
 </li>
 <li>
  A long-lived ezstream process keeps requiring more and more resources until
  they run out.
 </li>
 <li>
  A newer version of ezstream behaves surprisingly different, compared to an
  earlier release, even though the configuration hasn't changed.
  Such a behavioral change would have to be clearly intended and noted in the
  <a href="http://svn.xiph.org/trunk/ezstream/NEWS">NEWS file</a>,
  or it is considered a bug.
 </li>
 <li>
  A part of the documentation is missing, unclear, or plain wrong.
 </li>
</ul>

<p>
 Of course, sensible feature requests are also welcome.
 Please select <em>enhancement</em> in the &quot;Type:&quot; drop-down menu for
 those reports.
</p>

<h4>The Good Bug Report</h4>

<p>
 Since it's hard to read someone else's mind, especially over the Internet,
 a bug report needs to be "good".
 A good bug report is never too short, and contains all the necessary
 information to allow the developer to research, reproduce, understand and fix
 the problem.
</p>

<p>
 Unless you are able to provide a fix (unified diff with explanation) to your
 problem, please follow this guide as closely as possible:
</p>

<ul style="font-size: smaller; margin: 1em 3em">
 <li>
  <em>Be precise.</em><br/>
  Vague phrases like &quot;It doesn't work&quot;, &quot;It crashes&quot; or
  &quot;There is a strange problem&quot; are not helpful without a verbose
  explanation.
  Do not be afraid of bug reports that seem too lengthy; it's a fact of life.
 </li>
 <li>
  <em>What are you using?</em><br/>
  Include information about the ezstream version, your operating system name,
  version and architecture.
 </li>
 <li>
  <em>What did you do?</em><br/>
  Include the exact sequence of steps and events necessary to reproduce the
  problem.
  This includes, at the very least:
   <ul>
    <li>
     Any command line options passed to ezstream.
    </li>
    <li>
     The ezstream configuration file, in full. (Please censor passwords and
     host names, but keep it intact otherwise.)
    </li>
   </ul>
 </li>
 <li>
  <em>What happened?</em><br/>
  Describe the problem that you're having, including all error and log
  messages that you might be seeing.
  In case of a crash or an &quot;Internal error&quot;, if you're up to it,
  include a gdb backtrace:
  <ul>
   <li>
    Rebuild ezstream with debugging symbols.
    This can be achieved by building with &quot;<code>DEBUG=-g</code>&quot;
    in the environment.
   </li>
   <li>
    Reproduce the issue and get a fresh core dump.
    On some systems, core dumps need to be enabled with
    &quot;<code>ulimit -c unlimited</code>&quot;.
   </li>
   <li>
    Run <code>gdb(1)</code> with the core dump.
    The name of the file containing the dump is either
    <code>ezstream.core</code> or just <code>core</code> (adjust the following
    command, if necessary):
    &quot;<code>gdb ezstream ezstream.core</code>&quot;
   </li>
   <li>
    Get the backtrace, by typing &quot;<code>bt full</code>&quot; at the
    <em>(gdb)</em> prompt.
   </li>
   <li>
    Include the complete backtrace in your problem report.
    Note that coredump files are only meaningful on the system where ezstream
    crashed, and are useless elsewhere - do not include the core dump itself
    in your report.
   </li>
  </ul>
 </li>
 <li>
  <em>Do not diagnose the problem.</em><br/>
  Or, well, do not do so without including the other information as well.
  A diagnosis won't lead to a fix unless it can be verified by analyzing all
  of the symptoms.
  The only acceptable shortcut is a working patch in the report! :-)
 </li>
</ul>

<p>
 Good problem reports help make ezstream better, and your efforts are very
 much appreciated!
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
