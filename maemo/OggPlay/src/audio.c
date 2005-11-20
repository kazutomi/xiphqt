/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License. The original
 *  author of the programm (Tino H. Seifert) will decide if later versions
 *  are also acceptable.
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


#include "audio.h"


/* Audio callback for SDL */
static void
audio_cb(void *userdata,
	 Uint8 *stream,
	 int size) {

  Audio *audio = (Audio *) userdata;
  Uint8 *buffer;
  char *tmp;
  int filled = 0;

  buffer = g_new0(Uint8, size);
  tmp = g_new0(char, 8);

  while (filled < size) {
    /* don't hang here if audio buffer became empty */
    if (! ringbuffer_is_empty(audio->buffer)) {  

      /* read time tag */
      ringbuffer_get(audio->buffer, tmp);
      audio->timetag = ((long *) tmp)[0];

      /* read audio data */
      filled += ringbuffer_get(audio->buffer, buffer + filled);

    } else {
      break;
    }
  }
  
  /* mix audio stream with volume settings */
  if (filled == size)
    SDL_MixAudio(stream, buffer, size, audio->volume);

  g_free(buffer);
  g_free(tmp);

}


Audio *
audio_new() {

  Audio *audio = g_new0(Audio, 1);
  audio->buffer = ringbuffer_new(1000, 128);
  audio->opened = FALSE;
  audio->volume = 64;

  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "Could not initialize audio: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);

  return audio;

}


void
audio_free(Audio *audio) {

  ringbuffer_free(audio->buffer);
  g_free(audio);

}


void
audio_open_device(Audio *audio,
		  int channels,
		  int rate) {

  SDL_AudioSpec format;

  audio->channels = channels;
  audio->rate = rate;

  format.freq = rate;
  format.format = AUDIO_S16;
  format.channels = channels;
  format.samples = 1024;
  format.callback = audio_cb;
  format.userdata = (void *) audio;

  /* clear buffer for loading a new stream */
  audio_flush(audio);

  /* open audio device */
  if (SDL_OpenAudio(&format, NULL) < 0) {
    fprintf(stderr, "Could not open audio device: %s\n", SDL_GetError());
  } else {
    audio->opened = TRUE;
    SDL_PauseAudio(0);
  }

}


void
audio_close_device(Audio *audio) {

  if (audio->opened) {
    SDL_CloseAudio();
    audio->opened = FALSE;
  }

}


/* Schedules the given stream for playback. */
void
audio_play(Audio *audio,
	   const char *data,
	   int size,
	   long timetag) {

  char *tmp;

  /* quite a hack for transmitting the time tag, but it works well */
  tmp = g_new0(char, 8);
  ((long *) tmp)[0] = timetag;

  /* transmit the time tag and the audio data */
  ringbuffer_put(audio->buffer, tmp, 8);
  ringbuffer_put(audio->buffer, data, size);

  g_free(tmp);

}


/* flushs the audio buffer instantly */
void
audio_flush(Audio *audio) {

  ringbuffer_clear(audio->buffer);

}


void
audio_set_volume(Audio *audio,
		 int volume) {

  audio->volume = SDL_MIX_MAXVOLUME * (volume / 100.0);

}


long
audio_get_timetag(Audio *audio) {

  return audio->timetag;

}
