/* playlist_text.c
 * - Module for simple playlist
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

char playlist_file[1024];
int lineno;

#include <definitions.h>
#include "rand.h"

/* Private function declarations */
static void playlist_builtin_shuffle_playlist ();
static int playlist_builtin_verify_playlist (ices_config_t *ices_config);
static int playlist_builtin_line_skip (int lineno, FILE *fp);

/* Global function definitions */

/* Every time this is called, it opens the
   playlist file, skips to the right line,
   reads the line, closes the file and
   returns the line. */
char *
ices_playlist_builtin_get_next ()
{
	FILE *fp = ices_util_fopen_for_reading (playlist_file);
	char *out;

	if (!fp) {
		ices_log_error ("Could not open playlist file!");
		return NULL;
	}

	if (!playlist_builtin_line_skip (lineno, fp)) {
		ices_log_debug ("Caught end of file on playlist, starting over");
		lineno = 0;
		ices_util_fclose (fp);
		return ices_playlist_builtin_get_next ();
	}
	
	out = ices_util_read_line (fp);
	
	if (out && out[0]) {
		out[strlen (out) - 1] = '\0';
	} else if (feof (fp)) {
		ices_log_debug ("Caught end of file on playlist, starting over");
		lineno = 0;
		ices_util_fclose (fp);
		return ices_playlist_builtin_get_next ();
	}
	
	ices_util_fclose (fp);
	
	lineno++;

	ices_log_debug ("Builtin playlist handler serving: %s", ices_util_nullcheck (out));

	return out;
}

int
ices_playlist_builtin_get_current_lineno ()
{
	return lineno;
}

int
ices_playlist_builtin_initialize (ices_config_t *ices_config)
{
	ices_log_debug ("Initializing builting playlist handler...");

	if (!playlist_builtin_verify_playlist (ices_config)) {
		ices_log ("Could not find a valid playlist, and I can't bloody well make up the music myself.");
		exit (0);
	}
	
	strncpy (playlist_file, ices_config->playlist_file, 1024);

	if (ices_config->randomize_playlist) {
		ices_log_debug ("Randomizing playlist...");
		playlist_builtin_shuffle_playlist ();
	}
	
	lineno = 0;
	return 1;
}

int
ices_playlist_builtin_shutdown (ices_config_t *ices_config)
{
	if (ices_config->randomize_playlist)
		return ices_util_remove (playlist_file);
	return 1;
}
	
/* Private function definitions */
static void
playlist_builtin_shuffle_playlist ()
{
	char *newname, namespace[1024];
	FILE *old, *new;

	old = ices_util_fopen_for_reading (playlist_file);
	newname = ices_util_get_random_filename (namespace, "playlist");
	new = ices_util_fopen_for_writing (newname);

	if (!old || !new) {
		ices_log ("ERROR: Error opening playlist [%s] or playlist random [%s] file", playlist_file, newname);
		return;
	}
		
	rand_file (old, new);

	ices_util_fclose (new);
	ices_util_fclose (old);

	ices_log_debug ("Randomized playlist [%s] is now in [%s]", playlist_file, newname);

	strncpy (playlist_file, newname, 1024);
}

static int
playlist_builtin_verify_playlist (ices_config_t *ices_config)
{
	FILE *new;

	if (!ices_config->playlist_file || !ices_config->playlist_file[0]) {
		ices_log ("ERROR: Playlist file is not set!");
		return 0;
	}
	
	new = ices_util_fopen_for_reading (ices_config->playlist_file);

	if (new) {
		ices_util_fclose (new);
		return 1;
	} else {
		ices_log ("ERROR: Could not open playlist file [%s].", ices_config->playlist_file);
		return 0;
	}
}

static int
playlist_builtin_line_skip (int lineno, FILE *fp)
{
	int i;
	char buffvoid[1024];
	
	for (i = lineno; i > 0;) {
		if (!fgets (buffvoid, 1024, fp))
			return 0;
		if (strchr (buffvoid, '\n')) 
			i--; 
	}

	if (feof (fp))
		return 0;

	return 1;
}
		
