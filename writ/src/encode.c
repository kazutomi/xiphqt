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
 last mod: $Id: encode.c,v 1.3 2003/12/09 20:40:50 arc Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <writ/writ.h>

int writ_encode_init(writ_state *ws, ogg_uint32_t granule_num, 
                     ogg_uint32_t granule_den) {
  ws = _ogg_malloc(sizeof(writ_state));
  ws->granulepos = -1;
  ws->num_phrases = 0;
  ws->skip_phrases = 0;

  ws->opb = _ogg_malloc(oggpack_buffersize());
  ws->opb_state = ogg_buffer_create();

  ws->wi = _ogg_malloc(sizeof(writ_info));
  ws->wi->subversion = 0;
  ws->wi->granulerate_numerator = granule_num;
  ws->wi->granulerate_denominator = granule_den;
   
  return OGG_SUCCESS;
}

int writ_encode_clear(writ_state *ws) {
  _ogg_free(ws->wi);
  _ogg_free(ws->opb);
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
    ws->wi->languages = malloc(sizeof(writ_language)*256);
  } else {
    if (ws->wi->num_languages == 255) return -8;
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
  ws->wi->windows = malloc(sizeof(writ_window)*256);

  return OGG_SUCCESS;
}

int writ_encode_wind_add(writ_state *ws, int left, int top, int width, 
                         int height, int align_x, int align_y) {
  if (ws->wi->num_windows == 255) return -8;
  
  ws->wi->windows[ws->wi->num_windows].location_x = left;
  ws->wi->windows[ws->wi->num_windows].location_y = top;
  ws->wi->windows[ws->wi->num_windows].location_width = width;
  ws->wi->windows[ws->wi->num_windows].location_height = height;
  ws->wi->windows[ws->wi->num_windows].alignment_x = align_x;
  ws->wi->windows[ws->wi->num_windows].alignment_y = align_y;
  ws->wi->num_windows++;

  return OGG_SUCCESS;
}


int writ_encode_packetout(writ_state *ws, ogg_packet **op) {
  int i;

  if ( ws->granulepos != -1 ) return -1;
  
  oggpack_writeinit(ws->opb, ws->opb_state);
  oggpack_write(ws->opb, 0, 8);
  oggpack_write(ws->opb, 1953067639, 32);
  oggpack_write(ws->opb, 0, 8);
  oggpack_write(ws->opb, ws->wi->subversion, 8);
  oggpack_write(ws->opb, ws->wi->granulerate_numerator, 32);
  oggpack_write(ws->opb, ws->wi->granulerate_denominator, 32);
  ws->packet_queue[0] = oggpack_writebuffer(ws->opb);
  
  if (ws->wi->subversion > 0) {
    writ_language *wl;

    oggpack_writeinit(ws->opb, ws->opb_state);
    oggpack_write(ws->opb, 1, 8);
    oggpack_write(ws->opb, 1953067639, 32);
    oggpack_write(ws->opb, ws->wi->num_languages, 8);
    /* One or more times */
    for (i=0; i<=ws->wi->num_languages; i++) {
      wl = ws->wi->languages + i;
      writ_text_write(ws->opb, wl.language_name);
      writ_text_write(ws->opb, wl.language_desc);
    }
    ws->packet_queue[1] = oggpack_writebuffer(ws->opb);

    if (ws->wi->subversion > 1) {
      int bitx = ilog(ws->wi->location_scale_x);
      int bity = ilog(ws->wi->location_scale_y);

      oggpack_writeinit(ws->opb, ws->opb_state);
      oggpack_write(ws->opb, 2, 8);
      oggpack_write(ws->opb, 1953067639, 32);
      oggpack_write(ws->opb, ws->wi->location_scale_x, 16);
      oggpack_write(ws->opb, ws->wi->location_scale_y, 16);
      oggpack_write(ws->opb, ws->wi->num_windows, 8);
      /* Zero or more times */
      for (i=0; i < ws->wi->num_windows; i++) {
        wn = ws->wi->windows + i;
        oggpack_write(ws->opb, wn.location_x, bitx);
        oggpack_write(ws->opb, wn.location_y, bity);
        oggpack_write(ws->opb, wn.location_width, bitx);
        oggpack_write(ws->opb, wn.location_height, bity);
        oggpack_write(ws->opb, wn.alignment_x, 2);
        oggpack_write(ws->opb, wn.alignment_y, 2);
      }        
      ws->packet_queue[2] = oggpack_writebuffer(ws->opb);
      return 2;
    }
    return 1;
  }
  return 0;
}      
