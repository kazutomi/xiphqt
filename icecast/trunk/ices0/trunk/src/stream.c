/* stream.c
 * - Functions for streaming in ices
 * Copyright (c) 2000 Alexander Haväng
 * Copyright (c) 2001-3 Brendan Cully
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

#include "definitions.h"
#include "metadata.h"

#ifdef HAVE_LIBVORBISFILE
#include "in_vorbis.h"
#endif

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#define INPUT_BUFSIZ 4096
#define OUTPUT_BUFSIZ 32768
/* sleep this long in ms when every stream has errors */
#define ERROR_DELAY 999

typedef struct {
  char* data;
  size_t len;
} buffer_t;

static volatile int finish_send = 0;

/* Private function declarations */
static int stream_connect (ices_stream_t* stream);
static int stream_send (ices_config_t* config, input_stream_t* source);
static int stream_send_data (ices_stream_t* stream, unsigned char* buf,
			     size_t len);
static int stream_open_source (input_stream_t* source);
static int stream_needs_reencoding (input_stream_t* source, ices_stream_t* stream);

/* Public function definitions */

/* Top level streaming function, called once from main() to
 * connect to server and start streaming */
void
ices_stream_loop (ices_config_t* config)
{
  int consecutive_errors = 0;
  int linenum_old = 0;
  int linenum_new = 0;
  input_stream_t source;
  ices_stream_t* stream;
  int rc;

  for (stream = config->streams; stream; stream = stream->next)
    stream_connect (stream);

  while (1) {
    ices_metadata_set (NULL, NULL);

    source.path = ices_playlist_get_next ();

    /* keep track of the line numbers */
    linenum_old = linenum_new;
    linenum_new = ices_playlist_get_current_lineno ();
    ices_cue_set_lineno (linenum_new);

    /* we quit if we're told not to loop and the the new line num is lower than the old */
    if ( !config->pm.loop_playlist && ( linenum_new < linenum_old ) ) {
      ices_log ("Info: next playlist line number less than previous and looping disabled: quitting.");
      ices_setup_shutdown ();
    }

    /* We quit if the playlist handler gives us a NULL filename */
    if (!source.path) {
      ices_log ("Warning: ices_file_get_next() returned an error: %s", ices_log_get_error ());
      ices_setup_shutdown ();
    }

    if (source.path[0] == '\0') {
      ices_log ("Playlist file name is empty, shutting down.");
      ices_setup_shutdown ();
    }

    /* This stops ices from entering a loop with 10-20 lines of output per
	 second. Usually caused by a playlist handler that produces only
	 invalid file names. */
    if (consecutive_errors > 10) {
      ices_log ("Exiting after 10 consecutive errors.");
      ices_util_free (source.path);
      ices_setup_shutdown ();
    }

    if (stream_open_source (&source) < 0) {
      ices_log ("Error opening %s: %s", source.path, ices_log_get_error ());
      ices_util_free (source.path);
      consecutive_errors++;
      continue;
    }

    if (!source.read)
      for (stream = config->streams; stream; stream = stream->next)
        if (!stream->reencode) {
          ices_log ("Cannot play %s without reencoding", source.path);
          source.close (&source);
          ices_util_free (source.path);
          consecutive_errors++;
          continue;
	    }

    rc = stream_send (config, &source);
    source.close (&source);

    /* If something goes on while transfering, we just go on */
    if (rc < 0) {
      ices_log ("Encountered error while transfering %s: %s", source.path, ices_log_get_error ());

      consecutive_errors++;

      ices_util_free (source.path);
      continue;
    } else {
      /* Reset the consecutive error counter */
      consecutive_errors = 0;
    }
		
    ices_util_free (source.path);
  }
}

/* set a flag to tell stream_send to finish early */
void
ices_stream_next (void)
{
  finish_send = 1;
}

