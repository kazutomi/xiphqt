/* mp3.c
 * - Functions for mp3 in ices
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

#define MPG_MD_MONO 3

typedef struct mp3_headerSt
{
	int lay;
	int version;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int stereo;
} mp3_header_t;

static unsigned int bitrates[3][3][15] =
{
	{
		{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
		{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
	},
	{
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	},
	{
		{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
		{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
	}
};

static unsigned int s_freq[3][4] =
{
	{44100, 48000, 32000, 0},
	{22050, 24000, 16000, 0},
	{11025, 8000, 8000, 0}
};

static char *mode_names[5] = {"stereo", "j-stereo", "dual-ch", "single-ch", "multi-ch"};
static char *layer_names[3] = {"I", "II", "III"};
static char *version_names[3] = {"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};

/* Global function definitions */
int
ices_mp3_get_bitrate (const char *filename)
{
	FILE *file;
	unsigned char buff[1024];
	unsigned char *buffer;
	size_t temp;
	size_t readsize;
	
	mp3_header_t mh;
	
	if ((file = ices_util_fopen_for_reading (filename)) == NULL) {
		ices_log_error ("Could not access [%s], error: %s", filename, ices_util_strerror (errno, buff, 1024));
		ices_util_fclose (file);
		return -2;
	}
	
	fseek (file, 0, SEEK_SET);
	readsize = fread (&buff, 1, 1024, file);
	readsize -= 4;
	
	if (readsize <= 0) {
		ices_util_fclose (file);
		return -2;
	}
	
	buffer = buff - 1;
	
	do {
		buffer++;
		temp = ((buffer[0] << 4) & 0xFF0) | ((buffer[1] >> 4) & 0xE);
	} while ((temp != 0xFFE) && ((size_t)(buffer - buff) < readsize));
	
	if (temp != 0xFFE) {
		ices_util_fclose (file);
		return -2;
	} else {

		switch ((buffer[1] >> 3 & 0x3)) {
			case 3:
				mh.version = 0;
				break;
			case 2:
				mh.version = 1;
				break;
			case 0:
				mh.version = 2;
				break;
			default:
				ices_util_fclose (file);
				return -2;
				break;
		}

		mh.lay = 4 - ((buffer[1] >> 1) & 0x3);
		mh.error_protection = !(buffer[1] & 0x1);
		mh.bitrate_index = (buffer[2] >> 4) & 0x0F;
		mh.sampling_frequency = (buffer[2] >> 2) & 0x3;
		mh.padding = (buffer[2] >> 1) & 0x01;
		mh.extension = buffer[2] & 0x01;
		mh.mode = (buffer[3] >> 6) & 0x3;
		mh.mode_ext = (buffer[3] >> 4) & 0x03;
		mh.copyright = (buffer[3] >> 3) & 0x01;
		mh.original = (buffer[3] >> 2) & 0x1;
		mh.emphasis = (buffer[3]) & 0x3;
		mh.stereo = (mh.mode == MPG_MD_MONO) ? 1 : 2;

		ices_log_debug ("Layer: %s\t\tVersion: %s\tFrequency: %d", layer_names[mh.lay - 1], version_names[mh.version], s_freq[mh.version][mh.sampling_frequency]);
		ices_log_debug ("Bitrate: %d kbit/s\tPadding: %d\tMode: %s", bitrates[mh.version][mh.lay - 1][mh.bitrate_index], mh.padding, mode_names[mh.mode]);
		ices_log_debug ("Ext: %d\tMode_Ext: %d\tCopyright: %d\tOriginal: %d", mh.extension, mh.mode_ext, mh.copyright, mh.original);
		ices_log_debug ("Error Protection: %d\tEmphasis: %d\tStereo: %d", mh.error_protection, mh.emphasis, mh.stereo);
		ices_util_fclose (file);

		return bitrates[mh.version][mh.lay - 1][mh.bitrate_index];
	}
}


			
