/* stream.c
 * - Functions for streaming in ices
 * Copyright (c) 2000 Alexander Haväng
 * Copyright (c) 2001 Brendan Cully
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

#include <resolver.h>

#define INPUT_BUFSIZ 4096

extern ices_config_t ices_config;

static volatile int finish_send = 0;

/* Private function declarations */
static int ices_stream_send_file (const char *file);
static int ices_stream_open_source (input_stream_t* source);

/* Public function definitions */

/* Top level streaming function, called once from main() to
 * connect to server and start streaming */
void
ices_stream_loop (void)
{
  char namespace[1024];
  int consecutive_errors = 0;
  ices_stream_config_t* stream;

  /* Check if user gave us a hostname */
  for (stream = ices_config.streams; stream; stream = stream->next) {
    if (!isdigit ((int)(stream->conn.ip[strlen (stream->conn.ip) - 1]))) {
      char hostbuff[1024];

      /* Ok, he did, let's get the ip */
      stream->conn.ip = resolver_getip (stream->conn.ip, hostbuff, 1024);
      if (stream->conn.ip == NULL) {
	ices_log ("Failed resolving servername.");
	ices_setup_shutdown ();
      }
    }
  }

  /* Tell libshout to connect to the server */
  for (stream = ices_config.streams; stream; stream = stream->next) {
    if (!shout_connect (&stream->conn)) {
      ices_log ("Failed connecting to server %s, error: %s", stream->conn.ip, shout_strerror(&stream->conn, stream->conn.error, namespace, 1024));
      ices_setup_shutdown ();
    }
  }
	
  ices_log ("Connected to server %s...", ices_config.streams->conn.ip);

  /* Foreeeever streams, I want to be forever streaming... */
  while (1) {
    char *file;

    /* NULL definition as of now, it will handle scripted
     * DJ files in the future. Or perhaps there is no need
     * for this now that all playlist handling is done in
     * scripts. */
    ices_dj_pre ();

    /* Get the next file to stream from the playlist module.
     * This is a dynamically allocated string, which we free
     * further down in the loop */
    file = ices_playlist_get_next ();
		
    /* We quit if the playlist handler gives us a NULL filename */
    if (!file) {
      ices_log ("Warning: ices_file_get_next() gave me an error, this is not good. [%s]", ices_log_get_error ());
      ices_setup_shutdown ();
    }
		
    /* If something goes on while transfering, we just go on */
    if (ices_stream_send_file (file) < 0) {
      ices_log ("Warning: Encountered error while transfering %s. [%s]", file, ices_log_get_error ());

      consecutive_errors++;

      /* Not the best interface - we have to search for the error */
      /* Let's see if it was a libshout socket error */
      for (stream = ices_config.streams; stream; stream = stream->next) {
	if (stream->conn.error == SHOUTERR_SOCKET) {
	  ices_log ("Libshout communication problem, trying to reconnect to server");
				
	  /* Make sure we disconnect */
	  shout_disconnect (&stream->conn);

	  /* Reconnect */
	  if (!shout_connect (&stream->conn)) {
	    ices_log ("Failed reconnecting to server %s, error: %i", stream->conn.ip, stream->conn.error);
	    ices_setup_shutdown ();
	  }
	}
      }
			
      /* This stops ices from entering a loop with 10-20 lines of output per
	 second. Usually caused by a playlist handler that produces only
	 invalid file names. */
      if (consecutive_errors > 10) {
	ices_log ("Exiting after 10 consecutive errors.");
	ices_util_free (file);
	ices_setup_shutdown ();
      }

      ices_util_free (file);
      continue;
    } else {
      /* Reset the consecutive error counter */
      consecutive_errors = 0;
    }
		
    /* Run the post DJ program/script
     * Currently does nada */
    ices_dj_post ();
		
    /* Free the dynamically allocated filename */
    ices_util_free (file);
  }
  /* Not reached */
}

