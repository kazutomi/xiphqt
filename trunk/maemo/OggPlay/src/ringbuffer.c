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


#include "ringbuffer.h"


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * Creates and returns a new ring buffer object.
 */
RingBuffer *
ringbuffer_new(int n,
	       int capacity) {

  RingBuffer *rb;
  int i;

  rb = g_new(RingBuffer, 1);
  rb->n = n;

  rb->buffer = g_new0(char*, n);
  for (i = 0; i < n; i++)
    rb->buffer[i] = g_new0(char, capacity);

  rb->lengths = g_new0(int, n);

  rb->consumer_idx = 0;
  rb->producer_idx = 0;

  return rb;

}


void
ringbuffer_free(RingBuffer *rb) {

  int i;

  for (i = 0; i < rb->n; i++)
    g_free(rb->buffer[i]);
  g_free(rb->buffer);
  g_free(rb->lengths);
  g_free(rb);

}


void
ringbuffer_put(RingBuffer *rb,
	       char *data,
	       int size) {

  /* the producer must not write over parts which the consumer has not
     read yet */
  while ((rb->producer_idx + 1) % rb->n == rb->consumer_idx) {
    usleep(100);
  }

  pthread_mutex_lock(&mutex);
  memcpy(rb->buffer[rb->producer_idx], data, size);
  rb->lengths[rb->producer_idx] = size; 
  rb->producer_idx = (rb->producer_idx + 1) % rb->n;
  pthread_mutex_unlock(&mutex);

}


int
ringbuffer_get(RingBuffer *rb,
	       char *data) {

  int size;

  /* the consumer must not overtake the producer */
  while (rb->consumer_idx == rb->producer_idx)
    usleep(100);

  pthread_mutex_lock(&mutex);
  size = rb->lengths[rb->consumer_idx];
  memcpy(data, rb->buffer[rb->consumer_idx], size);
  rb->consumer_idx = (rb->consumer_idx + 1) % rb->n;
  pthread_mutex_unlock(&mutex);

  return size;

}


void
ringbuffer_clear(RingBuffer *rb) {

  //pthread_mutex_lock(&mutex);
  rb->consumer_idx = rb->producer_idx;
  //pthread_mutex_unlock(&mutex);

}


gboolean
ringbuffer_is_full(RingBuffer *rb) {

  return (rb->producer_idx + 2) % rb->n == rb->consumer_idx;

}


gboolean
ringbuffer_is_empty(RingBuffer *rb) {

  return (rb->consumer_idx == rb->producer_idx);

}
