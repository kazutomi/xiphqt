
#ifndef __ETHEORA_LIB__
#define __ETHEORA_LIB__


#include <theora/theora.h>
#include <ogg/ogg.h>
#include <stdio.h> 


#if 0
#define ETHEORA_ASPECT_NORMAL      0x32 /*4:3 aspect ratio*/
#define ETHEORA_ASPECT_WIDE_SCREEN 0xF8 /*16:9 aspect ratio*/
#define ETHEORA_ASPECT_PRESERVE    0x00 /*preserves frame width:height*/
#endif 

#define ETHEORA_LAST_FRAME            1 
#define ETHEORA_NOTLAST_FRAME         0 
#define ETHEORA_PIXEL_MIN             0 
#define ETHEORA_PIXEL_MAX           255 /*pixel deepth. don't change*/

/*default values for filling structures.*/
#define ETHEORA_TINFO_BITRATE 		256000
#define ETHEORA_TINFO_QUALITY 		63
#define ETHEORA_TINFO_PIXELFORMAT	OC_PF_420
#define ETHEORA_TINFO_COLORSPACE	OC_CS_UNSPECIFIED

#define MIN_BUFFER_READ 1024

#define ETHEORA_ERR_HEADERS	        -1
#define ETHEORA_ERR_OGG_INTERNAL        -2
#define ETHEORA_ERR_COMMENT	        -3
#define ETHEORA_ERR_TABLES	        -4
#define ETHEORA_ERR_MALLOC              -5
#define ETHEORA_ERR_NULL_ARGUMENT       -6
#define ETHEORA_ERR_UNDOCUMENTED 	-7
#define ETHEORA_NOT_ENOUGH_PACK		-8
#define ETHEORA_NOT_YET_IMPLEMENTED	-9 
#define ETHEORA_ERR_STRANGECHROMA	-10 
#define ETHEORA_ERR_UNKNOWN		-11 
#define ETHEORA_ERR_BAD_FRAME_DIMENSION -12
#define ETHEORA_ERR_CANT_START		-13
#define ETHEORA_ERR_CANT_WRITE_HEADER	-14
#define ETHEORA_ERR_WRITE_FAILED	-15
#define ETHEORA_ERR_CANT_READ_HEADER	-16
#define ETHEORA_ERR_READ_FAILED		-17

#define ETHEORA_NOMOREDATA 		-128
#define ETHEORA_ERR_BAD_STREAM          -129
#define ETHEORA_ERR_OGG_STREAM_PAGEIN   -130
#define ETHEORA_ERR_DEC_NOVIDEODATA     -131  



/* possible yuv channels.*/
enum etheora_yuv_channels {CH_Y, CH_U, CH_V}; 

/* video aspect ratio. normal 4:3, wide_screen 16:9, 
   preserve frame_width:frame_height */
typedef enum etheora_aspect_ { ETHEORA_ASPECT_NORMAL, 
ETHEORA_ASPECT_WIDE_SCREEN, ETHEORA_ASPECT_PRESERVE} etheora_aspect; 

/*
etheora_yuv_getelem(a, yuv, ch, i, j): 
macro for easy access of yuv_buffer elements. it makes
a = yuv.ch[i, j], without subsampling compensation, 
ie, yuv.y[i, j] generally does not belong to the same 
pixel as yuv.u[i, j]. 

ch can be CH_Y, CH_U, CH_V.

TODO: need to be tested with subsampled yuv_buffer. 
*/

#define etheora_yuv_getelem(a, yuv, ch, i, j)   \
	if (ch == CH_Y && yuv.y_stride < 0) \
		a = yuv.y[i + (j-1)*yuv.y_stride]; \
	else if (ch == CH_Y) \
		     a = yuv.y[i + j * yuv.y_stride]; \
	else if (ch == CH_U && yuv.uv_stride < 0) \
		a = yuv.u[i + (j-1)*yuv.uv_stride]; \
	else if (ch == CH_U) \
		     a = yuv.u[i + j * yuv.uv_stride]; \
	else if (ch == CH_V && yuv.uv_stride < 0) \
		a = yuv.v[i + (j-1)*yuv.uv_stride]; \
	else if (ch == CH_V) \
		     a = yuv.v[i + j * yuv.uv_stride]; \
	
/*
etheora_yuv_putelem(a, yuv, ch, i, j): 
macro for easy access of yuv_buffer elements. it makes
yuv.ch[i, j] = a, without subsampling compensation, 
ie, yuv.y[i, j] generally does not belong to the same 
pixel as yuv.u[i, j]. 

ch can be CH_Y, CH_U, CH_V.

TODO: need to be tested with subsampled yuv_buffer. 
*/

#define etheora_yuv_putelem(a, yuv, ch, i, j)   \
	if (ch == CH_Y && yuv.y_stride < 0) \
		yuv.y[i + (j-1)*yuv.y_stride] = a; \
	else if (ch == CH_Y) \
		     yuv.y[i + j * yuv.y_stride] = a; \
	else if (ch == CH_U && yuv.uv_stride < 0) \
		yuv.u[i + (j-1)*yuv.uv_stride] = a; \
	else if (ch == CH_U) \
		     yuv.u[i + j * yuv.uv_stride] = a; \
	else if (ch == CH_V && yuv.uv_stride < 0) \
		yuv.v[i + (j-1)*yuv.uv_stride] = a; \
	else if (ch == CH_V) \
		     yuv.v[i + j * yuv.uv_stride] = a; \
	

