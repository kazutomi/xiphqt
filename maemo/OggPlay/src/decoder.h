/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef DECODER_H
#define DECODER_H

#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>
#include <tremor/ogg.h>
#include <glib.h>
#include <pthread.h>
#include "ringbuffer.h"
#include "stream.h"
#include "audio.h"


enum decodercommand { DECODER_NONE, DECODER_SET_AUDIO };

struct _Decoder {

  OggVorbis_File vf;
  Audio *audio;

  enum decodercommand cmd;

  long millisecs;
  long total;
  int channels;
  int rate;
  gboolean is_playing;

  /* Ogg comment tags */
  char *tag_title;
  char *tag_artist;
  char *tag_album;

};
typedef struct _Decoder Decoder;



Decoder *decoder_new();
void decoder_free(Decoder *dec);
gboolean decoder_open_stream(Decoder *dec, Stream *stream);
void decoder_close_stream(Decoder *dec);
void decoder_decode(Decoder *dec, const char *data, int size);
long decoder_get_total(Decoder *dec);
long decoder_get_position(Decoder *dec);
void decoder_set_volume(Decoder *dec, int volume);
void decoder_seek(Decoder *dec, long millisecs);
#endif
