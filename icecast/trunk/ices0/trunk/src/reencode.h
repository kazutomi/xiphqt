/* reencode.h
 * - Function declarations for reencoding
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

/* Public function declarations */
void ices_reencode_initialize (void);
void ices_reencode_shutdown (void);
int ices_reencode_decode (unsigned char* buf, size_t blen);
int ices_reencode_reencode_chunk (ices_stream_config_t* stream,
				  unsigned char *buff, int buflen,
				  unsigned char *outbuf, int outlen);
void ices_reencode_reset (void);
int ices_reencode_flush (ices_stream_config_t* stream, unsigned char *outbuf,
			 int maxlen);

