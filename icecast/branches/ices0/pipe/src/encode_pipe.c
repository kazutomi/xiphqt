/* encode_pipe.c
* Encode PCM to output format via pipe
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
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "definitions.h"
#include "pipe.h"

#include <sys/select.h>
#include <unistd.h>

extern ices_config_t ices_config;

static int pipe_init(ices_stream_t* stream);
static int pipe_new_source(ices_stream_t* stream, input_stream_t* source);
static int pipe_encode(ices_stream_t* stream, int samples, int16_t* left,
                       int16_t* right, unsigned char* out, size_t olen);
static void pipe_close(ices_stream_t* stream);
static void pipe_shutdown(ices_stream_t* stream);

static ices_encoder_t PipeEncoder = {
  pipe_init,
  pipe_new_source,
  pipe_encode,
  pipe_shutdown
};

typedef struct {
  int pid;
  int rfd;
  int wfd;
  int16_t* buf;
  size_t blen;
} encoder_state_t;

ices_encoder_t *pipe_encoder(void) {
  return &PipeEncoder;
};

static int pipe_init(ices_stream_t* stream) {
  encoder_state_t* encoder;

  encoder = (encoder_state_t*)calloc(1, sizeof(encoder_state_t));
  if (!encoder) {
    ices_log_error("Could not allocate encoder state");
    return 0;
  }
  stream->enc2_state = encoder;

  /* TODO: extract sample rate, bit rate, channels, other opts here */
  ices_log_debug("Encoder pipe initialised: %d kbps", stream->bitrate);
  
  return 1;
}

static int pipe_new_source(ices_stream_t* stream, input_stream_t* source) {
  encoder_state_t* encoder;
  const char* cmd = "lame -b64 - -";

  encoder = (encoder_state_t*)stream->enc2_state;

  pipe_close(stream);
  if (encoder->blen) {
    free (encoder->buf);
    encoder->blen = 0;
  }

  if ((encoder->pid = ices_pipe(cmd, &encoder->rfd, &encoder->wfd)) < 0) {
    return -1;
  }

  return 1;
}

static int pipe_encode(ices_stream_t* stream, int samples, int16_t* left,
                       int16_t* right, unsigned char* out, size_t olen)
{
  encoder_state_t* encoder;
  int rc;
  int i = 0;
  ssize_t bytesread = 0;
  fd_set rfds;
  fd_set wfds;

  encoder = (encoder_state_t*)stream->enc2_state;

  if (encoder->blen < samples * 4) {
    if (encoder->blen)
      free (encoder->buf);
    encoder->blen = samples * 4;
    encoder->buf = malloc(encoder->blen);
  }
  for (i = 0; i < samples; i++) {
    encoder->buf[i*2] = left[i];
    encoder->buf[i*2+1] = right[i];
  }

  i = 0;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  while (i < samples*4) {
    FD_SET(encoder->rfd, &rfds);
    FD_SET(encoder->wfd, &wfds);
    select(FD_SETSIZE, &rfds, &wfds, NULL, NULL);
    if (FD_ISSET(encoder->wfd, &wfds)) {
      rc = write(encoder->wfd, ((char*)encoder->buf)+i, samples*4 - i);
      if (rc < 0) {
        ices_log_debug("error writing to pipe: %s", strerror(errno));
        pipe_close(stream);
        return -2;
      }

      i += rc;
    }

    if (FD_ISSET(encoder->rfd, &rfds)) {
      rc = read(encoder->rfd, out + bytesread, olen - bytesread);
      if (rc > 0) {
        bytesread += rc;
      }
      if (bytesread == olen)
        break;
    }
  }

  return bytesread;
}

static void pipe_close(ices_stream_t* stream) {
  encoder_state_t* encoder;
  
  encoder = (encoder_state_t*)stream->enc2_state;
  
  if (encoder->rfd) {
    close(encoder->rfd);
    close(encoder->wfd);
    encoder->rfd = 0;
    encoder->wfd = 0;
  }
}

static void pipe_shutdown(ices_stream_t* stream) {
  encoder_state_t* encoder;
  
  encoder = (encoder_state_t*)stream->enc2_state;

  ices_log_debug("Encoder pipe shutting down");

  pipe_close(stream);
  
  if (encoder->blen)
    free(encoder->buf);
  
  free(encoder);
  stream->enc2_state = NULL;
}