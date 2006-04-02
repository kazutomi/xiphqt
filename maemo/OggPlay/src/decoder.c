/*
 *    Ogg vorbis decoder using the Tremor library
 *    Copyright (c) 2005, 2006 Martin Grimme  <martin.grimme@lintegra.de>
 *
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


#include "decoder.h"


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cmdmutex = PTHREAD_MUTEX_INITIALIZER;


static void *
decoder_thread(Decoder *dec) {

  char pcmout[128];
  long ret = 1;
  int current_section;
  gboolean running = TRUE;

  while (running) {
    /* process commands */
    pthread_mutex_lock(&mutex);
    switch (dec->cmd) {
    case (DECODER_SET_AUDIO):
      audio_close_device(dec->audio);
      audio_open_device(dec->audio, dec->channels, dec->rate);
      break;
    default:
      ;
    }
    dec->cmd = DECODER_NONE;
    pthread_mutex_unlock(&cmdmutex);

    if (dec->is_playing) {
      ret = ov_read(&(dec->vf), pcmout, sizeof(pcmout), &current_section);
      dec->has_finished = FALSE;
      if (ret == 0) {
	fprintf(stderr, "End of stream.\n");
	dec->is_playing = FALSE;
	dec->has_finished = TRUE;
	audio_play(dec->audio, pcmout, 0, 0);
      } else if (ret < 0) {
	//fprintf(stderr, "Error in stream %d.\n", current_section);
      } else {
	dec->millisecs = (long) ov_time_tell(&(dec->vf));
	audio_play(dec->audio, pcmout, (int) ret, dec->millisecs);
      }
    }

    pthread_mutex_unlock(&mutex);

    if (! dec->is_playing) usleep(1000);
  }

}


/* read callback function */
static size_t
read_func(void *buffer,
	  size_t size,
	  size_t nmemb,
	  void *stream) {

  size_t bytes_read;

  bytes_read = stream_read((Stream *) stream, (char *) buffer, size * nmemb);
  return bytes_read;

}


/* seek callback function */
static int
seek_func(void *stream, ogg_int64_t offset, int whence) {

  stream_seek((Stream *) stream, (long) offset, whence);
  return 0;

}


/* tell callback function */
static long
tell_func(void *stream) {

  return stream_tell((Stream *) stream);

}


/* close callback function */
static int
close_func(void *stream) {

  stream_free((Stream *) stream);
  return 1;

}


/* Parses Vorbis tag comments */
static void
parse_comments(Decoder *dec, vorbis_comment *comment) {

  int i = 0;
  char **parts;

  for (i = 0; i < comment->comments; i++) {
    printf("%s\n", comment->user_comments[i]);
    parts = g_strsplit(comment->user_comments[i], "=", 2);
    if (g_strcasecmp(parts[0], "TITLE") == 0)
      dec->tag_title = g_strdup(parts[1]);
    else if (g_strcasecmp(parts[0], "ARTIST") == 0)
      dec->tag_artist = g_strdup(parts[1]);
    else if (g_strcasecmp(parts[0], "ALBUM") == 0)
      dec->tag_album = g_strdup(parts[1]);

    g_strfreev(parts);
  }

}



Decoder *
decoder_new() {

  pthread_t th;
  Decoder *dec = g_new0(Decoder, 1);
  dec->vf.datasource = NULL;
  dec->audio = audio_new();
  dec->is_playing = FALSE;
  dec->has_finished = FALSE;
  
  /* spawn decoder thread */
  pthread_create(&th, NULL, decoder_thread, (void *) dec);

  return dec;

}



gboolean
decoder_open_stream(Decoder *dec, Stream *stream) {

  ov_callbacks ov_cb;
  vorbis_info *vi;

  ov_cb.read_func = &read_func;
  ov_cb.seek_func = &seek_func;
  ov_cb.tell_func = &tell_func;
  ov_cb.close_func = &close_func;

  pthread_mutex_lock(&mutex);
  if (ov_open_callbacks((void *) stream, &(dec->vf), NULL, 0, ov_cb) < 0) {
    /* close stream if it's not an OGG Vorbis or does not exist */
    stream_free(stream);
    pthread_mutex_unlock(&mutex);
    fprintf(stderr, "Could not open stream.\n");
    return FALSE;
  }

  pthread_mutex_lock(&cmdmutex);

  vi = ov_info(&(dec->vf), -1);
  dec->channels = vi->channels;
  dec->rate = vi->rate;
  dec->cmd = DECODER_SET_AUDIO;

  parse_comments(dec, ov_comment(&(dec->vf), -1));
  dec->total = (long) ov_time_total(&(dec->vf), -1);

  pthread_mutex_unlock(&mutex);
  decoder_seek(dec, 0);

  return TRUE;

}


void
decoder_close_stream(Decoder *dec) {

  pthread_mutex_lock(&mutex);
  dec->is_playing = FALSE;
  g_free(dec->tag_title);
  g_free(dec->tag_artist);
  g_free(dec->tag_album);

  if (dec->vf.datasource) {
    ov_clear(&(dec->vf));
    dec->vf.datasource = NULL;
  }
  pthread_mutex_unlock(&mutex);

}


long
decoder_get_total(Decoder *dec) {

  return dec->total / 1000;

}


long
decoder_get_position(Decoder *dec) {

  return audio_get_timetag(dec->audio) / 1000;

}


void
decoder_set_volume(Decoder *dec,
		   int volume) {

  audio_set_volume(dec->audio, volume);

}


gboolean
decoder_is_playing(Decoder *dec) {

  return dec->is_playing;

}


gboolean
decoder_has_finished(Decoder *dec) {

  return dec->has_finished;

}


void
decoder_play(Decoder *dec) {

  dec->is_playing = TRUE;

}


void
decoder_stop(Decoder *dec) {

  dec->is_playing = FALSE;

}


void
decoder_seek(Decoder *dec,
	     long millisecs) {

  int success;

  pthread_mutex_lock(&mutex);
  success = ov_time_seek_page(&(dec->vf), (ogg_int64_t) millisecs);
  audio_flush(dec->audio);
  pthread_mutex_unlock(&mutex);  

}
