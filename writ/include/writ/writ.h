/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggWrit SOFTWARE CODEC SOURCE CODE.     *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggWrit SOURCE CODE IS (C) COPYRIGHT 2003                    *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: toplevel libwrit include
 last mod: $Id: writ.h,v 1.8 2003/12/09 20:40:49 arc Exp $

 ********************************************************************/

#include <ogg2/ogg.h>


/* These structs should all be abstracted for internal use only */
  
typedef struct writ_text {
  int    length;
  char  *string;
} writ_text;


typedef struct writ_language {
  writ_text language_name;
  writ_text language_desc;
} writ_language;


typedef struct writ_window {
  ogg_uint16_t  location_x;
  ogg_uint16_t  location_y;
  ogg_uint16_t  location_width;
  ogg_uint16_t  location_height;
  int	        alignment_x; 	 /* 0=left, 1=right, 2=center, 3=full */
  int           alignment_y;	 /* 0=top, 1=bottom, 2=middle, 3=full */
} writ_window;


typedef struct writ_info {
  int            subversion;
  
  ogg_uint32_t   granulerate_numerator;
  ogg_uint32_t   granulerate_denominator;
  
  /* Subversion 1+ */
  int            num_languages;
  writ_language *languages;
  
  /* Subversion 2+ */
  ogg_uint16_t  location_scale_x;
  ogg_uint16_t  location_scale_y;
  int           num_windows;
  writ_window  *windows;   
} writ_info;


typedef struct writ_phrase {
  ogg_int64_t    start;
  ogg_uint32_t   duration;
  writ_text    **text;
  int            window_id;
} writ_phrase;  


typedef struct writ_state {
  writ_info        *wi;
  
  oggpack_buffer   *opb;
  ogg_buffer_state *opb_state;

  ogg_int64_t       granulepos;
  ogg_packet       *packet_queue[4];

  int               num_phrases;
  int               skip_phrases;
  writ_phrase      *phrase_buffer;
  
} writ_state;


/* OggWrit Encoding Methods ******************************************/

extern int writ_encode_init(writ_state *ws, ogg_uint32_t granule_num,
                            ogg_uint32_t granule_den);
extern int writ_encode_clear(writ_state *ws);

extern int writ_encode_lang_add(writ_state *ws, char *name, char *desc);
extern int writ_encode_wind_init(writ_state *ws, 
                                 int scale_x, int scale_y);
extern int writ_encode_wind_add(writ_state *ws, int left, int top, 
                                int width, int height, 
                                int align_x, int align_y);
extern int writ_encode_packetout(writ_state *ws, ogg_packet **op);

/* A different call for each subversion, the best way? */
extern int writ_encode_phrase0(writ_state *ws, ogg_packet *ogg_packet,
                               ogg_int64_t start, ogg_uint32_t duration, 
                               char *text);
extern int writ_encode_phrase1(writ_state *ws, ogg_packet *ogg_packet,
                               ogg_int64_t start, ogg_uint32_t duration, 
                               char **text);
extern int writ_encode_phrase2(writ_state *ws, ogg_packet *ogg_packet,
                               ogg_int64_t start, ogg_uint32_t duration, 
                               char **text, int window_id);


extern int ilog(unsigned int v);	/* src/format.c */

