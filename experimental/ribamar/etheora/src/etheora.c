/*

   Etheora - a library for making theora programming easier.
   ribamar@gmail.com

   reasons to use etheora:
   - lot of code to get the 1st theora code running;
   - hard to debug code for video: you need to look at images
   to undestand which errors you may have thrown; 
   - transport abstraction (ogg packages); just present your file
   or network stream (FILE *) to the functions; 
   - theora uses the yuv colorspace; etheora provides rgb <-> yuv 
   convertion functions; 
   - theora (for now) only uses the 420 subsampling; functions for 
   444 and 422 upsampling and downsampling are provided. 

   implemented for now:
   - encoding process. 
   - decoding process. 
   - rgb <-> convertion functions. 

   needed tests:

   missing implementations:
   - audio encoding. 
   - audio decoding. 
   - speech encoding. 
   - speech decoding. 
*/



#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <etheora.h>
#include <etheora-int.h>

/*
 TODO: sanity tests.
 */


/*
etheora_fillyuv(): fills a yuv_buffer structure according to a
theora_info structure, so after this function, users may only 
yuv->y, yuv->u and yuv->v are ready to be filled and then
to be encoded with theora_encode_YUVin(), which is called by
etheora_writeframe().

Don't call this function again with the same yuv_buffer yuv 
before etheora_freeyuv(). You probably want to use the same
yuv_buffer to encode all frames in the same video so most users
may disregard this paragraph. 

finfo is a file descriptor where debug info is printed. 

Returns 
0 on succes.
< 0 on unsuccess. 

ETHEORA_ERR_MALLOC  -- could not allocate memory for one or 
                       more of y, u, v channels. 

*/

int etheora_fillyuv(theora_info *ti, yuv_buffer *yuv,
		FILE *finfo){
		
	yuv->y_width   = ti->width;
	yuv->y_height  = ti->height;
	yuv->y_stride  = +yuv->y_width;
	switch(ti->pixelformat){
		/*
		yuv->uv_width  means horizontal ss ratio.
		yuv->uv_height means  vertical  ss ratio.
		444 : neither horizontal nor vertical ss.
		422 : horizontal ss but no vertical ss.
		420 : horizontal and vertical ss.
		*/
		case OC_PF_444:
			yuv->uv_width  = ti->width;
			yuv->uv_height = ti->height;
			break;

		case OC_PF_422:
			yuv->uv_width  = ti->width / 2;
			yuv->uv_height = ti->height;
			break;

		case OC_PF_420:

		default:
			/*420 assumed by default*/
			yuv->uv_width  = ti->width / 2;
			yuv->uv_height = ti->height / 2;
	}
	yuv->uv_stride = +yuv->uv_width;

	size_t ma; 

	/*memory allocation for y channel.*/
	/*TODO: it's not working for negative strides!*/
	etheora_abs(ma, yuv->y_stride * yuv->y_height); 
	yuv->y = (unsigned char *) malloc(ma); 
	/*negative strides: ->y points to the end of y channel.*/	
	if(yuv->y_stride < 0) 
		yuv->y = &(yuv->y[ma]); 
	if(!(yuv->y)){
		fprintf(finfo, "etheora_fillyuv():"
				"memory allocation error (y).\n");
		return ETHEORA_ERR_MALLOC; 
	}

	/*memory allocation for y, u, v channels.*/
	etheora_abs(ma, yuv->uv_stride * yuv->y_height); 
	yuv->u = (unsigned char *) malloc(ma); 
	yuv->v = (unsigned char *) malloc(ma); 
	/*negative strides: ->x points to the end of x channel.*/	
	if(yuv->uv_stride < 0){ 
		yuv->u = &(yuv->u[ma]); 
		yuv->v = &(yuv->v[ma]); 
	}
	if(!(yuv->y && yuv->u && yuv->v)){
		fprintf(finfo, "etheora_fillyuv():"
				"memory allocation error (uv).\n");
		return ETHEORA_ERR_MALLOC; 
	}

	return 0; 

}

/*
etheora_freeyuv():
frees the memory allocated by etheora_fillyuv().
Note that the yuv_buffer pointer itself isn't allocated by
etheora_fillyuv(). 

returns: 
no returning value. 

*/

void etheora_freeyuv(yuv_buffer *yuv){
	if(yuv){
		free(yuv->y);
		free(yuv->u);
		free(yuv->v);
	}
	return; 
}


/*
etheora_free_streams_states(): 
frees the memory allocated by etheora_streams_states_init().
Note that the etheora_streams_states pointer itself isn't 
allocated by etheora_streams_states_init(). 
*/

void etheora_free_streams_states(etheora_streams_states *ess){
	if(ess){
		free(ess->streams); 
		free(ess->streams_states); 
	}
}
/*
etheora_filltinfo_nooffset() fills the theora_state structure for 
videos with no offsets (ie, the frame_width:height equals to
video_widht:height. However, you may always change any value
filled by this function after calling it, and specify offsets for
your video. The main target of this function is to provide a
method for filling a theora_info structure with pretty default
values (ie, for lazy people). 

aspect must be one of these: 
ETHEORA_ASPECT_NORMAL  -> 4:3 aspect ration
ETHEORA_ASPECT_WIDE_SCREEN  -> 16:9 aspect ratio
ETHEORA_ASPECT_PRESERVE   -> preserves frame width:height

fps_numerator/fps_denuminator must give the frames per 
second rate.

frame_width and height must be divisible by 16, 
according with theora specification. if you need values other 
than these, you must use offsets. 

Values provided by default (aimed for best quality): 
ti->colorspace = OC_CS_UNSPECIFIED;
ti->target_bitrate = 256000;
ti->quality = 63;
ti->pixelformat = OC_PF_420;
and those in the theora_filltinfo_hardvalues() function. 

returns 0 if success, and not zero if not success (but, even in
error, all parameters will be filled). 
*/

int etheora_filltinfo_nooffset(theora_info *ti, 
		ogg_uint32_t frame_width, 
		ogg_uint32_t frame_height, 
		etheora_aspect aspect,
		ogg_uint32_t  fps_numerator, 
		ogg_uint32_t  fps_denominator){

	int bitrate =  ETHEORA_TINFO_BITRATE;
	ti->width = frame_width;
	ti->height = frame_height;
	ti->frame_width = frame_width;
	ti->frame_height = frame_height;
	ti->offset_x = ti->offset_y = 0;
	ti->aspect_numerator = frame_width;
	ti->aspect_denominator = frame_height;
	if(aspect ==  ETHEORA_ASPECT_NORMAL){
		ti->aspect_numerator = 4;
		ti->aspect_denominator = 3;
	}
	if(aspect ==  ETHEORA_ASPECT_WIDE_SCREEN){
		ti->aspect_numerator = 16;
		ti->aspect_denominator = 9;
	}
	ti->fps_numerator = fps_numerator;
	ti->fps_denominator = fps_denominator;
	ti->colorspace = ETHEORA_TINFO_COLORSPACE;  //OC_CS_UNSPECIFIED;
	ti->target_bitrate = bitrate;
	ti->quality = ETHEORA_TINFO_QUALITY;
	ti->pixelformat = ETHEORA_TINFO_PIXELFORMAT;  // OC_PF_420;
	etheora_filltinfo_hardvalues(ti);
	/*frame_width and height must be divisible by 16, 
	 * according with theora specification.*/
	return (frame_width % 16) + (frame_height % 16);
}

/*
etheora_filltinfo_hardvalues() fills a theora_info structure
with the values which are commonly hardcoded in theora encoders.
You'll, however, may be able to change these values after calling
this function. 

returns 0 if success, and not zero if not success. 
 */
int etheora_filltinfo_hardvalues(theora_info *ti){
	ti->dropframes_p=0;
	ti->quick_p=1;
	ti->keyframe_auto_p=1;
	ti->keyframe_frequency=64;
	ti->keyframe_frequency_force=64;
	ti->keyframe_data_target_bitrate= (ogg_uint32_t) 
		                   (ti->target_bitrate*1.5);
	ti->keyframe_auto_threshold=80;
	ti->keyframe_mindistance=8;
	ti->noise_sensitivity=1;
	return 0; 
}