/* This function is called to stream a single file */
static int
stream_send (ices_config_t* config, input_stream_t* source)
{
  ices_stream_t* stream;
  unsigned char ibuf[INPUT_BUFSIZ];
  char namespace[1024];
  ssize_t len;
  ssize_t olen;
  int samples;
  int rc;
  int do_sleep;
#ifdef HAVE_LIBLAME
  int decode = 0;
  buffer_t obuf;
  /* worst case decode: 22050 Hz at 8kbs = 44.1 samples/byte */
  static int16_t left[INPUT_BUFSIZ * 45];
  static int16_t right[INPUT_BUFSIZ * 45];
#endif

#ifdef HAVE_LIBLAME
  obuf.data = NULL;
  obuf.len = 0;

  if (config->reencode) {
    ices_reencode_reset (source);
    for (stream = config->streams; stream; stream = stream->next)
      if (stream->reencode && stream_needs_reencoding (source, stream)) {
        decode = 1;
        break;
      }
  }

  if (decode) {
    obuf.len = OUTPUT_BUFSIZ;
    if (!(obuf.data = malloc(OUTPUT_BUFSIZ))) {
      ices_log_error("Error allocating encode buffer");
      return -1;
    }
  }
#endif

  for (stream = config->streams; stream; stream = stream->next)
    stream->errs = 0;
  
  ices_log ("Playing %s", source->path);

  ices_metadata_update (source);

  finish_send = 0;
  while (! finish_send) {
    len = samples = 0;
    /* fetch input buffer */
    if (source->read) {
      len = source->read (source, ibuf, sizeof (ibuf));
#ifdef HAVE_LIBLAME
      if (decode)
        samples = ices_reencode_decode (ibuf, len, sizeof (left), left, right);
    } else if (source->readpcm) {
      len = samples = source->readpcm (source, sizeof (left), left, right);
#endif
    }

    if (len == 0) {
      ices_log_debug ("Done sending");
      break;
    }
    if (len < 0) {
      ices_log_error ("Read error: %s", ices_util_strerror (errno, namespace, 1024));
      goto err;
    }
    
    do_sleep = 1;
    while (do_sleep) {
      rc = olen = 0;
      for (stream = config->streams; stream; stream = stream->next) {
        /* don't reencode if the source is MP3 and the same bitrate */
#ifdef HAVE_LIBLAME
        if (stream->reencode && stream_needs_reencoding (source, stream)) {
          if (samples > 0) {
            if (obuf.len < 7200 + samples + samples / 4) {
              char *tmpbuf;

              /* pessimistic estimate from lame.h */
              obuf.len = 7200 + 5 * samples / 2;
              if (!(tmpbuf = realloc(obuf.data, obuf.len))) {
	            ices_log_error ("Error growing output buffer, aborting track");
                goto err;
	          }
	          obuf.data = tmpbuf;
	          ices_log_debug ("Grew output buffer to %d bytes", obuf.len);
	        }
	        if ((olen = ices_reencode (stream, samples, left, right, obuf.data,
	            obuf.len)) < -1) {
	          ices_log_error ("Reencoding error, aborting track");
	          goto err;
	        } else if (olen == -1) {
	          char *tmpbuf;

	          if ((tmpbuf = realloc(obuf.data, obuf.len + OUTPUT_BUFSIZ))) {
                  obuf.data = tmpbuf;
	            obuf.len += OUTPUT_BUFSIZ;
	            ices_log_debug ("Grew output buffer to %d bytes", obuf.len);
	          } else
	            ices_log_debug ("%d byte output buffer is too small", obuf.len);
            } else if (olen > 0) {
              rc = stream_send_data (stream, obuf.data, olen);
            }
          }
        } else
#endif
          rc = stream_send_data (stream, ibuf, len);

        if (rc < 0) {
          if (stream->errs > 10) {
            ices_log ("Too many stream errors, giving up");
            goto err;
          }
          ices_log ("Error during send: %s", ices_log_get_error ());
        } else {
	      do_sleep = 0;
        }
        /* this is so if we have errors on every stream we pause before
         * attempting to reconnect */
        if (do_sleep) {
          struct timeval delay;
          delay.tv_sec = ERROR_DELAY / 1000;
          delay.tv_usec = ERROR_DELAY % 1000 * 1000;

          select (1, NULL, NULL, NULL, &delay);
        }
      }
    }
    ices_cue_update (source);
  }

#ifdef HAVE_LIBLAME
  for (stream = config->streams; stream; stream = stream->next)
    if (stream->reencode && stream_needs_reencoding (source, stream)) {
      len = ices_reencode_flush (stream, obuf.data, obuf.len);
      if (len > 0)
        rc = shout_send (stream->conn, obuf.data, len);
    }

  if (obuf.data)
    free(obuf.data);
#endif

  return 0;

err:
#ifdef HAVE_LIBLAME
  if (obuf.data)
    free(obuf.data);
#endif
  return -1;
}

