/* stream.c
 * - Functions for streaming in ices
 * Copyright (c) 2000 Alexander Haväng
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

#ifdef HAVE_LIBVORBISFILE
#include "in_vorbis.h"
#endif

#include <resolver.h>

#define INPUT_BUFSIZ 4096

/* -- local types -- */

extern ices_config_t ices_config;

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
    if (!ices_stream_send_file (file)) {
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
  unsigned char buf[INPUT_BUFSIZ * 4];
  char namespace[1024];
  ssize_t len;
  size_t bytes_read;
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

#ifdef HAVE_LIBLAME
  ices_reencode_reset ();
#endif
  
  ices_log ("Streaming from %s until EOF..", source.path);
	
  while (1) {
#ifdef HAVE_LIBLAME
    if (ices_config.reencode || source.type != ICES_INPUT_MP3) {
      len = source.readpcm (&source, sizeof (left), left, right);
    } else
#endif
      len = source.read (&source, buf, sizeof (buf));

    bytes_read += len;

    if (len > 0) {

      for (stream = ices_config.streams; stream; stream = stream->next) {
#ifdef HAVE_LIBLAME
	if (ices_config.reencode || source.type != ICES_INPUT_MP3)
	  len = ices_reencode_reencode_chunk (stream, len, left, right, buf,
					      sizeof (buf));
#endif
	if (len) {
	  rc = shout_send_data (&stream->conn, buf, len);
			
	  if (!rc) {
	    ices_log_error ("Libshout reported send error: %s", shout_strerror (&stream->conn, stream->conn.error, namespace, 1024));
	    break;
	  }
	}
      }
			
    } else if (len == 0) {
      source.close (&source);
      return 1;
    } else {
      ices_log_error ("Read error while reading %s: %s", source.path,
		      ices_util_strerror (errno, namespace, 1024));
      source.close (&source);
      return 0;
    }

    ices_cue_update (&source, bytes_read);
    for (stream = ices_config.streams; stream; stream = stream->next)
      shout_sleep(&stream->conn);
  }

#ifdef HAVE_LIBLAME
  for (stream = ices_config.streams; stream; stream = stream->next) {
    len = ices_reencode_flush (stream, buf, sizeof (buf));
    if (len > 0)
      rc = shout_send_data (&stream->conn, buf, len);
  }
#endif

  source.close (&source);

  return 1;
}

/* open up path, figure out what kind of input it is, and set up source */
static int
ices_stream_open_source (input_stream_t* source)
{
  ices_mp3_in_t* mp3_data;
  char buf[1024];
  int fd;
  int rc;

  if ((fd = open (source->path, O_RDONLY)) < 0) {
    ices_util_strerror (errno, buf, sizeof (buf));
    ices_log_error ("Error opening %s: %s", source->path, buf);

    return -1;
  }

  if (lseek (fd, SEEK_SET, 0) == -1)
    source->canseek = 0;
  else {
    source->canseek = 1;
  }
  ices_log_error ("Seek: %s", source->canseek ? "yes" : "no");

#ifdef HAVE_LIBVORBISFILE
  if (source->canseek) {
    if ((rc = ices_vorbis_open (source)) == 0) {
      close (fd);
      return 0;
    } else if (rc < 0) {
      close (fd);
      return -1;
    }
  }
#endif

  /* let's hope it's MP3 */
  if (source->canseek) {
    ices_mp3_parse_file (source->path);
    ices_id3_parse_file (source->path, 0);
    source->bitrate = ices_mp3_get_bitrate ();
    source->filesize = ices_util_fd_size (fd);
  }

  mp3_data = (ices_mp3_in_t*) malloc (sizeof (ices_mp3_in_t));
  mp3_data->fd = fd;

  source->type = ICES_INPUT_MP3;
  source->data = mp3_data;

  source->read = ices_mp3_read;
#ifdef HAVE_LIBLAME
  source->readpcm = ices_mp3_readpcm;
#endif
  source->close = ices_mp3_close;

  return 0;
}
