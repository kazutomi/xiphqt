Contribute to Xiph.Org
======================

You want to help Xiph.Org out on its mission to bring high-quality multimedia compression to
the world? Great! Here are a few ideas on how you might help us out…

*	Before You Start
	*	Use high-quality, modern libraries and toolkits to ensure that comments can be handled
		with a minimum of fuss.
	*	Plan your UI first.

*	Enhance a preexisting audio player to take advantage of the features of Vorbis and FLAC.
*	Make your software easy to use.
*	Make sure that our formats are well-supported on all popular platforms.
*	Use the features of our formats and tools to their fullest.
*	If you’re writing something using our formats, stay involved with what we’re doing.


## Make sure that it’s easy for relative novices to listen to media files in our formats.
Try to ensure that your 

## Improve Vorbis, FLAC, and Speex support in Windows Media Player.
## Improve Vorbis, FLAC, and Speex support in iTunes.
iTunes is _the_ audio player for OS X; in order to be usable on that platform by novices, we
need to be at least somewhat usable on that platform.
Translated: Make sure that people can play Vorbis, FLAC, and Speex on Windows Media player on
Windows and on iTunes on OS X.

_(links to directshow and qt filters)_



## Improve the quality of Vorbis, FLAC, and Speex support on free audio players.


### Add codec support for our codecs in your application

One of the ways you can help us out is to make sure our codecs work on as many players as
possible—preferably the most common players.

## Improve the comment handling in your (or someone else’s) application

A number of music player applications don’t handle all the corner cases that can happen with
[Vorbis-style comment headers][vcomment]. In particular…

* Does the app ensure that only UTF-8 enters the comment header?

* Does the app allow multiline comment values?

* Does the app handle streams that have more than one comment with the same name?
  
  For example, what would your app do if you had two `%artist%` comments—one set to
  “Duke Ellington” and the other to “Count Basie”? If your app has a way to select tracks by
  artist, would the track show up in both Count Basie’s and Duke Ellington’s lists?

* Concatenate two Ogg-encapsulated audio files together. Does the user lose any functionality
  compared to if they were not concatenated?

## Add VorbisGain support
[VorbisGain][] is a not-quite-widely-supported, nondestructive way to normalize your audio
collection so you don’t have to fiddle with the volume knob while listening to a shuffled playlist.
If you’re writing a music player that supports Vorbis, FLAC, or Speex, adding VorbisGain support
can make long listening sessions with your player a little _less_ interactive. In a good way.

## Make sure that your player can play Vorbis and FLAC files gaplessly
This is a very nice feature for classical music albums (where one symphony may span many tracks)
and for continuous mixes.

There are two main things that you need to do in order to make sure that your player can play
files gaplessly. First, you need support in the audio format so the audio player can know
how much noise to remove from the decoded output. Second, you need to avoid closing and reopening
the audio stream in between tracks.

_(please fill this in a bit more.)_

Costless software is nice; [free software][] is better. OS X has a number of abandonware audio
programs to handle Vorbis and FLAC files, but any one of them might break irreparably in the
next OS upgrade. By writing software and releasing the source to it, you help ensure that your
work outlives you.

## Give money to us

If all else fails, you could [give us money][donate]. Your funds help keep Monty hacking.

	[free]:				http://www.gnu.org/philosophy/free-sw.html "Free as in freedom, mind you"
	[free software]:	http://www.gnu.org/philosophy/free-sw.html "Free as in freedom, mind you"
	[VorbisGain]:		http://sjeng.org/vorbisgain.html
	[donate]:			/donate/

(the following was simply removed out of another file)

Enhancing For Xiph
------------------

Assuming your audio player already supports Vorbis and FLAC, what can you do to enhance it
further? Here are a number of corner cases that you could try…

## Ensure that your application supports UTF-8 comments properly.
Many older tagging standards did not specify an encoding; Vorbis-style comment headers do.
Your ability to handle UTF-8 may be dependent on the libraries at your disposal