/*
etheora_encode_init() is just a wrapper to 
theora_encode_init() to avoid users making direct calls to 
libtheora. 

TODO: document this (in context with the other functions):
we should not assume they know something about libtheora.  
*/

int etheora_encode_init(theora_state *ts, theora_info *ti){
	return theora_encode_init(ts, ti);
}

/*
etheora_writeheaders() writes the headers of the theora video 
represented by the theora_state ts, using the ogg_stream_state
stream (where the subsequent frames must be pageouted after 
the write of the headers). The comments in the theora_comment tc
will be part of the headers. 

the headers data will be written to the file descriptor fout, 
which may be a socket, stdout or any file. finfo will be used 
for printing the debug and informational data. 

a value rv will be used as serial number for the stream in the
function ogg_stream_init(). This value rv will be returned in 
case of success, and -rv in case of a generic error or these 
specific errors: 
> 5 on succes.
< 0 on unsuccess. 

ETHEORA_ERR_HEADERS       - problem encoding headers. 
ETHEORA_ERR_OGG_INTERNAL  - libogg error in ogg_stream_pageout()
ETHEORA_ERR_COMMENT       - problem encoding comment header. 
ETHEORA_ERR_TABLES        - problem encoding tables header. 
(rv and -rv will never be one of these errors). 

(Don't care if you can't get --- ) Note you *don't* need to call 
ogg_stream_init() for the ogg_stream_state stream outside of
this function; but you may, however, need the return value to
avoid serial number collision among other eventual 
ogg_stream_state, such as a stream for audio. 

*/

int etheora_writeheaders(theora_state *ts, 
		ogg_stream_state *stream, 
		theora_comment *tc, 
		FILE *fout, FILE *finfo){

	int ret = 0, rv = 0;
	ogg_page page;

	/*initiating stream. don't let the bitstream serial number be
	 one of the possible specific return codes.*/
	srand(time(NULL));
	do{
		rv = rand();
	}while(rv == ETHEORA_ERR_HEADERS && 
	       rv == ETHEORA_ERR_OGG_INTERNAL &&
	       rv == ETHEORA_ERR_COMMENT &&
	       rv == ETHEORA_ERR_TABLES); 
	etheora_abs(rv, rv); 
	ogg_stream_init(stream, rv);

	ogg_packet header_p, comment_p, tables_p;
	/*encoding header packet.*/
	if (theora_encode_header (ts, &header_p)){
		fprintf(finfo, "Can't encode headers.\n");
		return ETHEORA_ERR_HEADERS; 
	}

	ogg_stream_packetin(stream, &header_p);
	/*writing out header (already in stream) to the page... */
	if(ogg_stream_pageout(stream, &page)!=1){
		fprintf(finfo,"Internal Ogg library error.\n");
	        return ETHEORA_ERR_OGG_INTERNAL; 
	}
	/*...and the page out to fout file descriptor. */
	fwrite(page.header, 1, page.header_len, fout);
	fwrite(page.body,   1, page.body_len,   fout);

	/*encode comment packet.*/
	/*TODO: include comment info?*/
	theora_comment_init(tc);
	if (theora_encode_comment(tc, &comment_p)){
		fprintf(finfo, "Can't encode comment.\n");
		return ETHEORA_ERR_COMMENT; 
	}
	theora_comment_clear(tc);
	/*encoding tables header.*/
	if(theora_encode_tables (ts, &tables_p)){
		fprintf(finfo, "Can't encode tables.\n");
	        return ETHEORA_ERR_TABLES; 
	}

	/*writing out comment and tables to the stream... */
	ogg_stream_packetin(stream, &comment_p);
	ogg_stream_packetin(stream, &tables_p);

	/*...then to the page...*/
	/*(unnecessary, as encoder_example.c doesn't do this
	  here, probably when flushing this is automatically
	  done.)*/
	while((ret = ogg_stream_pageout(stream, &page)) > 0){

		/*...then the page to the fout file descriptor.*/
		fwrite(page.header, 1, page.header_len, fout);
		fwrite(page.body, 1, page.body_len, fout);
	}
	if (ret < 0) {
		fprintf(finfo, "problem pageout-ing. going on.\n");
	}

	/* One must flush after the last header packet: */
	while(1){
		int result = ogg_stream_flush(stream,&page);
		if(result < 0){
			fprintf(finfo,"Internal Ogg library error.\n");
			return ETHEORA_ERR_OGG_INTERNAL; 
		}
		if(result ==0 )break;
		fprintf(finfo, "flush-ing.\n");
		fwrite(page.header,1,page.header_len,fout);
		fwrite(page.body,1,page.body_len,fout);
	}

	return rv; 

}

/*
etheora_writeframe() encodes a frame present in the structure
yuv_buffer yuv according to the theora_state ts, using the
ogg_stream_state stream. iflastframe must be:
ETHEORA_LAST_FRAME -> if this is the last frame to be encoded; 
ETHEORA_NOTLAST_FRAME -> if there are more frames to be encoded. 

the ogg_stream_state stream must be the same passed to the 
stream_writeheaders(). You probably want the ogg_page page to be
the same which as passed for all encoded frames before. 

the data will be written to the file descriptor fout, 
which may be a socket, stdout or any file. finfo will be used 
for printing the debug and informational data. 


Returns:
0 on success; 
negative on error. 

TODO: code errors? 

*/

int etheora_writeframe(theora_state *ts, 
			  yuv_buffer *yuv, 
			  int iflastframe, 
			  ogg_stream_state *stream, 
			  ogg_page *page,
			  FILE *fout, FILE *finfo){

	int ret = 0; 

	/*the real thing: packing frame...*/
	if((ret = theora_encode_YUVin( ts , yuv))){
		if(ret == OC_EINVAL)
			fprintf(finfo,"Encoder is not ready, or is finished.\n");
		if(ret == -1)
			fprintf(finfo,"The size of the given frame differs "
			"from of those previously input.\n");
		return -1;
	}
	/*... into a packet.*/
	ogg_packet frame_p;
	if(! (ret = theora_encode_packetout( ts, iflastframe, &frame_p) )){
		if(ret == 0)
			fprintf(finfo,"No internal storage exists OR "
			"no packet is ready.\n");
		if(ret == -1)
			fprintf(finfo,"The encoding process has completed.\n");
		return -1;
	}
	/*printing info..*/
	fprintf(finfo, "number of packed bytes in the frame: %ld\n",
			frame_p.bytes);
	/*writing also the packet
	 * framePacket to the output file... */
	ogg_stream_packetin(stream, &frame_p);
	while((ret = ogg_stream_pageout(stream, page)) > 0){
		/*
		fprintf(finfo, "pageout-ing frame.\n");
		*/
		fwrite(page->header,1,page->header_len,fout);
		fwrite(page->body,1,  page->body_len,fout);
	}
	if (ret < 0) {
		fprintf(finfo, "problem pageout-ing frame.\n");
		return -1; 
	}
	return 0; 
}

/*
 etheora_flushpage(): flushs all lasting data in the ogg_page page, 
using the ogg_stream_state, into the file represented by the 
descriptor fout. finfo is used to print debug and additional 
information.

Returns:
0 on success;
negative on error. 
*/
/*TODO: rewrite ths function for agreeing to the code style or maybe
 eliminate it. */
int etheora_flushpage(ogg_stream_state *stream, ogg_page *page, 
		FILE *fout, FILE *finfo){
	while(1){
		int result = ogg_stream_flush(stream,page);
		if(result < 0){
			fprintf(finfo,"Internal Ogg library error.\n");
			return -1; 
		}
		if(result == 0)break;
		fprintf(finfo, "flush-ing frame.\n");
		fwrite(page->header,1,page->header_len,fout);
		fwrite(page->body,1,  page->body_len,fout);
	}
	return 0;
}



/*TODO: rewrite ths function for agreeing to the code style or maybe
 eliminate it. */