/*
etheora_interpolate(a, b, x, c, d, y):
find a y value in the range c..d equivalent to 
a x value in the range a..b

example: given a temperature X Celsius degree, 
ie, a..b = 0..100, find the temperature y Farenheit degree, 
ie, c..d = 32..180. 

etheora_interpolate(0, 100, X, 32, 180, y);

TODO: these functions may result inexact values. the caller
must ensure these values are in the desired range. 

*/

#define etheora_interpolate(a, b, x, c, d, y)  \
	y = (float)(x-a)*(d-c)/(b-a) + c;  \
	if(y < c) y = c; \
	if(y > d) y = d; 


/*
etheora_abs(a,b)
 	this macro makes a = |b|; 
*/

#define etheora_abs(a, b)  \
	a = b;             \
	if(a < 0) a = -a;  


/*TODO: unafortunately, ogg doesn't provide a function to test 
if a stream was initiated or not. so, we have to keep track of 
them. */

/*structure to keep track of a logical stream in the TODO(?). */
typedef struct etheora_ss{
	ogg_stream_state *streams; /*streams array.*/
	char *streams_states; /*carries info about the stream initialization.*/
	int count; /*number of streams.*/
}etheora_streams_states;  


struct etheora_context {
        theora_state ts;
        theora_info ti;
	etheora_streams_states ess; 
        theora_comment tc;
	ogg_sync_state os; 
        yuv_buffer yuv;
	/*TODO: is this int?*/
	ogg_int64_t current_frame; 
        /* finfo - a file descriptor to print out debug info.*/
        FILE *finfo; 
        /* fout - a file to print the ogg packets. 
           note it may be a socket stream.*/
        FILE *fogg; 
/* TODO: compute pixel callback function too!*/
}; 

typedef struct etheora_context etheora_ctx; 


/* official api: */


/* i. encoding functions. */

int  etheora_enc_setup(etheora_ctx *ec, 
		ogg_uint32_t frame_width, ogg_uint32_t frame_height, 
		etheora_aspect aspect, ogg_uint32_t  fps_numerator, 
		ogg_uint32_t  fps_denominator, FILE *fout, FILE *finfo); 

int  etheora_enc_start(etheora_ctx *ec); 

int  etheora_enc_yuv_draw(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        unsigned char y, unsigned char u, unsigned char v); 

int  etheora_enc_rgb_draw(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        float r, float g, float b); 

int  etheora_enc_nextframe(etheora_ctx *ec); 

int  etheora_enc_finish(etheora_ctx *ec); 


/* ii. decoding functions. */

int  etheora_dec_setup(etheora_ctx *ec, FILE *fin, FILE *finfo); 

int  etheora_dec_start(etheora_ctx *ec); 

int  etheora_dec_nextframe(etheora_ctx *ec); 

int  etheora_dec_yuv_read(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        unsigned char *y, unsigned char *u, unsigned char *v); 

int  etheora_dec_rgb_read(etheora_ctx *ec, ogg_uint32_t i, ogg_uint32_t j,
        float *r, float *g, float *b); 

int  etheora_dec_finish(etheora_ctx *ec); 

/* iii. attributes getting functions. */

ogg_uint32_t etheora_get_width(etheora_ctx *ec); 

ogg_uint32_t etheora_get_height(etheora_ctx *ec); 

ogg_uint32_t etheora_get_fps_numerator(etheora_ctx *ec); 

ogg_uint32_t etheora_get_fps_denominator(etheora_ctx *ec); 

ogg_uint32_t etheora_get_aspect_numerator(etheora_ctx *ec); 

ogg_uint32_t etheora_get_aspect_denominator(etheora_ctx *ec); 

/* iv. frame buffer related functions. */

theora_pixelformat etheora_yuvbuf_pixelformat(yuv_buffer *yuv); 

int  etheora_yuvbuf_resample(yuv_buffer *yuv_source, yuv_buffer *yuv_dest);

/* TODO: yet unavailable: 
int  etheora_yuvbuf_init(theora_info *ti, theora_pixelformat 
		pixel_format, yuv_buffer *yuv); 

void etheora_yuvbuf_free(yuv_buffer *yuv); 

*/

/* v. pixel related functions. */

int  etheora_pixel_yuv2rgb(const float y, const float u, const float v,
		float *r, float *g, float *b);
int  etheora_pixel_rgb2yuv(const float r, const float g, const float b,
		float *y, float *u, float *v);

int  etheora_pixel_YCbCr2yuv(const float Y, const float Cb, const float Cr,
		float *y, float *u, float *v);
int  etheora_pixel_yuv2YCbCr(const float y, const float u, const float v,
		float *Y, float *Cb, float *Cr);

int  etheora_pixel_yuv_reescale(float *y, float *u, float *v);
int  etheora_pixel_yuv_unreescale(float *y, float *u, float *v); 

int etheora_pixel_rgb_reescale(float *r, float *g, float *b); 
int etheora_pixel_rgb_unreescale(float *r, float *g, float *b); 


#ifdef ETHEORA_USE_INTERNAL
#include <etheora-int.h>
#endif

#endif /* __ETHEORA_LIB__ */

