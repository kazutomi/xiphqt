/* playlist_text.c
 * - Module for simple playlist
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

#include <definitions.h>
#include "rand.h"

static FILE* fp = NULL;
int lineno;

extern ices_config_t ices_config;

/* Private function declarations */
static char* playlist_builtin_get_next (void);
static int playlist_builtin_get_lineno (void);
static void playlist_builtin_shutdown (void);

static void playlist_builtin_shuffle_playlist (void);
static int playlist_builtin_verify_playlist (playlist_module_t* pm);

/* Global function definitions */

/* Initialize the builting playlist handler */
int
ices_playlist_builtin_initialize (playlist_module_t* pm)
{
  ices_log_debug ("Initializing builting playlist handler...");

  pm->get_next = playlist_builtin_get_next;
  pm->get_metadata = NULL;
  pm->get_lineno = playlist_builtin_get_lineno;
  pm->shutdown = playlist_builtin_shutdown;

  if (!playlist_builtin_verify_playlist (pm)) {
    ices_log ("Could not find a valid playlist file.");
    ices_setup_shutdown ();
    return -1;
  }

  if (pm->randomize) {
    ices_log_debug ("Randomizing playlist...");
    playlist_builtin_shuffle_playlist ();
  }

  lineno = 0;
  return 1;
}

static char *
playlist_builtin_get_next (void)
{
  char *out;
  static int level = 0;

  if (feof (fp)) {
    ices_log_debug ("Caught end of file on playlist, starting over");
    lineno = 0;
    rewind (fp);
  }

  if (! (out = ices_util_read_line (fp)))
    return NULL;

  if (out[0])
    out[strlen (out) - 1] = '\0';

  if (! out[0]) {
    if (level++) {
      level = 0;
      return NULL;
    }
    return playlist_builtin_get_next ();
  }

  level = 0;

  lineno++;

  ices_log_debug ("Builtin playlist handler serving: %s", ices_util_nullcheck (out));

  return out;
}

/* Return the current playlist file line number */
static int
playlist_builtin_get_lineno (void)
{
  return lineno;
}

/* Shutdown the builtin playlist handler */
static void
playlist_builtin_shutdown (void)
{
  if (fp)
    ices_util_fclose (fp);
}
	
/* Private function definitions */

/* Shuffle the playlist by creating a box-unique "internal" playlist
 * and using that as the playlist */
static void
playlist_builtin_shuffle_playlist (void)
{
  char *newname, namespace[1024], buf[1024];
  FILE* new;

  if (! ices_config.base_directory) {
    ices_log_error ("Base directory is invalid");
    return;
  }

  newname = ices_util_get_random_filename (buf, "playlist");
  snprintf (namespace, sizeof (namespace), "%s/%s",
	    ices_config.base_directory, buf);
  new = fopen (namespace, "w+");
  if (!new) {
    ices_log ("Error writing randomized playlist file: %s", namespace);
    return;
  }
  unlink (namespace);
  
  rand_file (fp, new);
  ices_util_fclose (fp);

  fp = new;
  rewind (fp);
}

/* Verify that the user specified playlist actually exists */
static int
playlist_builtin_verify_playlist (playlist_module_t* pm)
{
  if (!pm->playlist_file || !pm->playlist_file[0]) {
    ices_log ("ERROR: Playlist file is not set!");
    return 0;
  }

  fp = ices_util_fopen_for_reading (pm->playlist_file);

  if (fp) {
    return 1;
  } else {
    ices_log ("ERROR: Could not open playlist file: %s", pm->playlist_file);
    return 0;
  }
}
