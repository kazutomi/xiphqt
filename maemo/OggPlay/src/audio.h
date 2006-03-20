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


#ifndef AUDIO_H
#define AUDIO_H

#include <SDL/SDL.h>
#include <glib.h>
#include <stdio.h>
#include "ringbuffer.h"


struct _Audio {

  RingBuffer *buffer;

  gboolean opened;
  gboolean nodevice;  /* useful for running in scratchbox */
  int volume;
  int channels;
  int rate;
  long timetag;

};
typedef struct _Audio Audio;



Audio* audio_new();
void audio_free(Audio *audio);
void audio_open_device(Audio *audio, int channels, int rate);
void audio_close_device(Audio *audio);
void audio_play(Audio *audio, const char *data, int size, long timetag);
void audio_flush(Audio *audio);
void audio_set_volume(Audio *audio, int volume);
long audio_get_timetag(Audio *audio);

#endif
