/* util.c
 * - utility functions for ices
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

#include <string.h>
#include <thread.h>

extern shout_conn_t conn;
extern ices_config_t ices_config;

char **ices_argv;
int ices_argc;
const char cnull[7] = "(null)";

/* Public function definitions */
ices_conn_t *
ices_util_get_conn ()
{
	return &conn;
}

ices_config_t *
ices_util_get_config ()
{
	return &ices_config;
}

char *
ices_util_strdup (const char *string)
{
	if (string)
		return strdup (string);
	else
		return strdup ("(null)");
}

char **
ices_util_get_argv ()
{
	return ices_argv;
}

int 
ices_util_get_argc ()
{
	return ices_argc;
}

void
ices_util_set_args (int argc, char **argv)
{
	ices_argc = argc;
	ices_argv = argv;
}

int
ices_util_open_for_reading (const char *file) 
{
	return open (file, O_RDONLY);
}

FILE *
ices_util_fopen_for_reading (const char *file) 
{
	return fopen (file, "r");
}

FILE *
ices_util_fopen_for_writing (const char *file)
{
	char namespace[2048];

	if (!ices_util_directory_exists (ices_config.base_directory)) {
		ices_log ("Ices base directory [%s] does not exist, trying to create", ices_config.base_directory);
		if (ices_util_directory_create (ices_config.base_directory)) {
			ices_log ("Could not create directory [%s]");
			return NULL;
		}
	}
	
	if (file[0] != '/' && file[0] != '\\') {
		sprintf (namespace, "%s/file", ices_config.base_directory);
	} else {
		strncpy (namespace, file, 2048);
	}
	
	return fopen (namespace, "w");
}

void
ices_util_fclose (FILE *fp) 
{
	fclose (fp);
}

int
ices_util_valid_fd (int fd)
{
	return (fd >= 0);
}

char *
ices_util_read_line (FILE *fp)
{
	char temp[1024];

	temp[0] = '\0';

	if (!fgets (temp, 1024, fp)) {

		if (!feof (fp)) {
			ices_log_error ("Got error while reading file, error: [%s]", ices_util_strerror (errno, temp, 1024));
			return NULL;
		}

		return ices_util_strdup (temp);
	}

	return ices_util_strdup (temp);
}

char *
ices_util_get_random_filename (char *namespace, char *type)
{
#ifdef _WIN32
	doooh();
#else
	sprintf (namespace, "/tmp/ices.%s.%d", type, getpid ());
	return namespace;
#endif
}

int
ices_util_remove (const char *filename)
{
	return remove (filename);
}

int
ices_util_get_random () 
{
#ifdef _WIN32
	doooh();
#else
	return getpid();
#endif
}

int
ices_util_is_regular_file (int fd)
{
	struct stat st;
	
	if (fstat (fd, &st) == -1) {
		ices_log_error ("ERROR: Could not stat file");
		return -1;
	}

	if (S_ISLNK (st.st_mode) || S_ISREG (st.st_mode))
		return 1;

	if (S_ISDIR(st.st_mode)) {
		ices_log_error ("ERROR: Is directory!");
		return -2;
	}

	return 0;
}

int 
ices_util_fd_size (int fd)
{
	struct stat st;

	if (fstat (fd, &st) == -1) {
		ices_log_error ("ERROR: Could not stat file");
		return -1;
	}

	return st.st_size;
}

int 
ices_util_directory_exists (const char *name)
{
	struct stat st;
	
	if (stat (name, &st) == -1)
		return 0;

	if (!S_ISDIR (st.st_mode))
		return 0;

	return 1;
}

int
ices_util_directory_create (const char *name)
{
	return mkdir (name, 00755);
}

const char *
ices_util_nullcheck (const char *string)
{
	if (!string)
		return cnull;
	return string;
}

double
ices_util_percent (int this, int of_that)
{
	return (double)((double)this / (double) of_that) * 100.0;
}

char *
ices_util_file_time (int bitrate, int filesize, char *buf)
{
	unsigned long int seconds = (double)(((double)bitrate) / 8.0) / (double) filesize;
	unsigned long int days, hours, minutes, nseconds, remains;

	days = seconds / 86400;
	remains = seconds % 86400;

	hours = remains / 3600;
	remains = remains % 3600;

	minutes = remains / 60;
	nseconds = remains % 60;

	sprintf (buf, "%lu:%lu:%lu:%lu", days, hours, minutes, nseconds);
	
	return buf;
}

const char *
ices_util_strerror (int error, char *namespace, int maxsize)
{
	thread_library_lock ();
	strncpy (namespace, strerror (error), maxsize);
	thread_library_unlock ();
	return namespace;
}

void
ices_util_free (void *ptr)
{
	if (ptr)
		free (ptr);
}

void
ices_util_close (int fd)
{
	close (fd);
}

int
ices_util_verify_file (const char *filename)
{
	FILE *fp;

	if (!filename || !filename[0])
		return 0;

	if (!(fp = ices_util_fopen_for_reading (filename)))
		return 0;

	ices_util_fclose (fp);

	return 1;
}