/* open up path, figure out what kind of input it is, and set up source */
static int
stream_open_source (input_stream_t* source)
{
  char buf[INPUT_BUFSIZ];
  size_t len;
  int fd;
  int rc;

  source->filesize = 0;
  source->bytes_read = 0;

  if ((fd = open (source->path, O_RDONLY)) < 0) {
    ices_util_strerror (errno, buf, sizeof (buf));
    ices_log_error ("Error opening: %s", buf);

    return -1;
  }

  source->fd = fd;

  if ((rc = lseek (fd, 0, SEEK_END)) >= 0) {
    source->filesize = rc;
    lseek (fd, 0, SEEK_SET);
  }

  if ((len = read (fd, buf, sizeof (buf))) <= 0) {
    ices_util_strerror (errno, buf, sizeof (buf));
    ices_log_error ("Error reading header: %s", source->path, buf);

    close (fd);
    return -1;
  }

  if (!(rc = ices_mp3_open (source, buf, len)))
    return 0;
  if (rc < 0) {
    close (fd);
    return -1;
  }

#ifdef HAVE_LIBVORBISFILE
  if (!(rc = ices_vorbis_open (source, buf, len)))
    return 0;
#endif

  close (fd);
  return -1;
}

/* wrapper for shout_send_data, shout_sleep with error handling */
static int
stream_send_data (ices_stream_t* stream, unsigned char* buf, size_t len)
{
  int rc = -1;

  if (shout_get_connected (stream->conn) != SHOUTERR_CONNECTED)
    rc = stream_connect (stream);

  if (shout_get_connected (stream->conn) == SHOUTERR_CONNECTED) {
    shout_sync(stream->conn);
    if (shout_send (stream->conn, buf, len) == SHOUTERR_SUCCESS) {
      stream->errs = 0;
      rc = 0;
    } else {
      ices_log_error ("Libshout reported send error, disconnecting: %s",
		      shout_get_error (stream->conn));
      shout_close (stream->conn);
      stream->errs++;
      rc = -1;
    }
  }

  return rc;
}

static int
stream_connect (ices_stream_t* stream)
{
  time_t now = time(NULL);
  const char* mount = shout_get_mount (stream->conn);

  if (stream->connect_delay > now)
    return -1;

  if (shout_open (stream->conn) != SHOUTERR_SUCCESS) {
    ices_log_error ("Mount failed on http://%s:%d%s%s, error: %s",
		    shout_get_host (stream->conn), shout_get_port (stream->conn),
		    (mount && mount[0] == '/') ? "" : "/", ices_util_nullcheck (mount),
		    shout_get_error (stream->conn));
    stream->connect_delay = now + 1;
    stream->errs++;

    return -1;
  }

  ices_log ("Mounted on http://%s:%d%s%s", shout_get_host (stream->conn),
            shout_get_port (stream->conn),
            (mount && mount[0] == '/') ? "" : "/", ices_util_nullcheck (mount));

  return 0;
}

static int stream_needs_reencoding (input_stream_t* source, ices_stream_t* stream) {
    if (!source->read || source->bitrate != stream->bitrate
        || (stream->out_samplerate > 0 && source->samplerate != stream->out_samplerate)
        || (stream->out_numchannels > 0 && source->channels != stream->out_numchannels))
      return 1;

  return 0;
}