int etheora_buffer_data(FILE *fin, ogg_sync_state *os){
        int  buf_size = MIN_BUFFER_READ;
        char *buffer  = ogg_sync_buffer(os, buf_size);
        int  bytes    = fread(buffer, 1, buf_size, fin);
        ogg_sync_wrote(os,bytes);
        return(bytes);
}

/*
etheora_streams_states_init(): 
initializes a etheora_streams_states structures, with n_streams logical 
streams. maybe etheora_structs_init() is better fitted for you. 

arguments:
etheora_streams_states *ess: pointer to a yet not started structure.
n_streams: amount of streams which will be held by the structure. 

return values:
  0, success
 <0, unsuccess

ETHEORA_ERR_NULL_ARGUMENT, NULL pointer argument passed
ETHEORA_ERR_MALLOC,        couldn't allocate memory

*/

int etheora_streams_states_init(etheora_streams_states *ess, int n_streams){
	int i;
	/*sanity test*/
	if(ess == NULL) return ETHEORA_ERR_NULL_ARGUMENT; 

	/*filling data structure*/
	ess->count = n_streams; 
	ess->streams = (ogg_stream_state*) 
		malloc(sizeof(ogg_stream_state)*n_streams); 
	ess->streams_states = (char*) malloc(sizeof(char)*n_streams); 

	if (ess->streams == NULL || ess->streams_states == NULL) 
		return ETHEORA_ERR_MALLOC; 

	for (i = 0; i < n_streams; i++) ess->streams_states[i] = 0; 

	return 0; 
}

/*
TODO: shouldn't this be a _decode_ function? 
etheora_structs_init(): 
initializes:
- a theora_info structure.
- a theora_comment structure. 
- a off_sync_state structure. 
- a etheora_streams_states structure, with n_streams logical streams.

arguments:
theora_info *ti : pointer to a yet not started structure, or NULL.
theora_comment *tc: pointer to a yet not started structure or NULL.
ogg_sync_state *os: pointer to a yet not started structure or NULL.
etheora_streams_states *ess: pointer to a yet not started structure or NULL.
n_streams: amount of streams which will be held by the structure. 

return values:
  0, success
 <0, unsuccess

ETHEORA_ERR_NULL_ARGUMENT, NULL pointer argument passed
ETHEORA_ERR_MALLOC,        couldn't allocate memory

*/

int etheora_structs_init(theora_info *ti, theora_comment *tc, ogg_sync_state 
	*os, etheora_streams_states *ess, int n_streams){
        if (os != NULL ) ogg_sync_init(os);
        if (tc != NULL ) theora_comment_init(tc);
        if (ti != NULL ) theora_info_init(ti);
        if (ess!= NULL && etheora_streams_states_init(ess, n_streams)){
		return ETHEORA_ERR_MALLOC;  
        }
	return 0; 
}


/*

etheora_decode_packet(): 
this function reads the buffer FILE *fin until outputting a ogg_packet packet
valid to some stream in etheora_streams_states ess. 

arguments:
fin, file descriptor where to read the video data. 
TODO: all arguments.

Return values rv: 

rv >= 0, success
rv < 0, error

rv = 0..127,  last output packet was sent to the stream rv, 
              and we didn't have to init a ess->streams[i] (ogg_stream_init)
rv > 127,     last output packet was sent to the stream (rv - 127), 
              and we had to init a ess->streams[i] (ogg_stream_init)
ETHEORA_ERR_DEC_NOMOREDATA, input buffer has (maybe yet) no data to be read.
ETHEORA_ERR_BAD_STREAM, ogg_stream_init() failed.
*/

int etheora_decode_packet(FILE *fin, etheora_streams_states *ess, 
	ogg_sync_state *sync_s,  ogg_packet *packet, FILE *finfo){

/*TODO:  improve comments */

	ogg_page op;
	int a,  b, i, rv; 
	ogg_stream_state *streams = ess->streams; 
	char *initiated_stream = ess->streams_states; 
	int num_streams = ess->count; 

	a =  b = i = rv = 0; 
	for (i = 0; initiated_stream[i] && i < num_streams; i++){
		a = ogg_stream_packetout(&streams[i], packet);
		switch(a){
			case -1: fprintf(finfo, "ogg_stream_packetout(packet):"
						 " We're out of sync and there"
						 " is a gap in the data. Usually"
						 " this will not be a fatal"
						 " error.\n");
				 break;
			case  1: fprintf(finfo, "ogg_stream_packetout(packet):"
						 " packet was assembled normally.\n");
				 b = i+1; 
				 break;
			case  0: fprintf(finfo, "ogg_stream_packetout(packet):"
						 " there is insufficient data"
						 " available to complete a packet.\n");
				 break;
		}
	}
	if(b) return (rv = b - 1); 

        do{
                while(ogg_sync_pageout(sync_s, &op) != 1 ){
                        a = etheora_buffer_data(fin, sync_s);
                        if(!a){
				fprintf(finfo, "No data read from buffer.\n");
				return ETHEORA_NOMOREDATA; 
			}
                        else fprintf(finfo, "%i bytes read from buffer.\n", a);
                };

                /* is this page in the beggining of a logical bitstream? (ie, 
		   is this the vorbis/theora/... initial header?)
                   if it's, init the first not initiated stream. 
		   rv != 1 doesn't let initiate more than 1 stream by function
		   call: only one ogg_page is obtained by function run. 
		*/
                if(ogg_page_bos(&op) && rv != 1){
			i = 0; 
			while(initiated_stream[i] && i < num_streams){ i++; }
			if(!initiated_stream[i]){
				a = ogg_stream_init(&streams[i], ogg_page_serialno(&op));
				/*TODO: change to = 1 when everything's tested */
				initiated_stream[i] = 1; 
				/*initiated_stream[i]++; */
				if(a) {
					fprintf(finfo, "ogg_stream_init(): ogg_stream_state"
							" wasn't properly declared.\n");
					return ETHEORA_ERR_BAD_STREAM; 
				}
				else {
					fprintf(finfo, "stream[%i] initiated.\n", i); 
					rv = 1; 
				}
			}
                }

		/* what's the owner stream of this page? try all. */
		for (i = 0; initiated_stream[i] && i < num_streams; i++){
			fprintf(finfo, "page serial number: %i\n", 
				ogg_page_serialno(&op)); 
			fprintf(finfo, "stream (%i) serial number: %ld \n", i, 
				(&streams[i])->serialno ); 
			a = ogg_stream_pagein(&streams[i], &op); 
			if(!a) fprintf(finfo, "ogg_stream_pagein(): The page was"
					" successfully submitted to the bitstream (%i).\n", i);
			else {
				fprintf(finfo, "ogg_stream_pagein() fails: (%i). This"
						" means that the page serial number"
						" page did not match the bitstream serial number"
						" or that the page version was incorrect (or"
						" the stream wasn't initiated).\n", a); 
			}

			a = ogg_stream_packetout(&streams[i], packet);
			switch(a){
				case -1: fprintf(finfo, "ogg_stream_packetout(packet):"
							 " We're out of sync and there"
							 " is a gap in the data. Usually"
							 " this will not be a fatal"
							 " error.\n");
					 break;
				case  1: rv = i; 
					 b = i+1; 
					 fprintf(finfo, "ogg_stream_packetout(packet):"
							 " packet was assembled normally.\n");
					 break;
				case  0: fprintf(finfo, "ogg_stream_packetout(packet):"
							 " there is insufficient data"
							 " available to complete a packet.\n");
					 break;
			}
		}
	}while(!b);

	return rv; 
}

/*
TODO: improve this comments. 
almost a wrapper to theora_decode_header().

Return values: see the return errors for theora_decode_header. 

TODO: documentation: 
explain where to find help in ogg and theora documentation as: 
http://xiph.org/ogg/doc/libogg/ogg_function_name.html
http://xiph.org/ogg/doc/libogg/ogg_datastructure_name.html
e.g:
http://xiph.org/ogg/doc/libogg/ogg_stream_init.html
http://xiph.org/ogg/doc/libogg/ogg_stream_state.html

*/

