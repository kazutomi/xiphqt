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
static int ices_stream_send_reencoded (shout_conn_t *conn, unsigned char *buff, int read_bytes, int file_bytes);

/* Public function definitions */

/* Top level streaming function, called once from main() to
 * connect to server and start streaming */
void
ices_stream_loop ()
{
	char namespace[1024];

	/* Check if user gave us a hostname */
	if (!isdigit ((int)(conn.ip[strlen (conn.ip) - 1]))) {
		char hostbuff[1024];

		/* Ok, he did, let's get the ip */
		conn.ip = resolver_getip (conn.ip, hostbuff, 1024);
		if (conn.ip == NULL) {
			ices_log ("Failed resolving servername.");
			ices_setup_shutdown ();
		}
	}

	/* Tell libshout to connect to the server */
	if (!shout_connect (&conn)) {
		ices_log ("Failed connecting to server %s, error: %s", conn.ip, shout_strerror(&conn, conn.error, namespace, 1024));
		ices_setup_shutdown ();
	}
	
	ices_log ("Connected to server %s...", conn.ip);

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
			/* Let's see if it was a libshout socket error */
			if (conn.error == SHOUTERR_SOCKET) {
				ices_log ("Libshout communication problem, trying to reconnect to server");
				
				/* Make sure we disconnect */
				shout_disconnect (&conn);
				
				/* Reconnect */
				if (!shout_connect (&conn)) {
					ices_log ("Failed reconnecting to server %s, error: %i", conn.ip, conn.error);
					ices_setup_shutdown ();
				}
			}
			
			ices_util_free (file);
			continue;
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
	int file_bytes, ret;
	int fd = ices_util_open_for_reading (file);
	char namespace[1024];

	/* Could we open this file, is the file handle valid? */
	if (!ices_util_valid_fd (fd)) {
		ices_log_error ("Could not open file %s, error: %s", file, ices_util_strerror (errno, namespace, 1024));
		return 0;
	}

	/* Check the size of the file to stream */
	if ((file_bytes = ices_util_fd_size (fd)) == -1) {
		ices_util_close (fd);
		return 0;
	}
	
	/* Check the type of the file. If it's a regular file then we
	 * check the size of it and stream that size.
	 * If it's a fifo, we stream until eof. This is handled
	 * by two separate functions cause the semantics are too
	 * unwieldy to combine. */
	switch (ices_util_is_regular_file (fd)) {
		case -1:
		case -2:
			/* Some stat error */
			ices_util_close (fd);
			return 0;
			break;
		case 0:
			/* Fifo or pipe */
			ices_stream_fd_until_eof (fd, file);
			ices_util_close (fd);
			return 1;
			break;
	}

	/* Rest if for regular files only  */

	/* Check mp3 headers for bitrate, mode, channels, etc... */
	ices_mp3_parse_file (file);

	/* Check for id3 tag
	 * Send metadata to server
	 * Return file size excluding id3 tag
	 * This also does the metadata update on the server */
	file_bytes = ices_id3_parse_file (file, file_bytes);

	/* Id3 tag parsing went wrong */
	if (file_bytes <= 0) {
		ices_log_error ("Unable to get file size, or zero length file: %s, error: %s", file, ices_util_strerror (errno, namespace, 1024));
		ices_util_close (fd);
		return 0;
	}

	/* Stream file_bytes bytes from fd */
	ret = ices_stream_fd_size (fd, file, file_bytes);

	/* Be a good boy and close the file */
	ices_util_close (fd);

	return ret;
}

/* Stream file_bytes bytes from fd */
static int
ices_stream_fd_size (int fd, const char *file, int file_bytes)
{
	int read_bytes, ret = -1, orig_size = file_bytes;
	int bitrate = ices_mp3_get_bitrate ();
	unsigned char buff[4096];
	char namespace[1024];
	ices_config_t *ices_config = ices_util_get_config ();

	ices_log ("Streaming %d bytes from file %s", file_bytes, file);

#ifdef HAVE_LIBLAME
	if (ices_config->reencode) {
		ices_reencode_set_channels (ices_mp3_get_channels ());
		ices_reencode_set_mode (ices_mp3_get_mode ());
		ices_reencode_set_sample_rate (ices_mp3_get_sample_rate ());
		ices_reencode_reset ();
	}
#endif

	while (file_bytes >= 0) {
		read_bytes = read (fd, buff, 4096);
		
		if (read_bytes > 0) {

			file_bytes -= read_bytes;

			if (ices_config->reencode) {
				/* Users wants us to reencode to different
				 * bitrate, this reencodes and sends the
				 * reencoded buff to the server */
				ret = ices_stream_send_reencoded (&conn, buff, read_bytes, file_bytes);
			} else {
				/* Send read data as-is to the server */
				ret = shout_send_data (&conn, buff, read_bytes);
			}

			if (!ret) {
				ices_log_error ("Libshout reported send error: %s", shout_strerror (&conn, conn.error, namespace, 1024));
				return 0;
			}
			
		} else if (read_bytes == 0) 
			return 1;
		else {
			ices_log_error ("Read error while reading %s: %s", file, ices_util_strerror (errno, namespace, 1024));
			return 0;
		}

		/* Update cue file */
		ices_cue_update (file, orig_size, bitrate, orig_size - file_bytes);
		/* Give the libshout module a chance to sleep */
		shout_sleep(&conn);
	}

	/* All is fine */
	return 1;
}

/* This function is used when streaming from fifo or pipe */
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
                                ices_log_error ("Libshout reported send error: %s", shout_strerror (&conn, conn.error, namespace, 1024));
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

/* Reencode and send given chunk to server.
 * If file is up, flush the reencoding buffers and send the remains */
static int
ices_stream_send_reencoded (shout_conn_t *conn, unsigned char *buff, int read_bytes, int file_bytes)
{
	int ret = 1;
#ifdef HAVE_LIBLAME
	unsigned char reencode_buff[15000];
	int len = ices_reencode_reencode_chunk (buff, read_bytes, reencode_buff, 15000);
	
	if (len > 0)
		ret = shout_send_data (conn, reencode_buff, len);
	else if (len < 0)
		return 0;
	
	/* Flush and send remains if file is up */
	if (file_bytes <= 0) {
		len = ices_reencode_flush (reencode_buff, 15000);
		if (len > 0)
			ret = shout_send_data (conn, reencode_buff, len);
	}
#endif
	return ret;
}


