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

extern ices_config_t ices_config;

static char *ices_id3_song = NULL;
static char *ices_id3_artist = NULL;
static char *ices_id3_genre = NULL;
static int ices_id3_file_size = -1;
static mutex_t id3_mutex;
static int id3_is_initialized = 0;

/* Private function declarations */
static int ices_id3_parse (const char *filename, int file_bytes);
static void ices_id3_cleanup (void);

/* Global function definitions */

/* Initialize the id3 module, create the id3 module mutex */
void
ices_id3_initialize (void)
{
	thread_create_mutex (&id3_mutex);
	id3_is_initialized = 1;
}

/* Only shutdown if we are initialized */
void
ices_id3_shutdown (void)
{
	if (id3_is_initialized == 1)
		thread_destroy_mutex (&id3_mutex);
}

/* Do id3 tag parsing on the file, and update the metadata on
 * the server with the new information */
int
ices_id3_parse_file (const char *filename, int file_bytes)
{
	/* Make sure no one gets old/corrupt information */
	thread_lock_mutex (&id3_mutex);

	/* Cleanup the previous information */
	ices_id3_cleanup ();
	
	/* Do the actual id3 parsing. If the file has an id3 tag,
         * tell the stream module to stream 128 bytes less. */
	if (ices_id3_parse (filename, file_bytes))
		file_bytes -= 128;

	/* Give the go-ahead to external modules to get id3 info */
	thread_unlock_mutex (&id3_mutex);

	return file_bytes;
}

/* Return the id3 module artist name, if found. */
char *
ices_id3_get_artist (char *namespace, int maxlen)
{
	thread_lock_mutex (&id3_mutex);

	if (ices_id3_artist) {
		strncpy (namespace, ices_util_nullcheck (ices_id3_artist), maxlen);
	} else {
		namespace[0] = '\0';
		namespace = NULL;
	}

	thread_unlock_mutex (&id3_mutex);
	
	return namespace;
}

/* Return the id3 module title name, if found. */
char *
ices_id3_get_title (char *namespace, int maxlen)
{
	thread_lock_mutex (&id3_mutex);

	if (ices_id3_song) {
		strncpy (namespace, ices_util_nullcheck (ices_id3_song), maxlen);
	} else {
		namespace[0] = '\0';
		namespace = NULL;
	}

	thread_unlock_mutex (&id3_mutex);
	return namespace;
}

/* Return the id3 module genre name, if found. */
char *
ices_id3_get_genre (char *namespace, int maxlen)
{
	thread_lock_mutex (&id3_mutex);

	if (ices_id3_genre) {
		strncpy (namespace, ices_util_nullcheck (ices_id3_genre), maxlen);
	} else {
		namespace[0] = '\0';
		namespace = NULL;
	}

	thread_unlock_mutex (&id3_mutex);

	return namespace;
}

/* Private function definitions */

/* Function that does the id3 tag parsing of a file */
static int
ices_id3_parse (const char *filename, int file_bytes)
{
	FILE *temp;
	char tag[3];
	char song_name[31];
	char artist[31];
	char genre[31];
	char namespace[1024];

	ices_id3_file_size = file_bytes;

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

	ices_util_fclose (temp);

	return 1;
}

/* Make a clean slate for the next file */
static void
ices_id3_cleanup (void)
{
	if (ices_id3_song) {
		ices_util_free (ices_id3_song);
		ices_id3_song = NULL;
	}

	if (ices_id3_artist) {
		ices_util_free (ices_id3_artist);
		ices_id3_artist = NULL;
	}

	if (ices_id3_genre) {
		ices_util_free (ices_id3_genre);
		ices_id3_genre = NULL;
	}
}

