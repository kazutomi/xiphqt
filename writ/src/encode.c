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
 last mod: $Id: encode.c,v 1.1 2003/12/09 06:38:44 arc Exp $

 ********************************************************************/

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
