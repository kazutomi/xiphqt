/* id3.c
 * - Functions for id3 tags in ices
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

#include <thread.h>

char *ices_id3_filename = NULL;
char *ices_id3_song = NULL;
char *ices_id3_artist = NULL;
char *ices_id3_genre = NULL;
int ices_id3_file_size = -1;

extern ices_config_t ices_config;

/* Private function declarations */
static void ices_id3_update_metadata (const char *filename, int file_bytes);
void *ices_id3_update_thread (void *arg);
static int ices_id3_parse (const char *filename, int file_bytes);
static void ices_id3_cleanup ();
static char *ices_id3_filename_cleanup (const char *oldname, char *namespace, int maxsize);

/* Global function definitions */
int
ices_id3_parse_file (const char *filename, int file_bytes)
{
	ices_id3_cleanup ();
	
	if (ices_id3_parse (filename, file_bytes))
		file_bytes -= 128;

	ices_id3_update_metadata (filename, file_bytes);

	return file_bytes;
}

char *
ices_id3_get_artist ()
{
	return ices_id3_artist;
}

char *
ices_id3_get_title ()
{
	return ices_id3_song;
}

char *
ices_id3_get_genre ()
{
	return ices_id3_genre;
}

char *
ices_id3_get_filename ()
{
	return ices_id3_filename;
}

/* Private function definitions */

static void
ices_id3_update_metadata (const char *filename, int file_bytes)
{
	thread_create ("Metadata Update Thread", ices_id3_update_thread, NULL);
}

void *
ices_id3_update_thread (void *arg)
{
	int ret;
	char metastring[1024], song[2048];
	char *id3_artist = ices_id3_get_artist ();
	char *id3_song = ices_id3_get_title ();
	const char *filename = ices_util_nullcheck (ices_id3_get_filename ());

	if (id3_artist) {
		sprintf (song, "%s - %s", id3_artist, id3_song ? id3_song : filename);
	} else {
		sprintf (song, "%s", (id3_song != NULL) ? id3_song : ices_id3_filename_cleanup (filename, metastring, 1024));
	}
	
	if (ices_config.header_protocol == icy_header_protocol_e)
		sprintf (metastring, "%s", song);
	else
		sprintf (metastring, "%s", song); /* This should have length as well but libshout doesn't handle it correctly */
	ret = ices_update_metadata (ices_util_get_conn (), metastring);

	if (ret != 1)
		ices_log ("Updating metadata on server failed.");

	thread_exit (0);
	return 0;
}

static int
ices_id3_parse (const char *filename, int file_bytes)
{
	FILE *temp;
	char tag[3];
	char song_name[31];
	char artist[31];
	char genre[31];
	char namespace[1024];

	if (!(temp = ices_util_fopen_for_reading (filename))) {
		ices_log ("Error while opening file %s for id3 tag parsing. Error: %s", filename, ices_util_strerror (errno, namespace, 1024));
		return 0;
	}
	
	if (fseek (temp, -128, SEEK_END) == -1) {
		ices_log ("Error while seeking in file %s for id3 tag parsing. Error: %s", filename, ices_util_strerror (errno, namespace, 1024));
		ices_util_fclose (temp);
		return 0;
	}

	memset (song_name, 0, 31);
	memset (artist, 0, 31);
	memset (genre, 0, 31);

	if (fread (tag, sizeof (char), 3, temp) == 3) {
		if (strncmp (tag, "TAG", 3) == 0) {
			if (fseek (temp, -125, SEEK_END) == -1) {
				ices_log ("Error while seeking in file %s for id3 tag parsing. Error: %s", filename, ices_util_strerror (errno, namespace, 1024));
				ices_util_fclose (temp);
				return 0;
			}

			if (fread (song_name, sizeof (char), 30, temp) == 30) {
				while (song_name[strlen (song_name) - 1] == '\040')
					song_name[strlen (song_name) - 1] = '\0';
				ices_id3_song = ices_util_strdup (song_name);
				ices_log_debug ("ID3: Found song=[%s]", ices_id3_song);
			}

			if (fread (artist, sizeof (char), 30, temp) == 30) {
				while (artist[strlen (artist) - 1] == '\040')
					artist[strlen (artist) - 1] = '\0';
				ices_id3_artist = ices_util_strdup (artist);
				ices_log_debug ("ID3: Found artist=[%s]", ices_id3_artist);
			}

			if (fread (genre, sizeof (char), 30, temp) == 30) {
				while (genre[strlen (genre) - 1] == '\040')
					genre[strlen (genre) - 1] = '\0';
				ices_id3_genre = ices_util_strdup (genre);
				ices_log_debug ("ID3: Found genre=[%s]", ices_id3_genre);
			}
		}
	}

	ices_id3_file_size = file_bytes;
	ices_id3_filename = ices_util_strdup (filename);

	ices_util_fclose (temp);

	return 1;
}

static void
ices_id3_cleanup ()
{
	if (ices_id3_song)
		ices_util_free (ices_id3_song);
	if (ices_id3_artist)
		ices_util_free (ices_id3_artist);
	if (ices_id3_genre)
		ices_util_free (ices_id3_genre);
	if (ices_id3_filename)
		ices_util_free (ices_id3_filename);
}

static char *
ices_id3_filename_cleanup (const char *oldname, char *namespace, int maxsize)
{
	char *ptr;

	if ((ptr = strrchr (oldname, '/')) && (strlen (ptr) > 0)) {
		strncpy (namespace, ptr + 1, maxsize);
	} else {
		strncpy (namespace, oldname, maxsize);
	}
		    
	if ((ptr = strrchr (namespace, '.'))) {
		*ptr = '\0';
	}

	return namespace;
}







