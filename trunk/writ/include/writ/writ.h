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
 last mod: $Id: writ.h,v 1.5 2003/12/02 07:54:12 arc Exp $

 ********************************************************************/

#include <ogg2/ogg.h>

  
typedef struct writ_text {
  int    length;
  char  *string;
} writ_text;


typedef struct writ_langdef {
  writ_text language_name;
  writ_text language_desc;
} writ_langdef;


typedef struct writ_window {
  ogg_uint16_t  location_x;
  ogg_uint16_t  location_y;
  ogg_uint16_t  location_width;
  ogg_uint16_t  location_height;
  int	        alignment_x; 		/* 0=left, 1=right, 2=center, 3=full */
  int           alignment_y;		/* 0=top, 1=bottom, 2=middle, 3=full */
} writ_window;


typedef struct writ_wininfo {
  ogg_uint16_t  location_scale_x;
  ogg_uint16_t  location_scale_y;

  int           num_windows;
  writ_window  *windows;   
} writ_wininfo;


typedef struct writ_info {
  int            subver;
  int            subver_bytes;
  
  ogg_uint32_t   granulerate_numerator;
  ogg_uint32_t   granulerate_denominator;
  
  int            num_languages;
  writ_langdef  *languages;
  
  writ_wininfo  *window_info; /* Used with subversion 1+ only */
  /* Future subversion structs go here */
} writ_info;


typedef struct writ_phrase {
  ogg_int64_t    start;
  ogg_uint32_t   duration;
  char         **text;

  int            win_id;
  /* Future subversion fields go here */
} writ_phrase;  


typedef struct writ_state {
  writ_info   *wi;
  
  int          num_phrases;
  writ_phrase *phrase_buffer;
  
  ogg_int64_t granulepos;
} writ_state;


extern int ilog(unsigned int v);	/* src/format.c */
