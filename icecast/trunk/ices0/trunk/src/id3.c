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

static char *ices_id3_filename = NULL;
static char *ices_id3_song = NULL;
static char *ices_id3_artist = NULL;
static char *ices_id3_genre = NULL;
static int ices_id3_file_size = -1;
static mutex_t id3_mutex;
static int id3_is_initialized = 0;
extern ices_config_t ices_config;

/* Private function declarations */
static void ices_id3_update_metadata (const char *filename, int file_bytes);
void *ices_id3_update_thread (void *arg);
static int ices_id3_parse (const char *filename, int file_bytes);
static void ices_id3_cleanup ();
static char *ices_id3_filename_cleanup (const char *oldname, char *namespace, int maxsize);

/* Global function definitions */

/* Initialize the id3 module, create the id3 module mutex */
void
ices_id3_initialize ()
{
	thread_create_mutex (&id3_mutex);
	id3_is_initialized = 1;
}

/* Only shutdown if we are initialized */
void
ices_id3_shutdown ()
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
	thread_mutex_lock (&id3_mutex);

	/* Cleanup the previous information */
	ices_id3_cleanup ();
	
	/* Do the actual id3 parsing. If the file has an id3 tag,
         * tell the stream module to stream 128 bytes less. */
	if (ices_id3_parse (filename, file_bytes))
		file_bytes -= 128;

	/* Give the go-ahead to external modules to get id3 info */
	thread_mutex_unlock (&id3_mutex);

	/* Update metadata on server, in a spawned new thread */
	ices_id3_update_metadata (filename, file_bytes);
	return file_bytes;
}

/* Return the id3 module artist name, if found.
 * String return is static memory */
char *
ices_id3_get_artist ()
{
	static char artist[41];
	char *out = NULL;

	thread_mutex_lock (&id3_mutex);

	if (ices_id3_artist) {
		memset (artist, 0, 40);
		strncpy (artist, ices_util_nullcheck (ices_id3_artist), 40);
		out = &artist[0];
	}

	thread_mutex_unlock (&id3_mutex);
	
	return out;
}

/* Return the id3 module title name, if found.
 * String return is static memory */
char *
ices_id3_get_title ()
{
	static char title[41];
	char *out = NULL;

	thread_mutex_lock (&id3_mutex);

	if (ices_id3_song) {
		memset (title, 0, 40);
		strncpy (title, ices_util_nullcheck (ices_id3_song), 40);
		out = &title[0];
	}

	thread_mutex_unlock (&id3_mutex);
	return out;
}

/* Return the id3 module genre name, if found.
 * String return is static memory */
char *
ices_id3_get_genre ()
{
	static char genre[41];
	char *out = NULL;

	thread_mutex_lock (&id3_mutex);

	if (ices_id3_genre) {
		memset (genre, 0, 40);
		strncpy (genre, ices_util_nullcheck (ices_id3_genre), 40);
		out = &genre[0];
	}

	thread_mutex_unlock (&id3_mutex);

	return out;
}

/* Return the id3 module file name, if found.
 * String return is static memory */
char *
ices_id3_get_filename ()
{
	static char filename[1025];
	char *out = NULL;

	thread_mutex_lock (&id3_mutex);

	if (ices_id3_filename) {
		memset (filename, 0, 1024);
		strncpy (filename, ices_util_nullcheck (ices_id3_filename), 1024);
		out = &filename[0];
	}
	
	thread_mutex_unlock (&id3_mutex);
	
	return out;
}

/* Private function definitions */

/* Spawn a new thread to update metadata on server. Should be
 * very low overhead */
static void
ices_id3_update_metadata (const char *filename, int file_bytes)
{
	if (thread_create ("Metadata Update Thread", ices_id3_update_thread, NULL) == -1) {
		ices_log ("Error: Could not create metadata update thread!");
	}
}

/* Function used by the updating thread to update metadata on server.
 * It also does the job of cleaning up the song title to something the
 * world likes.
 * Note that the very first metadata update is delayed, because if we
 * try to update our new info to the server and the server has not yet
 * accepted us as a source, the information is lost */
void *
ices_id3_update_thread (void *arg)
{
	int ret;
	char metastring[1024], song[2048];
	char *id3_artist = ices_id3_get_artist ();
	char *id3_song = ices_id3_get_title ();
	const char *filename = ices_id3_get_filename ();
	
	if (id3_artist) {
		sprintf (song, "%s - %s", id3_artist, id3_song ? id3_song : filename);
	} else {
		sprintf (song, "%s", (id3_song != NULL) ? id3_song : ices_id3_filename_cleanup (filename, metastring, 1024));
	}
	
	if (ices_config.header_protocol == icy_header_protocol_e)
		sprintf (metastring, "%s", song);
	else
		sprintf (metastring, "%s", song); /* This should have length as well but libshout doesn't handle it correctly */

	/* If we just started, let's give the icecast server a while to
           accept and mount the source */
	if (ices_playlist_get_current_lineno () <= 1) {
		ices_log_debug ("Initially delaying metadata update...");
		thread_sleep (3000000);
	}

	ret = shout_update_metadata (ices_util_get_conn (), metastring);
	
	if (ret != 1)
		ices_log ("Updating metadata on server failed.");
	else
		ices_log ("Updated metadata on server to: %s", song);
	
	thread_exit (0);
	return 0;
}

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

/* Make a clean slate for the next file */
static void
ices_id3_cleanup ()
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

	if (ices_id3_filename) {
		ices_util_free (ices_id3_filename);
		ices_id3_filename = NULL;
	}
}

/* Cleanup a filename so it looks more like a song name */
static char *
ices_id3_filename_cleanup (const char *oldname, char *namespace, int maxsize)
{
	char *ptr =NULL;

	ices_log_debug ("Filename before cleanup: [%s]", oldname);

	if ((ptr = strrchr (oldname, '/')) && (strlen (ptr) > 0)) {
		strncpy (namespace, ptr + 1, maxsize);
	} else {
		strncpy (namespace, oldname, maxsize);
	}
		    
	if ((ptr = strrchr (namespace, '.'))) {
		*ptr = '\0';
	}

	ices_log_debug ("Filename cleaned up from [%s] to [%s]", oldname, namespace);
	return namespace;
}
