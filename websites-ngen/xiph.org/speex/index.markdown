Speex
=====

Speex is an [Open Source/Free Software][osfs], patent-free audio compression format
designed for speech. The Speex Project aims to lower the barrier of entry for voice
applications by providing a free alternative to expensive proprietary speech codecs
Moreover, Speex is well-adapted to Internet applications and provides useful features that
are not present in most other codecs.
Finally, Speex is part of the [<abbr title='GNU’s Not Unix'>GNU</abbr> Project][gnu]
and is available under the [Xiph.org variant of the BSD license][xiph-bsd].

	[osfs]:		./osfs/
	[gnu]:		http://www.gnu.org/
	[xiph-bsd]:	/licenses/bsd/
	
## The Technology

Speex is based on <abbr title='Code-Excited Linear Prediction'>CELP</abbr> and is designed
to compress voice at bitrates between 2 and 44 <abbr title='kilobits per second'>kbps</abbr>.

Some of Speex’s features include:

*	Narrowband (8 kHz), wideband (16 kHz), and ultra-wideband (32 kHz) compression
	in the same bitstream
*	Intensity stereo encoding
*	Packet loss concealment
*	Variable bitrate operation (VBR)
*	Voice activity detection (VAD)
*	Discontinuous transmission (DTX)
*	An in-progress fixed-point port

Speex has a number of features that other codecs don’t such as intensity stereo encoding
and a <abbr title='variable bitrate'>VBR</abbr> mode; see our [comparison page][] for more.

	[comparison page]: ./compare/

## Getting Involved

If you want to get involved with Speex, one of the easiest things you can do is use it
in your application. Speex is well-suited for <abbr title='Voice over IP'>VoIP</abbr>,
internet audio streaming, voice mail or interview archival, and audio books. For a list
of applications using Speex, have a look at our [users][] page.

	[users]: ./users/
