/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: format.c,v 1.1.2.1 2001/12/08 23:59:25 volsung Exp $

 ********************************************************************/

#include "transport.h"
#include "format.h"
#include "string.h"

extern format_t oggvorbis_format;

format_t *formats[] = { &oggvorbis_format, NULL };


format_t *get_format_by_name (char *name)
{
  int i = 0;

  while (formats[i] != NULL && strcmp(name, formats[i]->name) != 0)
    i++;

  return formats[i];
}


format_t *select_format (data_source_t *source)
{
  int i = 0;

  while (formats[i] != NULL && !formats[i]->can_decode(source))
    i++;

  return formats[i];
}


int audio_format_equal (audio_format_t *a, audio_format_t *b)
{
  return 
    a->big_endian    == b->big_endian    &&
    a->word_size     == b->word_size     &&
    a->signed_sample == b->signed_sample &&
    a->rate          == b->rate          &&
    a->channels      == b->channels;
}


void audio_format_copy (audio_format_t *dest, audio_format_t *source)
{
  dest->big_endian    = source->big_endian;
  dest->word_size     = source->word_size;
  dest->signed_sample = source->signed_sample;
  dest->rate          = source->rate;  
  dest->channels      = source->channels;
}  
