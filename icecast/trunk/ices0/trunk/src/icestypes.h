/* icestypes.h
 * - Datatypes for ices
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

#ifndef _ICES_ICESTYPES_H
#define _ICES_ICESTYPES_H

typedef enum {icy_header_protocol_e = 0, xaudiocast_header_protocol_e = 1} header_protocol_t;
typedef enum {ices_playlist_python_e = 0, ices_playlist_builtin_e = 1, ices_playlist_perl_e = 2} playlist_type_t;

typedef struct ices_config_St {
	char *host;
	int port;
	char *mount;
	char *password;
	header_protocol_t header_protocol;
	char *name;
	char *genre;
	char *description;
	char *url;
	int bitrate;
	int ispublic;
	int daemon;
	int randomize_playlist;
	int pre_dj;
	int post_dj;
	int playlist_type;
	int verbose;
	int reencode;
	char *dumpfile;
	char *configfile;
	char *playlist_file;
	char *interpreter_file;
	char *base_directory;
}ices_config_t;

#endif
