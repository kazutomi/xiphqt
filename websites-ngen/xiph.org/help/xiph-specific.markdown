Enhancing For Xiph
==================

Assuming your audio player already supports Vorbis and FLAC, what can you do to enhance it
further? Here are a number of corner cases that you could try…

## Ensure that your application supports UTF-8 comments properly.
Many older tagging standards did not specify an encoding; Vorbis-style comment headers do.
Your ability to handle UTF-8 may be dependent on the libraries at your disposal
