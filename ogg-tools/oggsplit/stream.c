/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <stdio.h>
#include <sys/types.h>
#include "system.h"

#include "stream.h"

char *xmalloc();
char *xrealloc();
char *xstrdup();

const char *magic_vorbis = "\x01vorbis";
const char *magic_theora = "\x80theora";
const char *magic_speex  = "Speex";
const char *magic_flac   = "fLaC";

int stream_ctrl_init(stream_ctrl_t *sc)
{
  /* begin with 2 stream_t:s */
  sc->streams=(stream_t *)xmalloc(sizeof(stream_t)*2);

  sc->streams_size=2;
  sc->streams_used=0;
  return 1;
}

int stream_ctrl_free(stream_ctrl_t *sc)
{
  free(sc->streams);
  return 1;
}

stream_t *stream_ctrl_stream_new(stream_ctrl_t *sc, ogg_page *og)
{
  stream_t *stream;

  if(sc->streams_used==sc->streams_size){
    /* we need more streams, alloc 2 more */
    sc->streams=(stream_t *)xrealloc(sc->streams,sizeof(stream_t)*(sc->streams_size+2));
    sc->streams_size+=2;
  }

  stream=&sc->streams[sc->streams_used++];

  stream->serial=ogg_page_serialno(og);

  if(strncmp(og->body, magic_vorbis, strlen(magic_vorbis))==0)
    stream->type=STREAM_TYPE_VORBIS;
  else if(strncmp(og->body, magic_theora, strlen(magic_theora))==0)
    stream->type=STREAM_TYPE_THEORA;
  else if(strncmp(og->body, magic_speex, strlen(magic_speex))==0)
    stream->type=STREAM_TYPE_SPEEX;
  else if(strncmp(og->body, magic_flac, strlen(magic_flac))==0)
    stream->type=STREAM_TYPE_FLAC;
  else
    stream->type=STREAM_TYPE_UNKNOWN;

  stream->op=NULL;

  return stream;
}

stream_t *stream_ctrl_stream_get(stream_ctrl_t *sc, int serial)
{
  int i;
  for(i=0; i<sc->streams_used; i++)
    if(sc->streams[i].serial == serial){
      return &sc->streams[i];
    }

  return NULL;
}

int stream_ctrl_stream_free(stream_ctrl_t *sc, int serial)
{
  int i;
  for(i=0; i<sc->streams_used; i++){
    if(sc->streams[i].serial == serial){
      sc->streams_used--;

      /* If this isn't the last stream, move around some memory.
       * It doesn't matter if the address of a stream_t object is changed
       * by this, since a new reference to it is found for every page.
       */
      if(sc->streams_used > i)
	memmove(&sc->streams[i],
		&sc->streams[i+1],
		sizeof(stream_t)*(sc->streams_used-i));
      return 1;
    }
  }
  return 0;
}

const char* stream_type_name(stream_t *stream)
{
  switch(stream->type){
  case STREAM_TYPE_VORBIS:
    return "vorbis";
    break;
  case STREAM_TYPE_THEORA:
    return "theora";
    break;
  case STREAM_TYPE_SPEEX:
    return "speex";
    break;
  case STREAM_TYPE_FLAC:
    return "flac";
    break;
  case STREAM_TYPE_UNKNOWN:
  default:
    return "unknown";
    break;
  }
}
