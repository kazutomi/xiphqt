
AUTHOR(S):

Robert Voigt: robert.voigt@gmx.de



INTRODUCTION:

This is an application that measures masking. The Vorbis project needs it to develop a better psychoacoustic model. When it's finished hopefully a lot of people will download this program and take the masking test, then send the file created by the program back to the Vorbis developers.
The program is written with Qt for the GUI and libao for audio output.
It starts with measuring the threshold of hearing for the given combination of soundcard, headphones and ears. It's not the absolute threshold (ATH), it's just relative to a fixed volume (INIT0dB in testwindow.cpp). This threshold for the midrange frequencies is then used to calculate 0 dB SL, which is needed for the masking test.
Then the threshold of hearing for noise is tested.
Then comes the masking test. For each frequency and amplitude, the masker is produced DURATION (audio.cpp) seconds, with the masked signal added in the first and third quarter of DURATION. 



COMPILATION:

(Note that the final version is supposed to be distributed as a binary to the users.)

You need Qt 2.x. I tested with 2.2.1 (the version required by KDE 2.0) and 2.2.3 (the latest version). You can get Qt from www.trolltech.com .

You also need moc, the Qt meta object compiler.

You may also need tmake to generate a new Makefile. You can get tmake from http://www.trolltech.com/products/download/freebies/tmake.html . Set TMAKEPATH to something like "/usr/lib/tmake/lib/linux-g++" .
Use tmake the following way:
tmake mask.pro -o Makefile
where mask.pro is the project file for tmake.

You also need libao (from Vorbis CVS for instance). At first I couldn't link this C++ app to libao. I changed ao.h in the following way, then make and install libao again, and after a while I could link. Could someone please add this to CVS?

#ifndef __AO_H__
#define __AO_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

-- code of ao.h --

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __AO_H__ */



TODO-LIST:

- Change the copyright notice to GPL or something. The current copyright notice is just there to have something at all.

- Find a name for the program.

- Add a noise making algorithm.

- Add the functionality for saving and restoring sessions, to allow the user to quit at some point and continue at the same point later.

- Add version information to the file that is being produced.

- Add system output volume control.

- Improve software design as far as necessary (it's for one time use only).

- Determine what combinations of frequencies and amplitudes should be testet and what resolution should be used. At the moment all possible combinations are tested at a luxurious resolution. The whole thing would take about 160 hours this way.

- Add something that tells the user how much is left to do.

- Improve the algorithm for finding the threshold of audibility. Right now it's a plain binary tree algorithm. It minimizes the number of iterations to find a value within a given range around the actual value *only* if the input from the user (could hear it or not) is correct. We need something more robust here.

- Add a window function to at least Audio::addtone() to avoid clicks.

- Fix lots of bugs.

- Rewrite README-user.txt to adapt it to changes and put it into good English.