int etheora_decode_header(theora_info *ti, theora_comment *tc, 
		ogg_packet *o_packet, FILE *finfo){
	int rv; 
	rv = theora_decode_header(ti, tc, o_packet); 
        if ( rv ) {
                fprintf(finfo, "theora_decode_header(ogg_packet) can't start: ");
                switch (rv){
                        case OC_BADHEADER:
                                fprintf(finfo, "OC_BADHEARDER.\n");
                                break;
                        case OC_VERSION:
                                fprintf(finfo, "OC_VERSION.\n");
                                break;
                        case OC_NEWPACKET:
                                fprintf(finfo, "OC_NEWPACKET.\n");
                                break;
                        default:
                                fprintf(finfo, "Reason undocumented"
                                " by libtheora API (return value=%i).\n", rv);

                }
        }
        else fprintf(finfo, "o_packet show.\n");
	return rv; 
}

/*
etheora_decode_headers(): 
do the complete job of reading data from input fin untill getting all
headers, ready for the decode initialization. info about the video 
will be accordingly placed into theora_info structure. 

support for audio is still unavailable. 

return values:
 0, on success. 
<0, on unsuccess. 

error codes:
ETHEORA_NOT_ENOUGH_PACK, can't decode enough packets for the task. 

*/

int etheora_decode_headers(FILE *fin, theora_info *ti, etheora_streams_states 
	*ess, ogg_sync_state *os, theora_comment *tc,  FILE *finfo){

	ogg_packet o_pack; 
	int nh = 0; 
	/* TODO: support for audio is still unavailable. */
	/*decode packets and decode headers untill we get the
	  nh == 3 (3 -> number of theora headers).*/
        for(nh = 0; nh < 3; ){
                fprintf(finfo, ".................... header. %i \n", nh);
                if(etheora_decode_packet(fin, ess, os, &o_pack, finfo) < 0 ){
			return ETHEORA_NOT_ENOUGH_PACK; 
		}
                if(!etheora_decode_header(ti, tc, &o_pack, finfo)){
                        fprintf(finfo, "header show (%i/3).\n", ++nh);
                }
        }
	return 0; 
}

/*
etheora_decode_init(): is just a wrapper to 
theora_decode_init() to avoid users making direct calls to 
libtheora. 

TODO: document this (in context with the other functions):
we should not assume they know something about libtheora.  
*/

int etheora_decode_init(theora_state *ts, theora_info *ti){
	        return theora_decode_init(ts, ti);
}


/*
etheora_decode_frame():
this function exposes the buffer FILE *fin to etheora_decode_packet() until 
getting a packet that can be decodable into a yuv_frame; and returns this
frame in yuvb. 

arguments: TODO

return values: 
0, success. 
ETHEORA_ERR_DEC_NOMOREDATA, input buffer has (maybe yet) no data to be read.

*/

int etheora_decode_frame(FILE *fin, theora_state *ts, etheora_streams_states *ess,
        ogg_sync_state *sync_s,  yuv_buffer *yuvb, FILE *finfo){
	int a; 
	ogg_packet o_pack; 
	/*loop untill we get a decoded packet. */
	do{
		a = etheora_decode_packet(fin, ess, sync_s, &o_pack, finfo);
		if( a == ETHEORA_NOMOREDATA) return a;
		/*while this, a video packet wasn't decoded*/
	}while(a && a!=127 && (a != ETHEORA_NOMOREDATA));

	/*decode frame from the packet.*/
	a = theora_decode_packetin(ts, &o_pack);
	if(a) { 
		fprintf(finfo,"theora_decode_packetin(): frame does not"
			" contain encoded video data.\n" );
		return ETHEORA_ERR_DEC_NOVIDEODATA; 
	}
	a = theora_decode_YUVout(ts, yuvb);
	if(a) {
		fprintf(finfo,"theora_decode_YUVout():"
			" undocummented error\n.");
		return ETHEORA_ERR_UNDOCUMENTED; 
	}

	return 0; 
} 

/* 
etheora_444to420():  
 yuv_buffer conversion functions. use etheora_yuvbuf_resample(), and
 not these, as they'll become deprecated someday. 
*/

int etheora_444to420(yuv_buffer *yuv444, yuv_buffer *yuv420){
	int i, j; 
	/*copying Y*/
	for (i = 0; i  < yuv444->y_width; i++){
		for (j = 0; j < yuv444->y_height ; j++){

			int 
			 index420 = i + (yuv420->y_stride)*(j), 
			 index444 = i + (yuv444->y_stride)*(j); 
			if(yuv420->y_stride < 0)
				index420 += (yuv420->y_stride); 
			if(yuv444->y_stride < 0)
				index444 += (yuv444->y_stride); 

			yuv420->y[index420] = yuv444->y[index444]; 
		}
	}

	/*downsampling CbCr*/
	for (i = 0; i  < yuv444->uv_width; i=i+1){
		for (j = 0; j < yuv444->uv_height; j=j+1){

			int 
			 index420 = (i/2) + (yuv420->uv_stride)*(j/2), 
			 index444 =   i   +  yuv444->uv_stride *  j; 
			if(yuv420->y_stride < 0)
				index420 += (yuv420->y_stride); 
			if(yuv444->y_stride < 0)
				index444 += (yuv444->y_stride); 

			yuv420->u[index420] = yuv444->u[index444]; 
			yuv420->v[index420] = yuv444->v[index444]; 
		}
	/*TODO: this loop is accessing the same yuv420->x[index] 
	  more than once. should be optmized.*/
	}
	return 0; 
}

/* 
etheora_444to422():  
 yuv_buffer conversion functions. use etheora_yuvbuf_resample(), and
 not these, as they'll become deprecated someday. 
*/
int etheora_444to422(yuv_buffer *yuv444, yuv_buffer *yuv422){
	int i, j; 
	/*copying Y*/

	for (i = 0; i  < yuv444->y_width; i++){
		for (j = 0; j < yuv444->y_height ; j++){

			int 
			 index422 = i + (yuv422->y_stride)*(j), 
			 index444 = i + (yuv444->y_stride)*(j); 
			if(yuv422->y_stride < 0)
				index422 += (yuv422->y_stride); 
			if(yuv444->y_stride < 0)
				index444 += (yuv444->y_stride); 

			yuv422->y[index422] = yuv444->y[index444] ; 
		}
	}

	/*downsampling CbCr*/
	for (i = 0; i  < yuv444->uv_width; i=i+1){
		for (j = 0; j < yuv444->uv_height; j=j+1){
			int 
			 index422 = (i/2) + (yuv422->uv_stride) * j, 
			 index444 =   i   +  yuv444->uv_stride  * j; 
			if(yuv422->y_stride < 0)
				index422 += (yuv422->y_stride); 
			if(yuv444->y_stride < 0)
				index444 += (yuv444->y_stride); 
			yuv422->u[index422] = yuv444->u[index444]; 
			yuv422->v[index422] = yuv444->v[index444]; 
		}
	/*TODO: this loop is accessing the same yuv422->x[index] 
	  more than once. should be optmized.*/
	}
	return 0; 
}

