/* mp3.h
 * - mp3 function declarations for ices
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

/* -- structures -- */

typedef struct {
  int fd;
} ices_mp3_in_t;

/* Public function declarations */
int ices_mp3_parse_file (const char *file);
int ices_mp3_get_bitrate (void);
int ices_mp3_get_sample_rate (void);
int ices_mp3_get_mode (void);
int ices_mp3_get_channels (void);

ssize_t ices_mp3_read (input_stream_t* self, void* buf, size_t len);
ssize_t ices_mp3_readpcm (input_stream_t* self, size_t len, int16_t* left,
			  int16_t* right);
int ices_mp3_close (input_stream_t* self);
int ices_mp3_get_metadata (input_stream_t* self, char* buf, size_t len);
