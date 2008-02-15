
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
put. */


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


