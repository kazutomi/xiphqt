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

 encode.c: Writ stream encoding
 last mod: $Id: encode.c,v 1.2 2003/12/09 07:11:30 arc Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <writ/writ.h>

int writ_encode_init(writ_state *ws, ogg_uint32_t granule_num, 
                     ogg_uint32_t granule_den) {
  ws = _ogg_malloc(sizeof(writ_state));
  ws->granulepos = 0;
  ws->num_phrases = 0;
  ws->skip_phrases = 0;

  ws->wi = _ogg_malloc(sizeof(writ_info));
  ws->wi->subversion = 0;
  ws->wi->granulerate_numerator = granule_num;
  ws->wi->granulerate_denominator = granule_den;
   
  return OGG_SUCCESS;
}

int writ_encode_clear(writ_state *ws) {
  _ogg_free(ws->wi);
  _ogg_free(ws);

  return OGG_SUCCESS;
}

int writ_encode_lang_add(writ_state *ws, char *name, char *desc) {
  writ_language *new_lang;
  int name_length = strlen(name);
  int desc_length = strlen(desc);

  if (ws->wi->subversion == 0) {
    ws->wi->subversion = 1;
    ws->wi->num_languages = 0;
    ws->wi->languages = malloc(sizeof(writ_language)*16);
  } else {
    if (ws->wi->num_languages == 15) 
      ws->wi->languages = realloc(ws->wi->languages, 
                                  sizeof(writ_language)*256);
    if (ws->wi->num_languages == 255) return -1;
    ws->wi->num_languages++;
  }
  new_lang = ws->wi->languages + ws->wi->num_languages;
  new_lang->language_name.length = name_length;
  new_lang->language_name.string = malloc(name_length+desc_length);
  memcpy(new_lang->language_name.string, name, name_length); 
  new_lang->language_desc.length = desc_length;
  new_lang->language_desc.string = new_lang->language_name.string +
                                   name_length;
  memcpy(new_lang->language_desc.string, desc, desc_length); 

  return ws->wi->num_languages;
}

int writ_encode_wind_init(writ_state *ws, int scale_x, int scale_y) {
  if (ws->wi->subversion == 0) return -1; /* Lang must come first */
  if (ws->wi->subversion >= 2) return -1; /* Windows already init */

  ws->wi->location_scale_x = scale_x;
  ws->wi->location_scale_y = scale_y;
  ws->wi->num_windows = 0;
  ws->wi->windows = malloc(sizeof(writ_window)*16);

  return OGG_SUCCESS;
}

int writ_encode_wind_add(writ_state *ws, int left, int top, int width, 
                         int height, int align_x, int align_y) {
  if (ws->wi->num_windows == 15) 
      ws->wi->windows = realloc(ws->wi->windows, 
                                sizeof(writ_window)*256);
  if (ws->wi->num_windows == 255) return -1;
  
  ws->wi->windows[ws->wi->num_windows].location_x = left;
  ws->wi->windows[ws->wi->num_windows].location_y = top;
  ws->wi->windows[ws->wi->num_windows].location_width = width;
  ws->wi->windows[ws->wi->num_windows].location_height = height;
  ws->wi->windows[ws->wi->num_windows].alignment_x = align_x;
  ws->wi->windows[ws->wi->num_windows].alignment_y = align_y;
  ws->wi->num_windows++;

  return OGG_SUCCESS;
}