/* 
etheora_420to444():  
 yuv_buffer conversion functions. use etheora_yuvbuf_resample(), and
 not these, as they'll become deprecated someday. 
*/
int etheora_420to444( yuv_buffer *yuv420,  yuv_buffer *yuv444){
	int i, j; 

	/*copying Y*/
	for (i = 0; i  < yuv444->y_width; i++){
		for (j = 0; j < yuv444->y_height ; j++){
			int 
			 index420 = i + (yuv420->y_stride)*(j), 
			 index444 = i + (yuv444->y_stride)*(j); 
			if(yuv420->y_stride < 0)
				index420 += (yuv420->y_stride); 
			if(yuv444->y_stride < 0)
				index444 += (yuv444->y_stride); 

			yuv444->y[index444] = yuv420->y[index420]; 
		}
	}

	/*upsampling CbCr*/
	for (i = 0; i  < yuv444->uv_width; i++){
		for (j = 0; j < yuv444->uv_height ; j++){
			int 
			 index420 = (i/2) + (yuv420->uv_stride)*(j/2), 
			 index444 =   i   + (yuv444->uv_stride) * j; 
			 if(yuv420->uv_stride < 0)
				index420 += (yuv420->y_stride); 
			 if(yuv444->uv_stride < 0)
				index444 += (yuv444->y_stride); 
			/*even rows in CbCr plane:*/
			yuv444->u[index444]     = yuv420->u[index420]; 
			yuv444->u[index444 +1 ] = yuv420->u[index420]; 
			yuv444->v[index444]     = yuv420->v[index420]; 
			yuv444->v[index444 +1 ] = yuv420->v[index420]; 
			/*odd rows in CbCr plane are equal to the even
			ones:*/
			index444 = i + (yuv444->uv_stride) * (j+1); 
			yuv444->u[index444]     = yuv420->u[index420]; 
			yuv444->u[index444 +1 ] = yuv420->u[index420]; 
			yuv444->v[index444]     = yuv420->v[index420]; 
			yuv444->v[index444 +1 ] = yuv420->v[index420]; 
			/* (Note +1 makes odd columns in CbCr plane
			equal to the even ones.)*/
		}
	}
	return 0; 
}

/* 
etheora_422to444():  
 yuv_buffer conversion functions. use etheora_yuvbuf_resample(), and
 not these, as they'll become deprecated someday. 
*/

int etheora_422to444( yuv_buffer *yuv422,  yuv_buffer *yuv444){
	int i, j; 
	/*copying Y*/

	for (i = 0; i  < yuv444->y_width; i++){
		for (j = 0; j < yuv444->y_height ; j++){
			int 
			 index422 = i + (yuv422->y_stride)*(j), 
			 index444 = i + (yuv444->y_stride)*(j); 
			if(yuv422->y_stride < 0)
				index422 += (yuv422->y_stride); 
			if(yuv444->y_stride < 0)
				index444 += (yuv444->y_stride); 

			yuv444->y[index444] = yuv422->y[index422]; 
		}
	}
	/*upsampling CbCr*/
	for (i = 0; i  < yuv444->uv_width; i= i + 2){
		for (j = 0; j < yuv444->uv_height; j= j + 1){
			int 
			 index422 =(i/2) + (yuv422->uv_stride) * j, 
			 index444 =  i   +  yuv444->uv_stride  * j; 
			yuv444->u[index444]     = yuv422->u[index422]; 
			yuv444->u[index444 +1 ] = yuv422->u[index422]; 
			yuv444->v[index444]     = yuv422->v[index422]; 
			yuv444->v[index444 +1 ] = yuv422->v[index422]; 
			/* (Note +1 makes odd columns in CbCr plane
			equal to the even ones.)*/
		}
	}
	return 0; 
}

/*
etheora_pixel_yuv2rgb(): converts the pixel (y,u,v) into (r,g,b), 
according to the V4L2 Video Imagem Format Specification which can be found 
at:
http://www.thedirks.org/v4l2/v4l2fmt.htm (a copy is supplied in the doc dir)

probably you want to use etheora_pixel_rgb_reescale() after using this. 

the equations here were modified so yuv values here must be in the 
range 0..255, and not -127..127 for uv as in the equations in that
specification. 

Returns: 0 on success. 

*/
int etheora_pixel_yuv2rgb(const float y, const float u, const float v, 
                          float *r, float *g, float *b){

	*r = y +                 1.402*(v-127); 
	*g = y - 0.344*(u-127) - 0.714*(v-127); 
	*b = y + 1.722*(u-127); 
	return 0; 
} 

/*
etheora_pixel_rgb2yuv(): converts the pixel (r,g,b) into (y,u,v), according
to the V4L2 Video Imagem Format Specification which can be found at:
http://www.thedirks.org/v4l2/v4l2fmt.htm 

probably you want to use etheora_pixel_yuv_reescale() after using this. 

the equations here were modified so yuv values here are procude in the 
range 0..255, and not -127..127 for uv as in the equations in that 
specification. 

Returns: 0 on success. 

*/
int etheora_pixel_rgb2yuv(const float r, const float g, const float b, 
                          float *y, float *u, float *v){

	*y = 0.2990 *r + 0.5670*g + 0.1140*b;
	*u = -0.1687*r - 0.3313*g + 0.5000*b + 127;
	*v = 0.5000 *r - 0.4187*g - 0.0813*b + 127; 
	return 0; 

} 

/*
etheora_pixel_YCbCr2yuv(): converts the pixel (Y,Cb,Cr) into (y,u,v), according
to the V4L2 Video Imagem Format Specification which can be found at:
http://www.thedirks.org/v4l2/v4l2fmt.htm (a copy is supplied in the doc dir)

but the u, v channels are output in the range 0..255. the range for y and 
YCbCr are the same as specified. 

Returns: 0 on success. 

*/
int etheora_pixel_YCbCr2yuv(const float Y, const float Cb, const float Cr, 
                          float *y, float *u, float *v){

	etheora_interpolate(16 , 235, Y , 0, 255, *y); 
	etheora_interpolate(16 , 140, Cb, 0, 255, *u); 
	etheora_interpolate(16 , 140, Cr, 0, 255, *v); 
	return 0; 

} 

/*
etheora_pixel_yuv2YCbCr(): converts the pixel (y,u,v) into (Y,Cb,Cr), according
to the V4L2 Video Imagem Format Specification which can be found at:
http://www.thedirks.org/v4l2/v4l2fmt.htm (a copy is supplied in the doc dir)

but the u, v channels must be input in the range 0..255. the range for y and 
YCbCr are the same as specified. 

Returns: 0 on success. 

*/
int etheora_pixel_yuv2YCbCr(const float y, const float u, const float v, 
                          float *Y, float *Cb, float *Cr){

	etheora_interpolate(0, 255, y, 16 , 235, *Y ); 
	etheora_interpolate(0, 255, u, 16 , 140, *Cb); 
	etheora_interpolate(0, 255, v, 16 , 140, *Cr); 
	return 0; 

} 

/*
etheora_pixel_yuv_reescale(): "clamps" the pixel (y,u,v) to the legal 
range 0..255. Read the documentation below, you're probably interested in 
this function, although etheora_pixel_yuv_clamp() function is seemed. 

The yuv values produced by etheora_pixel_rgb2yuv() are in these ranges:
     Min   Max
y     0   249.9
u   - .5  254.5
v   - .5  254.5

This function interprets that any value in the range 0..255 should
be legal for y.  If the interpretation you use says that y is 
already in the legal range, you need the etheora_pixel_yuv_clamp() 
function.  Both functions work in the same way for u and v. 

The need of "clamp"ing values to the legal range is stated in the 
V4L2 Video Imagem Format Specification which can be found at:
http://www.thedirks.org/v4l2/v4l2fmt.htm
(a copy is supplied in the doc dir)

Returns: 0 on success. 

*/
int etheora_pixel_yuv_reescale(float *y, float *u, float *v){
	etheora_interpolate( 0  , 249.9, *y, 0, 255, *y); 
	etheora_interpolate(- .5, 254.5, *u, 0, 255, *u); 
	etheora_interpolate(- .5, 254.5, *v, 0, 255, *v); 
	return 0; 
}

/*
etheora_pixel_yuv_unreescale() does the inverse operations of the
etheora_pixel_yuv_reescale() function, ie, returns (y,u,v) to the old
range. Refer to etheora_pixel_yuv_reescale() for more information. 

Returns: 0 on success. 

*/
int etheora_pixel_yuv_unreescale(float *y, float *u, float *v){
	etheora_interpolate(0, 255, *y,  0  , 249.9, *y); 
	etheora_interpolate(0, 255, *u, - .5, 254.5, *u); 
	etheora_interpolate(0, 255, *v, - .5, 254.5, *v); 
	return 0; 
}

