#ifndef __ETHEORA_INTERNAL_LIB__
#define __ETHEORA_INTERNAL_LIB__

/* maybe these functions below won't be public anymore. */

/* general encoding and decoding functions. */

int etheora_streams_states_init(etheora_streams_states *ess, int n_streams);

int  etheora_fillyuv(theora_info *ti, yuv_buffer *yuv, FILE *finfo); 

void etheora_freeyuv(yuv_buffer *yuv); 

void etheora_free_streams_states(etheora_streams_states *ess); 

/*encode functions.*/

/*todo should be called etheora_encode_configure() or similar.*/
int  etheora_filltinfo_nooffset(theora_info *ti, 
		ogg_uint32_t frame_width, ogg_uint32_t frame_height, 
		etheora_aspect aspect, ogg_uint32_t  fps_numerator, 
		ogg_uint32_t  fps_denominator); 

/* TODO: maybe should never be public: */
int  etheora_filltinfo_hardvalues(theora_info *ti); 

int  etheora_encode_init(theora_state *ts, theora_info *ti); 

/*TODO: should be called etheora_encode_headers() or similar, and maybe
 use streams_states. */
int  etheora_writeheaders(theora_state *ts, 
		ogg_stream_state *stream, 
		theora_comment *tc, 
		FILE *fout, FILE *finfo); 

/*TODO: should be called etheora_encode_headers() or similar, and maybe 
 use streams_states. */
int  etheora_writeframe(theora_state *ts,
		yuv_buffer *yuv,
		int iflastframe,
		ogg_stream_state *stream,
		ogg_page *page,
		FILE *fout, FILE *finfo); 

int  etheora_flushpage(ogg_stream_state *stream, ogg_page *page,
		FILE *fout, FILE *finfo); 



/*decode functions.*/

/*TODO: shouldn't all these functions be _decode_ ? */

int etheora_buffer_data(FILE *in, ogg_sync_state *os); 

/*TODO: _structs_init() and decode_headers() should have the same
 parameters order. but i need to think what's the best.*/

int etheora_structs_init(theora_info *ti, theora_comment *tc, ogg_sync_state
        *os, etheora_streams_states *ess, int n_streams); 

int etheora_decode_packet(FILE *fin, etheora_streams_states *ess,
        ogg_sync_state *sync_s,  ogg_packet *packet, FILE *finfo); 

int etheora_decode_header(theora_info *ti, theora_comment *tc,
                ogg_packet *o_packet, FILE *finfo); 

int etheora_decode_headers(FILE *fin, theora_info *ti, etheora_streams_states
        *ess, ogg_sync_state *sync_s, theora_comment *tc,  FILE *finfo); 

int  etheora_decode_init(theora_state *ts, theora_info *ti); 

int etheora_decode_frame(FILE *fin, theora_state *ts, etheora_streams_states 
	*ess, ogg_sync_state *sync_s,  yuv_buffer *yuvb, FILE *finfo); 




/* frame buffer pixel accessing and changing functions. */

int etheora_yuvbuf_yuv_pixelget(yuv_buffer *yuv, ogg_uint32_t i, ogg_uint32_t j,
        unsigned char *y, unsigned char *u, unsigned char *v); 

int etheora_yuvbuf_rgb_pixelget(yuv_buffer *yuv, ogg_uint32_t i, ogg_uint32_t j,
        float *r, float *g, float *b); 

int etheora_yuvbuf_yuv_pixelput(yuv_buffer *yuv, ogg_uint32_t i, ogg_uint32_t j,
        unsigned char *y, unsigned char *u, unsigned char *v); 

int etheora_yuvbuf_rgb_pixelput(yuv_buffer *yuv, ogg_uint32_t i, ogg_uint32_t j,
        float *r, float *g, float *b); 


/* conversion functions. don't use them, use 
   etheora_yuvbuf_resample(). */

int  etheora_444to420(yuv_buffer *yuv444, yuv_buffer *yuv420);

int  etheora_444to422(yuv_buffer *yuv444, yuv_buffer *yuv422);

int  etheora_420to444(yuv_buffer *yuv420, yuv_buffer *yuv444);

int  etheora_422to444(yuv_buffer *yuv422, yuv_buffer *yuv444);


#endif /*__ETHEORA_INTERNAL_LIB__*/
