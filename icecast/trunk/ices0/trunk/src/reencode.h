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
void ices_reencode_initialize ();
void ices_reencode_shutdown ();
int ices_reencode_reencode_chunk (unsigned char *buff, int buflen, unsigned char *outbuf, int outlen);
void ices_reencode_set_channels (int channels);
void ices_reencode_set_sample_rate (int samplerate);
void ices_reencode_set_mode (int mode);
void ices_reencode_reset ();
int ices_reencode_flush (unsigned char *outbuf, int maxlen);

