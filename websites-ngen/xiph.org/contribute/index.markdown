Contribute to Xiph.Org
======================

<!--
* Improve our codecs.
* Document our codecs.

* Improve our tools.
* Document our tools.

* Use our codecs in your app.
* Make sure that all our features are supported in your app.
  * Have decent Unicode support.
  * Support multiline Vorbis-style comments.

-->

If you want to help Xiph.Org ensure that the foundations of Internet multimedia remain free, there are three main ways to help:

* Improve our codecs, libraries, or tools.
* Improve others’ tools and libraries that use our codecs and tools.
* Write a new tool that uses our codecs, libraries, and tools.

If you want to improve our stuff, here are some suggestions:

* Improve our codecs so they sound better and/or use less bandwidth.
* Write a BSD-licensed wrapper for our libraries in Managed C++ to make Ogg, Vorbis, and FLAC
  available to users of the .NET Framework.
* Help improve our documentation.

However, it’s not enough to have multimedia codecs; their success is dependent on the quality and availability of the tools that use them. As such, if you want to help further our mission, you may want to consider improving a music or video player rather than working on our libraries and tools directly. For example, you may want to work on the UI or plugins of a CD ripper so people who want to reencode their music collection in Ogg Vorbis or FLAC can do so with a minimum of fuss. If you are writing or improving an application, have a look at [our suggestions on how you can improve your application][betterapps].

	[betterapps]: ./betterapps/

<i>(I stopped writing here)</i>

*	Enhance a preexisting audio player to take advantage of the features of Vorbis and FLAC.
*	Use the features of our formats and tools to their fullest.
*	Make sure that our formats are well-supported on all popular platforms.
*	If you’re writing something using our formats, stay involved with what we’re doing.


##  Help Windows Media Player support more of our formats

Help work on [illiminable’s Ogg DirectShow Filters][illi] so Windows Media Player will be able to play Ogg Vorbis, Ogg Speex, Ogg FLAC, and Ogg Theora files.

	[illi]: http://www.illiminable.com/ogg/


## Improve Vorbis, FLAC, and Speex support in iTunes

iTunes is _the_ audio player for OS X; in order to be usable on that platform by novices, we need to be at least somewhat usable on that platform. Translated: Make sure that people can play Vorbis, FLAC, and Speex on Windows Media player on Windows and on iTunes on OS X.


## Implement our codecs in your application

One of the ways you can help us out is to make sure our codecs work on as many players as possible—preferably the most common players.


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


## Make sure that your player can play Vorbis and FLAC files gaplessly

This is a very nice feature for classical music albums (where one symphony may span many tracks)
and for continuous mixes.

There are two main things that you need to do in order to make sure that your player can play
files gaplessly. First, you need support in the audio format so the audio player can know
how much noise to remove from the decoded output. Second, you need to avoid closing and reopening
the audio stream in between tracks.

<i>(please fill this in a bit more.)</i>

Costless software is nice; [free software][] is better. OS X has a number of abandonware audio
programs to handle Vorbis and FLAC files, but any one of them might break irreparably in the
next OS upgrade. By writing software and releasing the source to it, you help ensure that your
work outlives you.


## Give money to us

If all else fails, you could [give us money][donate]. Your funds help keep Monty hacking.

	[free]:				http://www.gnu.org/philosophy/free-sw.html "Free as in freedom, mind you"
	[free software]:	http://www.gnu.org/philosophy/free-sw.html "Free as in freedom, mind you"
	[vorbisgain]:		http://sjeng.org/vorbisgain.html
	[donate]:			/donate/
