/* cue.c
 * - Functions for cue file in ices
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

char *ices_cue_filename = NULL;

/* Syntax of cue file
 * filename
 * size
 * bitrate
 * minutes:seconds total
 * % played
 * current line in playlist
 * artist
 * songname
 */

/* Global function definitions */
void
ices_cue_update (const char *filename, const int filesize, const int bitrate, const int bytes_played) 
{
	char buf[1024];

	FILE *fp = ices_util_fopen_for_writing (ices_cue_get_filename());
	
	char *id3artist = ices_id3_get_artist ();
	char *id3title = ices_id3_get_title ();

	if (!fp) {
		ices_log ("Could not open cuefile [%s] for writing, cuefile not updated!", ices_cue_get_filename ());
		return;
	}

	fprintf (fp, "%s\n%d\n%d\n%s\n%f\n%d\n%s\n%s\n", filename, filesize, bitrate, ices_util_file_time (bitrate, filesize, buf),
		 ices_util_percent (bytes_played, filesize), ices_playlist_get_current_lineno (), ices_util_nullcheck (id3artist), ices_util_nullcheck (id3title));
	
	ices_util_fclose (fp);
}

void
ices_cue_set_filename (const char *filename)
{
	ices_cue_filename = ices_util_strdup (filename);
}

const char *
ices_cue_get_filename ()
{
	static char filespace[1024];

	if (ices_cue_filename)
		return ices_cue_filename;

	return ices_util_get_random_filename (filespace, "cue");
}


			
