Glossary
========

<dl>
<dt id='lossless'>lossless</dt>
<dd>
	<p>
		<dfn>Lossless</dfn> data compression methods do not remove input data in order to make it
		compress better. Examples of lossless compression methods include Zip,
		<abbr title='Portable Network Graphics'>PNG</abbr>, and
		<abbr title='Free Lossless Audio Codec'>FLAC</abbr>.
	<p>
	<p><abbr title='compare'>cf.</abbr> <a href='#lossy'>lossy</a>.</p>
</dd>


<dt id='lossy'>lossy</dt>
<dd>
	<p>
		<dfn>Lossy</dfn> data compression methods remove some of their input data in order to
		improve compression ratios. Some lossy compressors include Vorbis, Speex,
		<abbr title='Joint Photographic Experts Group'>JPEG</abbr>, and Theora.
	</p>
	<p><abbr title='compare'>cf.</abbr> <a href='#lossless'>lossless</a>.</p>

</dd>

<dt id='transcode'>transcode</dt>
<dd>
	<p>
		<dfn>Transcoding</dfn> is the process of encoding a compressed file from another compressed
		file instead of from a pristine source.
	</p>
	<p>
		When transcoding from a <a href='#lossy'>lossy</a> source format like Vorbis or MP3 to
		another lossy format, extra data is lost because the imperfections and audio artifacts of
		<strong>both</strong> codecs turn up in the final data.
	</p>
	<p>
		Transcoding from a <a href='#lossless'>lossless</a> source format like
		<abbr title='Free Lossless Audio Codec'>FLAC</abbr> to a lossy format does
		not incur the double-artifact penalty.
	</p>
</dd>
</dl>
