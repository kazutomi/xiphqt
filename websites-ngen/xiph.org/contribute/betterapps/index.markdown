Making Your App Better
======================

All of the advice herein assumes that you’re capable of listening to users and are willing to
do what you can to provide a terriffic user experience. If you’re not convinced that an intuitive
UI is a necessity, read Eric Raymond’s [The Luxury of Ignorance][ignorance] as well as
John Gruber’s [Spray-On Usability][spray] to get an idea of what you’ll need to do.

	[ignorance]:	http://www.catb.org/~esr/writings/cups-horror.html
	[spray]:		http://daringfireball.net/2004/04/spray_on_usability

Now, after you’ve planned out what you want your app to do, start planning what sort of libraries
you’ll use. One tool that you might find useful is Scott Wheeler’s [TagLib][], a library that
makes it easy to access and modify Vorbis-style comments in Ogg Vorbis and FLAC files. 


## Unicode support

Vorbis, FLAC, and Speex all use Vorbis-style comments, which specify that their contents are in
UTF-8, a Unicode encoding. The snag is that UTF-8 support can’t be taken for granted quite yet;
you’ll have little hope of actually being able to handle UTF-8 competently unless you’re using a
library that supports it properly. Windows NT, 2000, and XP are capable of handling Unicode
(Windows uses UCS-2 internally); if you’re writing something for a free operating system,
[GTK 2][] is one of the more polished solutions out there as it relies on the [Pango][] library.
If you’re still not sure what all this Unicode stuff is, Joel Spolsky
[explains the rudiments of Unicode][joel] quite well.

Also, don’t think that this is just an issue for non-English speakers—you may have an album by
μ-Ziq or Tool’s Ænima that you’d like to see displayed properly.

	[gtk 2]:	http://www.gtk.org/
	[pango]:	http://www.pango.org/
	[joel]:		http://www.joelonsoftware.com/articles/Unicode.html


## VorbisGain

If you’re writing an audio player, you might want to consider adding [VorbisGain][] support to it.
VorbisGain writes a few Vorbis-style comments into the headers of your music files; these comments
say how much to quiet or amplify each song so they all have roughly the same loudness. This will
let you do more productive things than fiddle with the volume knob every three to five minutes
when you’re listening to a shuffled playlist.

	[taglib]:		http://developer.kde.org/~wheeler/taglib.html
	[vorbisgain]:	http://sjeng.org/vorbisgain.html