/* This function is called to stream a single file */
static int
ices_stream_send_file (const char *file)
{
  input_stream_t source;
  ices_stream_config_t* stream;
  unsigned char ibuf[INPUT_BUFSIZ];
  unsigned char buf[INPUT_BUFSIZ * 4];
  char namespace[1024];
  ssize_t len;
  ssize_t olen;
  size_t bytes_read;
  int samples;
  int rc;
#ifdef HAVE_LIBLAME
  static int16_t left[INPUT_BUFSIZ * 30];
  static int16_t right[INPUT_BUFSIZ * 30];
#endif

  source.path = file;
  bytes_read = 0;

  if (ices_stream_open_source (&source) < 0) {
    ices_log_error ("Error sending %s", source.path);
    return 0;
  }

  if (! source.read) {
    if (! source.readpcm) {
      ices_log_error ("No read method implemented for %s", source.path);
      source.close (&source);
      return -1;
    }
    for (stream = ices_config.streams; stream; stream = stream->next) {
      if (! (stream->reencode)) {
	ices_log_error ("Cannot play without reencoding");
	source.close (&source);
	return -1;
      }
    }
  }

  ices_metadata_set (&source);

#ifdef HAVE_LIBLAME
  ices_reencode_reset ();
#endif
  
  ices_log ("Now playing %s", source.path);

  finish_send = 0;
  while (! finish_send) {
    len = samples = 0;
    if (source.read) {
      len = source.read (&source, ibuf, sizeof (ibuf));
      bytes_read += len;
#ifdef HAVE_LIBLAME
      if (ices_config.reencode)
	samples = ices_reencode_decode (ibuf, len, sizeof (left), left, right);
    } else if (source.readpcm) {
      len = samples = source.readpcm (&source, sizeof (left), left, right);
#endif
    }

    if (len > 0) {
      olen = 0;
      rc = 1;
      for (stream = ices_config.streams; stream; stream = stream->next) {
	if (! stream->reencode) {
	  rc = shout_send_data (&stream->conn, ibuf, len);
	  shout_sleep (&stream->conn);
	}
#ifdef HAVE_LIBLAME
	else {
	  olen = ices_reencode_reencode_chunk (stream, samples, left, right,
					       buf, sizeof (buf));
	  if (olen == -1) {
	    ices_log_debug ("Output buffer too small, skipping chunk");
	  } else if (olen > 0) {
	    rc = shout_send_data (&stream->conn, buf, olen);
	    shout_sleep (&stream->conn);
	  }
	}
#endif

	if (!rc) {
	  ices_log_error ("Libshout reported send error: %s", shout_strerror (&stream->conn, stream->conn.error, namespace, 1024));
	  source.close (&source);
	  return -1;
	}
      }
    } else if (len == 0) {
      finish_send = 1;
    } else {
      ices_log_error ("Read error: %s",
		      ices_util_strerror (errno, namespace, 1024));
      source.close (&source);
      return -1;
    }

    ices_cue_update (&source, bytes_read);
  }

#ifdef HAVE_LIBLAME
  for (stream = ices_config.streams; stream; stream = stream->next)
    if (stream->reencode) {
      len = ices_reencode_flush (stream, buf, sizeof (buf));
      if (len > 0)
	rc = shout_send_data (&stream->conn, buf, len);
    }
#endif

  source.close (&source);

  return 0;
}

/* open up path, figure out what kind of input it is, and set up source */
static int
ices_stream_open_source (input_stream_t* source)
{
  char buf[1024];
  size_t len;
  int fd;
  int rc;

  if ((fd = open (source->path, O_RDONLY)) < 0) {
    ices_util_strerror (errno, buf, sizeof (buf));
    ices_log_error ("Error opening %s: %s", source->path, buf);

    return -1;
  }

  source->fd = fd;

  if (lseek (fd, SEEK_SET, 0) == -1)
    source->canseek = 0;
  else {
    source->canseek = 1;
    source->filesize = ices_util_fd_size (fd);
  }

  if ((len = read (fd, buf, sizeof (buf))) <= 0) {
    ices_util_strerror (errno, buf, sizeof (buf));
    ices_log_error ("Error reading header from %s: %s", source->path, buf);
    
    return -1;
  }

  if (!(rc = ices_mp3_open (source, buf, len)))
    return 0;
  if (rc < 0)
    goto err;

#ifdef HAVE_LIBVORBISFILE
  if (!(rc = ices_vorbis_open (source, buf, len)))
    return 0;
  if (rc < 0)
    goto err;
#endif

err:
  close (fd);
  return -1;
}

/* set a flag to tell ices_stream_send_file to finish early */
void
ices_stream_next (void)
{
  finish_send = 1;
}
