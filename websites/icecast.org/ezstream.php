<? include "common/header.php"; ?>
<h2>ezstream</h2>
<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Download</h3>
<br></br>
<p>
Version 0.1.2 is available at <a href="http://downloads.us.xiph.org/releases/ezstream/ezstream-0.1.2.tar.gz">ezstream-0.1.2.tar.gz</a> - Version 0.1.2 supports handling of a SIGHUP message which will cause ezstream to reread the currently processing playlist.  This only works on unix platforms and so I'm not going to release a 0.1.2 binary for win32.
</p>
<p>
A windows binary is available as well at <a href="http://downloads.us.xiph.org/releases/ezstream/ezstream_win32_0.1.1_setup.exe">ezstream_win32_0.1.1_setup.exe</a>
</p>
<br></br>
<h3>What is it ?</h3>
<br></br>
<p>
ezstream is a command line utility which is a improved version of the old
"shout" utility.  It enables you to stream mp3 or vorbis files to an icecast
server without reencoding and thus requires very little CPU.  ezstream is
controlled via a XML config file (a few examples are provided in the conf
directory).
</p>
<p>
ezstream can stream mp3 and ogg vorbis files as well as reading from stdin.
ID3v1 tags are supported in mp3 files and all ogg vorbis tags are propagated
as metadata as well.
</p>
<br><br>
<h3>How to use it ?</h3>
<br></br>
<p>
ezstream is XML config file based.  Configuration is done by editing the config
file to suit your needs..The following is an example config file :
</p>
<p>
&lt;ezstream&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;url&gt;http://localhost:8000/testmount.ogg&lt;/url&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;sourcepassword&gt;hackme&lt;/sourcepassword&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;format&gt;OGGVORBIS&lt;/format&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;filename&gt;sunking.ogg&lt;/filename&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfoname&gt;My Stream&lt;/svrinfoname&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfourl&gt;http://www.oddsock.org&lt;/svrinfourl&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfogenre&gt;RockNRoll&lt;/svrinfogenre&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfodescription&gt;This is a stream description&lt;/svrinfodescription&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfobitrate&gt;192&lt;/svrinfobitrate&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfoquality&gt;4.0&lt;/svrinfoquality&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfochannels&gt;2&lt;/svrinfochannels&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfosamplerate&gt;44100&lt;/svrinfosamplerate&gt;<br>
	&nbsp;&nbsp;&nbsp;&lt;svrinfopublic&gt;1&lt;/svrinfopublic&gt;<br>
&lt;/ezstream&gt;<br>
</p>
<p>
<b>URL</b> - this URL specified the location and mountpoint of the icecast server to which the stream will be sent.
</p>
<p>
<b>SOURCEPASSWORD </b>- the source password for the icecast server
</p>
<p>
<b>FORMAT </b>- either MP3 or OGGVORBIS, you must specify which format you input files are in.
</p>
<p>
<b>FILENAME </b>- This can be a single file (in which ezstream will stream that file over and over continuously) or can be a .m3u file which is a playlist of files.  currently, ezstream will go through the files sequentially.  If you specify "stdin" as the filename then ezstream will read the input from stdin.
</p>
<p>
<b>SVRINFONAME </b>- Broadcast name (informational only)
</p>
<p>
<b>SVRINFOURL </b>- Website associated with the broadcast (informational only)
</p>
<p>
<b>SVRINFOGENRE </b>- Genre of broadcast (informational only) (used for YP)
</p>
<p>
<b>SVRINFODESCRIPTION </b>- Description of broadcast (informational only) (used for YP)
</p>
<p>
<b>SVRINFOBITRATE </b>- Bitrate of broadcast (informational only) (used for YP) It is YOUR responsibility to ensure that the bitrate specified here matches up with your input.  Note that this info is not for anything OTHER than YP listing info.
</p>
<p>
<b>SVRINFOQUALITY </b>- Used only for OggVorbis streams, similar to the bitrate in that it is used only for informational and YP purposes.
</p>
<p>
<b>SVRINFOCHANNELS </b>- 1 = MONO, 2 = STEREO (informational only) (used for YP)
</p>
<p>
<b>SVRINFOSAMPLERATE </b>- (informational only) (used for YP)
</p>
<p>
<b>SVRINFOPUBLIC </b>- Indicates wether to list this stream in a public YP.
</p>

</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br><br>

<br><br>
<? include "common/footer.php"; ?>