/*
etheora_pixel_rgb_reescale(): "clamps" the pixel (r,g,b) to the legal 
range 0..255. 

The rgb values produced by etheora_pixel_yuv2rgb() are in these ranges:
     Min    Max
r  -177.8  432.8
g  -132.37 389.37
b  -218.69 473.69

The need of "clamp"ing values to the legal range is stated in the 
V4L2 Video Imagem Format Specification which can be found at:
http://www.thedirks.org/v4l2/v4l2fmt.htm
(a copy is supplied in the doc dir)

Returns: 0 on success. 

TODO: these functions presume 8 bit deep channels. would we
like a more generic function? 

TODO: wouldn't it be better if these functions were macros? 

*/

int etheora_pixel_rgb_reescale(float *r, float *g, float *b){
	etheora_interpolate(-177.8  , 432.8  , *r, 0, 255, *r); 
	etheora_interpolate(-132.366, 389.366, *g, 0, 255, *g); 
	etheora_interpolate(-218.694, 473.694, *b, 0, 255, *b); 
	return 0; 
}

/*
etheora_pixel_rgb_unreescale() does the inverse operations of the
etheora_pixel_rgb_reescale() function, ie, returns (r,g,b) to the old
range. Refer to etheora_pixel_rgb_reescale() for more information. 

Returns: 0 on success. 

*/
int etheora_pixel_rgb_unreescale(float *r, float *g, float *b){
	etheora_interpolate(0, 255, *r, -177.8  , 432.8  , *r); 
	etheora_interpolate(0, 255, *g, -132.366, 389.366, *g); 
	etheora_interpolate(0, 255, *b, -218.694, 473.694, *b); 
	return 0; 
}


/*
etheora_yuvbuf_yuv_pixelget(): 
returns in (y, u, v) the yuv pixel (i, j). 

arguments: 
yuv: yuv_buffer where to get the pixel. 
y,u,v: pointers to where to put the pixel data. 
i,j: x,y coordinate of the pixel in the buffer. 

return values: 
 0, on success;
<0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more pointers required by this
function was/were passed as NULL. 
ETHEORA_ERR_STRANGECHROMA, yuv_buffer isn't 420, 422 neither 444.
*/

int etheora_yuvbuf_yuv_pixelget(yuv_buffer *yuv, ogg_uint32_t i,
	ogg_uint32_t j,
	unsigned char *y, unsigned char *u, unsigned char *v){

	char di, dj; 
	/*sanity tests. */
	if(y == NULL || u == NULL || v == NULL || yuv == NULL) 
		return ETHEORA_ERR_NULL_ARGUMENT; 

	switch(etheora_yuvbuf_pixelformat(yuv)){ 
		case OC_PF_420: di = 2; dj = 2; 
				break; 
		case OC_PF_422: di = 2; dj = 1; 
				break; 
		case OC_PF_444: di = 1; dj = 1; 
				break; 
		default: 	return ETHEORA_ERR_STRANGECHROMA; 
	}
	etheora_yuv_getelem(*y, (*yuv), CH_Y, i, j);
	etheora_yuv_getelem(*u, (*yuv), CH_U, i/di, j/dj);
	etheora_yuv_getelem(*v, (*yuv), CH_V, i/di, j/dj);
	return 0; 

}


/*
etheora_yuvbuf_rgb_pixelget(): 
returns in (r, g, b) the yuv pixel (i, j) (correctly converted
to rgb).

arguments: 
yuv: yuv_buffer where to get the pixel. 
r,g,b: pointers to where to put the pixel data. 
i,j: x,y coordinate of the pixel in the buffer. 

return values: 
 0, on success;
<0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more pointers required by this
function was/were passed as NULL. 
ETHEORA_ERR_STRANGECHROMA, yuv_buffer isn't 420, 422 neither 444.
*/

int etheora_yuvbuf_rgb_pixelget(yuv_buffer *yuv, ogg_uint32_t i, 
	ogg_uint32_t j, float *r, float *g, float *b){

	unsigned char y, u, v; 

	/*sanity tests. */
	if(r == NULL || g == NULL || b == NULL || yuv == NULL) 
		return ETHEORA_ERR_NULL_ARGUMENT; 

	if (etheora_yuvbuf_yuv_pixelget(yuv, i, j, &y, &u, &v))
		return ETHEORA_ERR_STRANGECHROMA; 
	etheora_pixel_yuv2rgb(y, u, v, r, g, b);
	etheora_pixel_rgb_reescale( r, g, b );
	return 0; 
}

/*
etheora_yuvbuf_yuv_pixelput(): 
puts in the yuv (i, j) coordinate the pixel (y, u, v). 

arguments: 
yuv: yuv_buffer where to put the pixel. 
y,u,v: pointers to where to get the pixel data. 
i,j: x,y coordinate where to put the pixel in the yuv_buffer. 

return values: 
 0, on success;
<0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more pointers required by this
function was/were passed as NULL. 
ETHEORA_ERR_STRANGECHROMA, yuv_buffer isn't 420, 422 neither 444.
*/

int etheora_yuvbuf_yuv_pixelput(yuv_buffer *yuv, ogg_uint32_t i, 
	ogg_uint32_t j,
	unsigned char *y, unsigned char *u, unsigned char *v){

	char di, dj; 
	/*sanity tests. */
	if(y == NULL || u == NULL || v == NULL || yuv == NULL) 
		return ETHEORA_ERR_NULL_ARGUMENT; 

	switch(etheora_yuvbuf_pixelformat(yuv)){ 
		case OC_PF_420: di = 2; dj = 2; 
				break; 
		case OC_PF_422: di = 2; dj = 1; 
				break; 
		case OC_PF_444: di = 1; dj = 1; 
				break; 
		default: 	return ETHEORA_ERR_STRANGECHROMA; 
	}
	etheora_yuv_putelem(*y, (*yuv), CH_Y, i, j);
	etheora_yuv_putelem(*u, (*yuv), CH_U, i/di, j/dj);
	etheora_yuv_putelem(*v, (*yuv), CH_V, i/di, j/dj);
	return 0; 

}


/*
etheora_yuvbuf_rgb_pixelput(): 
puts the (r, g, b) pixel in the you coordinate(i, j) (correctly converted
to yuv).

arguments: 
yuv: yuv_buffer where to put the pixel. 
r,g,b: pointers to where to get the pixel data. 
i,j: x,y coordinate of the pixel in the yuv yuv_buffer. 

return values: 
 0, on success;
<0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more pointers required by this
function was/were passed as NULL. 
ETHEORA_ERR_STRANGECHROMA, yuv_buffer isn't 420, 422 neither 444.
*/

int etheora_yuvbuf_rgb_pixelput(yuv_buffer *yuv, ogg_uint32_t i, 
	ogg_uint32_t j, float *r, float *g, float *b){

	float y, u, v; 
	unsigned char a0, a1, a2; 

	/*sanity tests. */
	if(r == NULL || g == NULL || b == NULL || yuv == NULL) 
		return ETHEORA_ERR_NULL_ARGUMENT; 

	etheora_pixel_rgb2yuv(*r, *g, *b, &y, &u, &v);
	etheora_pixel_yuv_reescale( &y, &u, &v );
	a0 = (unsigned char) y;
	a1 = (unsigned char) u;
	a2 = (unsigned char) v;
	if(etheora_yuvbuf_yuv_pixelput(yuv, i, j, &a0, &a1, &a2))
		return ETHEORA_ERR_STRANGECHROMA; 

	return 0; 
}


/*
etheora_yuvbuf_pixelformat():
determines the chroma/pixelformat of a yuv_buffer. 

arguments: 
yuv: yuv_buffer whose chroma is to be determined. 

return values: 
OC_PF_420, 4:2:0 chroma
OC_PF_422, 4:2:2 chroma
OC_PF_444, 4:4:4 chroma
OC_PF_RSVD, on error. 

TODO: tested in revision (opensvn) 860. maybe deserve a separated test. 
*/

