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

#include <resolver.h>

extern shout_conn_t conn;

/* Private function declarations */
static int ices_stream_send_file (const char *file);
static int ices_stream_fd_size (int fd, const char *file, int file_bytes);
static int ices_stream_fd_until_eof (int fd, const char *file);

/* Public function definitions */
void
ices_stream_loop ()
{
	if (!isdigit ((int)(conn.ip[strlen (conn.ip) - 1]))) {
		char hostbuff[1024];

		conn.ip = resolver_lookup (conn.ip, hostbuff, 1024);
		if (conn.ip == NULL) {
			ices_log ("Failed resolving servername.");
			return;
		}
	}

	if (!shout_connect (&conn)) {
		ices_log ("Failed connecting to server %s, error: %i\n", conn.ip, conn.error);
		return;
	}
	
	ices_log ("Connected to server %s...", conn.ip);

	while (1) {
		char *file;

		ices_dj_pre ();

		file = ices_playlist_get_next ();

		if (!file) {
			ices_log ("Warning: ices_file_get_next() gave me an error, this is not good. [%s]", ices_log_get_error ());
			return;
		}
		
		if (!ices_stream_send_file (file)) {
			ices_log ("Warning: Encountered error while transfering %s. [%s]", file, ices_log_get_error ());
			continue;
		}
		
		ices_dj_post ();

		ices_util_free (file);
	}
}

static int
ices_stream_send_file (const char *file)
{
	int file_bytes, ret;
	int fd = ices_util_open_for_reading (file);
	char namespace[1024];

	if (!ices_util_valid_fd (fd)) {
		ices_log_error ("Could not open file %s, error: %s", file, ices_util_strerror (errno, namespace, 1024));
		return 0;
	}

	/* Check the size of the file to stream */
	if ((file_bytes = ices_util_fd_size (fd)) == -1) {
		ices_util_close (fd);
		return 0;
	}
	
	switch (ices_util_is_regular_file (fd)) {
		case -1:
		case -2:
			ices_util_close (fd);
			return 0;
			break;
		case 0:
			ices_stream_fd_until_eof (fd, file);
			ices_util_close (fd);
			break;
	}

	/* Check for id3 tag
	 * Send metadata to server
	 * Return file size excluding id3 tag */
	file_bytes = ices_id3_parse_file (file, file_bytes);

	if (file_bytes <= 0) {
		ices_log_error ("Unable to get file size, or zero length file: %s, error: %s", file, ices_util_strerror (errno, namespace, 1024));
		ices_util_close (fd);
		return 0;
	}

	ret = ices_stream_fd_size (fd, file, file_bytes);
	ices_util_close (fd);

	return ret;
}

static int
ices_stream_fd_size (int fd, const char *file, int file_bytes)
{
	int read_bytes, ret;
	unsigned char buff[4096];
	int orig_size = file_bytes;
	int bitrate = ices_mp3_get_bitrate (file);
	char namespace[1024];

	ices_log ("Streaming %d bytes from file %s", file_bytes, file);

	while (file_bytes >= 0) {
		read_bytes = read (fd, buff, 4096);
		
		if (read_bytes > 0) {

			file_bytes -= read_bytes;

			ret = shout_send_data (&conn, buff, read_bytes);

			if (!ret) {
				ices_log_error ("Libshout reported send error: %i...", conn.error);
				break;
			}
			
		} else if (read_bytes == 0) 
			return 1;
		else {
			ices_log_error ("Read error while reading %s: %s", file, ices_util_strerror (errno, namespace, 1024));
			return 0;
		}

		/* Update cue file */
		ices_cue_update (file, orig_size, bitrate, orig_size - file_bytes);

		shout_sleep(&conn);
	}

	return 1;
}

static int
ices_stream_fd_until_eof (int fd, const char *file)
{
	int file_bytes_read, read_bytes, ret;
        unsigned char buff[4096];
	char namespace[1024];

	ices_log ("Streaming from %s until EOF..", file);
	
        file_bytes_read = 0;
	
        while (1) {
                read_bytes = read (fd, buff, 4096);
		
                if (read_bytes > 0) {
			
                        file_bytes_read = file_bytes_read + read_bytes;
                        ret = shout_send_data (&conn, buff, read_bytes);
			
                        if (!ret) {
                                ices_log_error ("Libshout reported send error:%i...", conn.error);
                                break;
                        }
			
                } else if (read_bytes == 0)
                        return 1;
                else {
                        ices_log_error ("Read error while reading %s: %s", file, ices_util_strerror (errno, namespace, 1024));
                        return 0;
                }
                shout_sleep(&conn);
        }
	return 1;
}

	
