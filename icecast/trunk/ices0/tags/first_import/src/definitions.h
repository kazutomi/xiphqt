/* definitions.h
 * - All declarations and defines for ices
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

#ifdef _WIN32
#include <win32config.h>
#elif defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#ifndef __USE_MISC
# define __USE_MISC
#endif

#ifndef __USE_GNU
# define __USE_GNU
#endif

#ifndef __USE_BSD
# define __USE_BSD
#endif

#ifndef __EXTENSIONS__
# define __EXTENSIONS__
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#ifndef _THREAD_SAFE
# define _THREAD_SAFE
#endif

#ifndef _REENTRANT
# define _REENTRANT
#endif

#ifndef __USE_POSIX
# define __USE_POSIX
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199506L
#endif

/* This for freebsd (needed on 3.2 at least) */
#ifdef SOMEBSD
# ifndef _POSIX_VERSION
# define _POSIX_VERSION 199309L
# endif
#endif

#include <stdio.h>
#include <stdarg.h>
#include <shout.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

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
	char *dumpfile;
	char *configfile;
	char *playlist_file;
	char *base_directory;
}ices_config_t;

#include "setup.h"
#include "stream.h"
#include "log.h"
#include "util.h"
#include "playlist.h"
#include "cue.h"
#include "dj.h"
#include "id3.h"
#include "mp3.h"
#include "signals.h"
#include "playlist_builtin/playlist_builtin.h"
#include "interpreter/interpreter.h"
#include "xml/parse.h"

#define BUFSIZE 8192
#define ICES_DEFAULT_HOST "127.0.0.1"
#define ICES_DEFAULT_PORT 8000
#define ICES_DEFAULT_MOUNT "/ices"
#define ICES_DEFAULT_PASSWORD "letmein"
#define ICES_DEFAULT_HEADER_PROTOCOL xaudiocast_header_protocol_e
#define ICES_DEFAULT_NAME "Default stream name"
#define ICES_DEFAULT_GENRE "Default genre"
#define ICES_DEFAULT_DESCRIPTION "Default description"
#define ICES_DEFAULT_URL "http://www.icecast.org/"
#define ICES_DEFAULT_BITRATE 128
#define ICES_DEFAULT_ISPUBLIC 1
#define ICES_DEFAULT_CONFIGFILE "ices.conf"
#define ICES_DEFAULT_PLAYLIST_FILE "playlist.txt"
#define ICES_DEFAULT_RANDOMIZE_PLAYLIST 0
#define ICES_DEFAULT_DAEMON 0
#define ICES_DEFAULT_PRE_DJ 0
#define ICES_DEFAULT_POST_DJ 0
#define ICES_DEFAULT_BASE_DIRECTORY "/tmp"
#define ICES_DEFAULT_PLAYLIST_TYPE ices_playlist_builtin_e;
#define ICES_DEFAULT_VERBOSE 0
