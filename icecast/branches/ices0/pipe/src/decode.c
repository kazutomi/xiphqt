/* decode.c
 * Decode source audio to WAV via pipe
 * Copyright (c) 2005 Brendan Cully
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

typedef struct {
  int pid;
  int rfd;
  int wfd;
} decoder_t;

/* Open a pipe to cmd for decoding. Returns decoder handle, or NULL on
 * failure. */
void* decode_init(const char* cmd) {
  decoder_t* decoder;

  decoder = (decoder_t*)calloc(1, sizeof(decoder_t));
  if (!decoder) {
    ices_log_error("Could not allocate decoder");
    return NULL;
  }
  
  if ((decoder->pid = ices_pipe(cmd, &rfd, &wfd)) < 0) {
    return NULL;
  }

  return decoder;
}

int decode_shutdown(void* decoder) {
  free(decoder);
}