theora_pixelformat etheora_yuvbuf_pixelformat(yuv_buffer *yuv){
	if(yuv->y_width == yuv->uv_width) return OC_PF_444; 
	if(yuv->y_width/2 == yuv->uv_width){ 
		if (yuv->y_height/2 == yuv->uv_height) return OC_PF_420; 
		else if (yuv->y_height == yuv->uv_height) return OC_PF_422;
	}
	return OC_PF_RSVD; 
}


/* 

etheora_yuvbuf_resample(): 
converts a yuv_buffer into another with different chroma subsampling. 
the following conversions are supported: 

420 <-> 444
422 <-> 444

note: it's not much interesting to have convertion between two 
chroma SUBsampling, but if needed, it's possible to make 
such a convertion by making a 420 or 422 to 444 convertion 
before. 

In future, support to direct convertion will be added (TODO).
In future, support to copy (same chroma) will be added (TODO).

arguments:
yuv_source: yuv_buffer containing yuv data source. 
yuv_dest: yuv_buffer containing yuv data dest. 

return values: 
 0, on success, 
<0, on unsucess.

error codes: 
ETHEORA_ERR_STRANGECHROMA,   a yuv_buffer isn't 420, 422 neither 444.
ETHEORA_NOT_YET_IMPLEMENTED, feature not yet implemented (420<->422 
				conversion or same chroma conversion). 

*/

int  etheora_yuvbuf_resample(yuv_buffer *yuv_source, yuv_buffer *yuv_dest){

	/*TODO: this function must be rewritten. Really isn't needed 
	 all the conversion functions below, we can control some few
	 parameters and get a unique function. */

	theora_pixelformat a, b; 
	a = etheora_yuvbuf_pixelformat(yuv_source); 
	b = etheora_yuvbuf_pixelformat(yuv_dest); 
	if (a == OC_PF_RSVD || b == OC_PF_RSVD) 
		return ETHEORA_ERR_STRANGECHROMA;
	if (a != OC_PF_444 && b != OC_PF_444) 
		return ETHEORA_NOT_YET_IMPLEMENTED;
	if (a == b) 
		return ETHEORA_NOT_YET_IMPLEMENTED;
	if ( a == OC_PF_444 && b == OC_PF_422)
		return etheora_444to422(yuv_source, yuv_dest); 
	if ( a == OC_PF_444 && b == OC_PF_420)
		return etheora_444to420(yuv_source, yuv_dest); 
	if ( b == OC_PF_444 && a == OC_PF_422)
		return etheora_422to444(yuv_source, yuv_dest); 
	if ( b == OC_PF_444 && a == OC_PF_420)
		return etheora_420to444(yuv_source, yuv_dest); 
	return ETHEORA_ERR_UNKNOWN; 
}


/*
etheora_enc_setup():
setup the etheora_ctx context for decoding process.

usually called: 
after  none
before etheora_enc_start()

arguments: 
ec: pointer to an etheora context. 
frame_width: frame horizontal dimension. Must be divisible by 16.  
frame_height: frame vertical dimension. Must be divisible by 16.  
aspect: video aspect ratio. 
fps_numerator: frames per second ratio numerator. 
fps_denominator: frames per second ratio denominator. 
fout: pointer to the file descriptor where to output ogg/theora video. 
finfo: pointer to a file descriptor whete to output debug info. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more NULL arguments was/were passed. 
ETHEORA_ERR_MALLOC, can't allocate memory. 
ETHEORA_ERR_BAD_FRAME_DIMENSION, one or more frame dimension isn't 
				divisible by 16. 

*/

int  etheora_enc_setup(etheora_ctx *ec,
		ogg_uint32_t frame_width, ogg_uint32_t frame_height,
		etheora_aspect aspect, ogg_uint32_t  fps_numerator,
		ogg_uint32_t  fps_denominator, FILE *fout, FILE *finfo){

       /*sanity test.*/
       if(ec!= NULL && finfo != NULL && fout != NULL ){
	       ec->finfo = finfo; 
	       ec->fogg = fout; 
       }
       else return ETHEORA_ERR_NULL_ARGUMENT; 

       if(etheora_filltinfo_nooffset(&ec->ti,
                frame_width,
                frame_height,
                aspect,
                fps_numerator,
                fps_denominator))
	       return ETHEORA_ERR_BAD_FRAME_DIMENSION; 
	else return 0; 

	
}

/*
etheora_enc_start():
starts the encoding process. effectively allocate memory for frame buffer
(ec->yuv) and write video headers to ec->fogg.  

usually called: 
after  etheora_enc_setup() 
before etheora_enc_yuv_draw() or  etheora_enc_rgb_draw()

arguments: 
ec: pointer to an etheora context. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
ETHEORA_ERR_MALLOC, can't allocate memory. 
ETHEORA_ERR_CANT_START, theora encoder refuses to start 
		(theora_encode_init()).
ETHEORA_ERR_CANT_WRITE_HEADER, can't write headers to file. 
*/

int  etheora_enc_start(etheora_ctx *ec){

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	/*TODO: currently, only video stream supported. */
	if(etheora_streams_states_init(&ec->ess, 1))
		return ETHEORA_ERR_MALLOC; 

	if(etheora_fillyuv(&ec->ti, &ec->yuv, ec->finfo))
		return ETHEORA_ERR_MALLOC; 

	if(etheora_encode_init(&ec->ts, &ec->ti))
		return ETHEORA_ERR_CANT_START; 

	if(etheora_writeheaders(&ec->ts,
				&ec->ess.streams[0],
				&ec->tc,
				ec->fogg, ec->finfo) < 0)
		return ETHEORA_ERR_CANT_WRITE_HEADER; 

	return 0; 
}

/*
etheora_enc_nextframe(): 
submit the contents of ec->yuv frame buffer to the action of 
encoder. 

usually called: 
after  etheora_enc_yuv_draw() or  etheora_enc_rgb_draw()
before etheora_enc_yuv_draw() or  etheora_enc_rgb_draw()

arguments: 
ec: pointer to an etheora context. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

TODO: etheora_writeframe() need error codes. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
any other: can't write frame data to file. 

*/

int  etheora_enc_nextframe(etheora_ctx *ec){

	ogg_page  page; 

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	return etheora_writeframe(&ec->ts,
			&ec->yuv,
			ETHEORA_NOTLAST_FRAME,
			&ec->ess.streams[0],
			&page,
			ec->fogg, ec->finfo);

	return 0; 
}

/*
etheora_enc_finish(): 
submits the content of frame buffer (ec->yuv) to the encoder. 
frees the structures allocated by etheora. 

usually called: 
after  etheora_enc_yuv_draw() or  etheora_enc_rgb_draw()
before etheora_enc_yuv_draw() or  etheora_enc_rgb_draw()

arguments: 
ec: pointer to an etheora context. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

TODO: etheora_writeframe() need error codes. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
ETHEORA_ERR_WRITE_FAILED, can't write last frame. 

*/

int  etheora_enc_finish(etheora_ctx *ec){

	ogg_page  page; 

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	if(etheora_writeframe(&ec->ts,
			&ec->yuv,
			ETHEORA_LAST_FRAME,
			&ec->ess.streams[0],
			&page,
			ec->fogg, ec->finfo))
		return ETHEORA_ERR_WRITE_FAILED; 
	etheora_flushpage(&ec->ess.streams[0], &page,	
			ec->fogg, ec->finfo); 
	theora_info_clear(&ec->ti); 
	theora_comment_clear(&ec->tc); 
	theora_clear(&ec->ts); 
	etheora_freeyuv(&ec->yuv); 
	etheora_free_streams_states(&ec->ess); 
	return 0; 
}

/*
etheora_dec_setup():
setup the etheora_ctx context for decoding process.

usually called: 
after  none
before etheora_dec_start() 

arguments: 
ec: pointer to an etheora context. 
fin: pointer to the file descriptor where to read ogg/theora video. 
finfo: pointer to a file descriptor whete to output debug info. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more NULL pointer was/were passed. 
*/

