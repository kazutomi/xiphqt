
Steps to encode vorbis: 

vorbis structures needed: 


/* bitstream settings.*/
 vorbis_info      vi; 
	
/* comment on headers.*/
 vorbis_comment   vc; 

/* central working state for the packet->PCM decoder. */
 vorbis_dsp_state vd; 

/* local working space for packet->PCM decode. */
 vorbis_block     vb; 


/* initialize vorbis_info:*/
 vorbis_info_init(&vi);

/* choose an encoding mode (VBR, CBR, ABR) */
/* http://xiph.org/vorbis/doc/vorbisenc/overview.html 
   explains this. */

 vorbis_encode_init() /*or*/
 vorbis_encode_init_vbr()

/* setup comments: */
 vorbis_comment_init(&vc);
 vorbis_comment_add_tag(&vc,"ENCODER","encoder_example.c");


/* setup lasting structures: */
 vorbis_analysis_init(&vd, &vi);
 vorbis_block_init(&vd, &vb);

 ogg_stream_init()

/* get the 3 header packets: */
 vorbis_analysis_headerout()
 ogg_stream_packetin()
 ogg_stream_packetin()
 ogg_stream_packetin()
 ogg_stream_flush() (or _pageout(), but as the datasize is probably 
 lesser then a page, it wouldn't pageout the page).
 fwrite()
 fwrite()

/* start reading audio data into a buffer. */

 buffer = vorbis_analysis_buffer()

/* this buffer is a matrix where data read from the WAV file must be
put. see below in reading data from WAV how to fill this buffer. */


/* tell the library how much we actually submitted */
 vorbis_analysis_wrote(&vd,i);

/* if there is no more data, submit with i = 0 */

/*loop: submit blocks to encode. */
 while vorbis_analysis_blockout(&vd,&vb) == 1
 ogg_stream_packetin()

/* analysis. TODO: don't know what's the difference here with
 VBR or managed mode.*/
 vorbis_analysis(&vb,NULL);
 vorbis_bitrate_addblock(&vb);

 while(vorbis_bitrate_flushpacket(&vd, &op))
 ogg_stream_packetin()
 
 while !ogg_page_eos()
 ogg_stream_pageout(), fwrite(), fwrite()

/* end loops and clean up: */ 
 ogg_stream_clear(&os);
 vorbis_block_clear(&vb);
 vorbis_dsp_clear(&vd);
 vorbis_comment_clear(&vc);
 vorbis_info_clear(&vi);



reading an wave file: 
http://ccrma.stanford.edu/courses/422/projects/WaveFormat/

the fields we're interested in are (for configuring
vorbis_encode_init...()): 

- SampleRate (little endian) 24th byte (starting from zero). 4 bytes of
data.  (eg 44100 Hz)

- NumChannels 22nd byte (sf0). little endian. 2 bytes of data. (eg Stereo = 2) 

- BitsPerSample 34th byte (sf0). little.  2 bytes of data. (eg 16 bits)

- BlockAlign: can be calculated ( NumChannels * BitsPerSample/8 ) or
  read: 32nd byte (sf0). little endian. 2 bytes of data. 


understanding the 
 float **buffer = vorbis_analysis_buffer()

buffer[0..(numChannels -1)][0..(i-1)] where i is the 
second parameter (buffer size) of vorbis_analysis_buffer(), which is
an arbitrarily set value. 

see numChannels above, when we learn to read the wav files. 

the number 4 used several times in the code for the  vorbis encode_example.c 
is the wav parameter BlockAlign. 

everything now needed to get the correct  buffer value (for each
channel) now is to convert from little to big endian and, finally,
divide the result by 2^(BitsPerSample - 1). 



---- 

embedding vorbis in theora: 

Headers:

theora: _encode_init, _comment_init
vorbis: _encode_init, _comment_init, _analysis_init, _block_init
theora: _encode_header,  with: ogg_stream_packetin, ogg_stream_pageout, fwrite
	_encode_comment, with: ogg_stream_packetin (to be flushed)
	_encode_tables,  with: ogg_stream_packetin (to be flushed)
vorbis: _analysis_headerout (1,2,3), with: ogg_stream_packetin (1), ogg_stream_pageout, fwrite
	ogg_stream_packetin (2) (to be flushed)
	ogg_stream_packetin (3) (to be flushed)
theora: ogg_stream_flush, fwrite
vorbis: ogg_stream_flush, fwrite

Frames data: 

submit video data and audio data to encode, while not getting a video
or audio page. when a page is gotten, fwrite it. Even running
linearly the code, we should think these processes (of submitting and 
encoding video and audio) run in parallel. So, we must consider the 
possibility of two pages being generated at 'the same time'. In this 
case, we must fwrite first the one which has the lesser
_granule_time(). 



