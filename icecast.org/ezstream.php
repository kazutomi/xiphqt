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
Version 0.2.0 is available at <a href="http://downloads.xiph.org/releases/ezstream/ezstream-0.2.0.tar.gz">ezstream-0.2.0.tar.gz</a>
</p>
<p>
A windows binary is available as well at <a href="http://downloads.xiph.org/releases/ezstream/ezstream_win32_0.2.0_setup.exe">ezstream_win32_0.2.0_setup.exe</a>
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
<h3>Re-encoding</h3>
<br></br>
<p>
ezstream now supports reencoding.  This means that your output stream need not
be the same bitrate/samplerate or even format as your input files.
</p>
<p>    
Reencoding is supported via the use of external programs.  When you enable reencoding
you need to make sure of a few things :
</p>
<p>
1. You define a "decoder" for each type of input file.<br>
2. You define a "encoder" for each possible type of output stream.<br>
</p>

<p>
So if you had a mixture of mp3 and vorbis files in your playlist, you will need to make
sure that a decoder is provided for each type.  Ezstream will take the output of the
decoder and send it directly to the specific encoder.  All output of the decoder should
be written to stdout and should be in raw form.  Most decoder support this mode.  Encoders
should all be configured also with raw input and should read it from stdin.
</p>

<p>
The following decoder/encoders can be used : 
</p>

<p>
For MP3 :<br>
decoder : madplay<br>
encoder : lame<br>
</p>

<p>
For Vorbis :<br>
decoder : oggdec<br>
encoder : oggenc<br>
</p>
<p>
For FLAC :<br>
decoder : FLAC<br>
encoder : not yet supported by libshout, and thus not by ezstream.<br>
</p>

<p>
Additional decoders and encoders may be used, as long as they follow the rules defined above.
</p>
<p>
The following config file section defines the reencoding parameters :
</p>

<p>
<pre>
&lt;reencode&gt;
        &lt;enable&gt;1&lt;/enable&gt;
        &lt;encdec&gt;
        &lt;!-- Support for FLAC decoding (input files) --&gt;
                &lt;format&gt;FLAC&lt;/format&gt; &lt;!-- format = output stream format --&gt;
                &lt;match&gt;.flac&lt;/match&gt;  &lt;!-- match = input file format --&gt;
                &lt;decode&gt;flac -s -d --force-raw-format --sign=signed 
                              --endian=little @T@ -o -&lt;/decode&gt;
                &lt;encode&gt;Not supported Yet&lt;/encode&gt;
        &lt;/encdec&gt;
        &lt;encdec&gt;
        &lt;!-- Support for MP3 decoding via madplay, and encoding via LAME --&gt;
                &lt;format&gt;MP3&lt;/format&gt;
                &lt;match&gt;.mp3&lt;/match&gt;
                &lt;decode&gt;madplay -o raw:- @T@ 2&gt;/dev/null&lt;/decode&gt;
                &lt;encode&gt;lame -r -x -b 56 -s 44.1 --resample 22.05 -a - - 
                              2&gt;/dev/null&lt;/encode&gt;
        &lt;/encdec&gt;
        &lt;encdec&gt;
        &lt;!-- Support for Vorbis decoding via oggdec, and encoding via oggenc --&gt;
                &lt;format&gt;VORBIS&lt;/format&gt;
                &lt;match&gt;.ogg&lt;/match&gt;
                &lt;decode&gt;oggdec --raw=1 @T@ -o - 2&gt;/dev/null&lt;/decode&gt;
                &lt;encode&gt;oggenc -Q -r -q 0 --resample=44100 --downmix 
                              -t "@M@" -c STREAMER=ezstream -&lt;/encode&gt;
        &lt;/encdec&gt;
&lt;/reencode&gt;
</pre>
</p>
<p>
Note that the following keywords can be used :
</p>

<p>
@T@ = The fully qualified name of the track being played in the playlist<br>
@M@ = The metadata for the current track<br>
</p>

<p>
All encoding options (bitrate/samplerate/channels/quality) are set as command line options of
each of the encoders.  Each encoder has slightly different options that control these values.
The examples here can be used as a guide, but please make sure you check the manual for each
encoder for the correct encoding options.  In all cases, the encoder should be configured to
read RAW audio data from stdin.
</p>


</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br><br>

<br><br>
<? include "common/footer.php"; ?>


