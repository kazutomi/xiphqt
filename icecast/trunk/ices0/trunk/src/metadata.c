/* metadata.c
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

#include <thread.h>

extern ices_config_t ices_config;

static int delay = 3000000;

/* Private function declarations */
static void *ices_metadata_thread (void *arg);
static char *ices_metadata_clean_filename (const char* path, char *buf,
					   size_t len);
/* Global function definitions */

/* Spawn a new thread to update metadata on server. Should be
 * very low overhead */
void
ices_metadata_set (input_stream_t* source)
{
  static int first = 1;

  if (first) {
    ices_log_debug ("Initially delaying metadata update...");
    delay = 3000000;
    first = 0;
  } else {
    delay = 1000;
  }

  if (thread_create ("Metadata Update Thread", ices_metadata_thread, source) == -1) {
    ices_log ("Error: Could not create metadata update thread!");
  }
}

/* Function used by the updating thread to update metadata on server.
 * It also does the job of cleaning up the song title to something the
 * world likes.
 * Note that the very first metadata update is delayed, because if we
 * try to update our new info to the server and the server has not yet
 * accepted us as a source, the information is lost */
static void *
ices_metadata_thread (void *arg)
{
  ices_stream_config_t* stream;
  input_stream_t* source = (input_stream_t*) arg;
  char song[1024];
  char* playlist_metadata;
  char* metadata;
  int rc;

  if (delay)
    thread_sleep (delay);

  if (! (playlist_metadata = ices_playlist_get_metadata ())) {
    if (source->get_metadata (source, song, sizeof (song)) < 0) {
      ices_metadata_clean_filename (source->path, song, sizeof (song));
    }
    metadata = song;
  } else
    metadata = playlist_metadata;
  
  if (ices_config.header_protocol == icy_header_protocol_e)
    ;
  else
    /* This should have length as well but libshout doesn't handle it */
    ;

  for (stream = ices_config.streams; stream; stream = stream->next) {
    rc = shout_update_metadata (&stream->conn, metadata);
	
    if (rc != 1)
      ices_log_error ("Updating metadata on %s failed.", stream->mount);
    else
      ices_log_debug ("Updated metadata on %s to: %s", stream->mount, metadata);
  }

  ices_util_free (playlist_metadata);

  thread_exit (0);
  return 0;
}

/* Cleanup a filename so it looks more like a song name */
static char *
ices_metadata_clean_filename (const char* path, char *buf, size_t len)
{
  char *ptr =NULL;

  if (!path || !buf) {
    ices_log ("ERROR: Polluted strings sent to filename cleanup.");
    return NULL;
  }

  /* Find last slash */
  ptr = strrchr (path, '/');

  if (ptr && strlen (ptr) > 0) {
    strncpy (buf, ptr + 1, len);
  } else {
    strncpy (buf, path, len);
  }

  if ((ptr = strrchr (buf, '.'))) {
    *ptr = '\0';
  }

  ices_log_debug ("Filename cleaned up from [%s] to [%s]", path, buf);
  return buf;
}