int  etheora_dec_setup(etheora_ctx *ec, FILE *fin, FILE *finfo){

	/*sanity test.*/
	if(ec == NULL || fin == NULL || finfo == NULL) 
		return ETHEORA_ERR_NULL_ARGUMENT;

	ec->fogg = fin; 
	ec->finfo = finfo; 

	return 0; 
}

/*
etheora_dec_start():
starts the decoding process. effectively allocate memory for frame buffer
ec->yuv and read video headers from ec->fogg.  

usually called: 
after  etheora_dec_setup() 
before etheora_dec_nextframe() 

arguments: 
ec: pointer to an etheora context. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
ETHEORA_ERR_MALLOC, can't allocate memory necessary for the process.
ETHEORA_ERR_CANT_READ_HEADER, can't video read headers. 
ETHEORA_ERR_CANT_START, theora refuses to start encoding process. 
*/

int  etheora_dec_start(etheora_ctx *ec){

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

        if(etheora_structs_init(&ec->ti, &ec->tc, &ec->os, 
		&ec->ess, 2))
		return ETHEORA_ERR_MALLOC; 

        if( etheora_decode_headers(ec->fogg, &ec->ti, &ec->ess, &ec->os, 
		&ec->tc, ec->finfo))
		return ETHEORA_ERR_CANT_READ_HEADER; 
	

        if(etheora_decode_init(&ec->ts, &ec->ti))
		return ETHEORA_ERR_CANT_START; 

	return 0; 
}

/*
etheora_dec_nextframe(): 
ask the decoder to decode next available frame, and as result the 
decoded buffer is made accessible in the frame buffer (ec->yuv).

usually called: 
after  etheora_dec_start() 
before etheora_dec_yuv_read() etheora_dec_rgb_read() 

arguments: 
ec: pointer to an etheora context. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
ETHEORA_ERR_READ_FAILED, can't read next frame. 

*/

int  etheora_dec_nextframe(etheora_ctx *ec){

	int a; 

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	a = etheora_decode_frame(ec->fogg, &ec->ts, &ec->ess, 
	&ec->os, &ec->yuv, ec->finfo); 

	if(a == ETHEORA_NOMOREDATA) return a; 
	else if(a < 0) return ETHEORA_ERR_READ_FAILED;  

	return 0; 
}

/*
etheora_dec_finish(): 
frees the structures allocated by etheora. 

usually called: 
after  etheora_dec_yuv_read() etheora_dec_rgb_read() 
before none

arguments: 
ec: pointer to an etheora context. 

returns: 
   0, on success, 
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
 
*/

int  etheora_dec_finish(etheora_ctx *ec){

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	ogg_sync_clear(&ec->os); 
	theora_info_clear(&ec->ti); 
	theora_comment_clear(&ec->tc); 
	theora_clear(&ec->ts); 
	etheora_free_streams_states(&ec->ess); 

	return 0; 
}


/*
etheora_enc_yuv_draw(): 

puts in the current frame buffer (ec->yuv) (i, j) coordinate the 
pixel (y, u, v). 

usually called: 
after  etheora_enc_start() etheora_enc_nextframe()
before etheora_enc_nextframe() etheora_enc_finish()

arguments: 
ec: pointer to an etheora context. 
y,u,v: pointers to where to get the pixel data. 
i,j: x,y coordinate where to put the pixel in the yuv_buffer. 

returns: 
   0, on success;
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer.
ETHEORA_ERR_STRANGECHROMA, ec contains strange data. 
*/

int  etheora_enc_yuv_draw(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        unsigned char y, unsigned char u, unsigned char v){

	unsigned char a = y, b = u, c = v; 

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	return etheora_yuvbuf_yuv_pixelput(&ec->yuv, i, j, &a, &b, &c); 

}

/*
etheora_enc_rgb_draw(): 
puts the (r, g, b) pixel in the you coordinate(i, j) (correctly converted
to yuv) of the current frame buffer (ec->yuv).

usually called: 
after  etheora_enc_start() etheora_enc_nextframe()
before etheora_enc_nextframe() etheora_enc_finish()

arguments: 
ec: pointer to an etheora context. 
r,g,b: pointers to where to get the pixel data. 
i,j: x,y coordinate of the pixel in the yuv yuv_buffer. 

returns: 
   0, on success;
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, ec is NULL pointer. 
ETHEORA_ERR_STRANGECHROMA, ec contains strange data. 
*/

int etheora_enc_rgb_draw(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        float r, float g, float b){

	float x = r, y = g, z = b; 

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	return etheora_yuvbuf_rgb_pixelput(&ec->yuv, i, j, &x, &y, &z); 
}


/*
etheora_dec_yuv_read(): 
returns in (y, u, v) the yuv pixel (i, j) of the current frame 
buffer (ec->yuv).

usually called: 
after  etheora_dec_nextframe()
before etheora_dec_nextframe() etheora_dec_finish()

arguments: 
ec: pointer to an etheora context.
y,u,v: pointers to where to put the pixel data. 
i,j: x,y coordinate of the pixel in the buffer. 

return values: 
   0, on success;
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more pointers required by this
function was/were passed as NULL. 
ETHEORA_ERR_STRANGECHROMA, ec contains strange data. 
*/

int  etheora_dec_yuv_read(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        unsigned char *y, unsigned char *u, unsigned char *v){

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	return etheora_yuvbuf_yuv_pixelget(&ec->yuv, i, j, y, u, v); 

}

/*
etheora_dec_rgb_read(): 
returns in (r, g, b) the yuv pixel (i, j) (correctly converted
to rgb) of the current frame buffer (ec->yuv).

usually called: 
after  etheora_dec_nextframe()
before etheora_dec_nextframe() etheora_dec_finish()

arguments: 
ec: pointer to an etheora context.
r,g,b: pointers to where to put the pixel data. 
i,j: x,y coordinate of the pixel in the buffer. 

return values: 
   0, on success;
!= 0, on unsuccess. 

error codes: 
ETHEORA_ERR_NULL_ARGUMENT, one or more pointers required by this
function was/were passed as NULL. 
ETHEORA_ERR_STRANGECHROMA, ec contains strange data. 
*/

int etheora_dec_rgb_read(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        float *r, float *g, float *b){

	/*sanity test.*/
	if(ec == NULL) return ETHEORA_ERR_NULL_ARGUMENT;

	return etheora_yuvbuf_rgb_pixelget(&ec->yuv, i, j, r, g, b); 
}


/*
etheora_get_width():
attribute getting function. 

arguments: 
ec: pointer to an etheora context.

returns:
the value of the requisited attribute. 
*/

ogg_uint32_t etheora_get_width(etheora_ctx *ec){
	return ec->ti.width; 
}

/*
etheora_get_height():
attribute getting function. 

arguments: 
ec: pointer to an etheora context.

returns:
the value of the requisited attribute. 
*/
ogg_uint32_t etheora_get_height(etheora_ctx *ec){
	return ec->ti.height; 
}

/*
etheora_get_fps_numerator():
attribute getting function. 

arguments: 
ec: pointer to an etheora context.

returns:
the value of the requisited attribute. 
*/
ogg_uint32_t etheora_get_fps_numerator(etheora_ctx *ec){
	return ec->ti.fps_numerator; 
}

/*
etheora_get_fps_denominator():
attribute getting function. 

arguments: 
ec: pointer to an etheora context.

returns:
the value of the requisited attribute. 
*/
ogg_uint32_t etheora_get_fps_denominator(etheora_ctx *ec){
	return ec->ti.fps_denominator; 
}

/*
etheora_get_aspect_numerator():
attribute getting function. 

arguments: 
ec: pointer to an etheora context.

returns:
the value of the requisited attribute. 
*/
ogg_uint32_t etheora_get_aspect_numerator(etheora_ctx *ec){
	return ec->ti.aspect_numerator; 
}

/*
etheora_get_aspect_denominator():
attribute getting function. 

arguments: 
ec: pointer to an etheora context.

returns:
the value of the requisited attribute. 
*/
ogg_uint32_t etheora_get_aspect_denominator(etheora_ctx *ec){
	return ec->ti.aspect_denominator; 
}


