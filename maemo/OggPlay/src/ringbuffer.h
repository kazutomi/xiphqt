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


#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <pthread.h>
#include <unistd.h>
#include <glib.h>


struct _RingBuffer {

  int n;

  char **buffer;
  int *lengths;

  int consumer_idx;
  int producer_idx;

};

typedef struct _RingBuffer RingBuffer;


RingBuffer *ringbuffer_new(int n, int capacity);
void ringbuffer_free(RingBuffer *rb);
void ringbuffer_put(RingBuffer *rb, char *data, int size);
int ringbuffer_get(RingBuffer *rb, char *data);
void ringbuffer_clear(RingBuffer *rb);
gboolean ringbuffer_is_full(RingBuffer *rb);
gboolean ringbuffer_is_empty(RingBuffer *rb);

#endif
