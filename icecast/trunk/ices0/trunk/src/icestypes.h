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

typedef enum {
  ices_playlist_builtin_e,
  ices_playlist_python_e,
  ices_playlist_perl_e
} playlist_type_t;

typedef struct ices_stream_config_St {
  shout_conn_t conn;
  void* encoder_state;
  int encoder_initialised;

  char* mount;

  char* name;
  char* genre;
  char* description;
  char* url;
  int ispublic;

  int reencode;
  int bitrate;
  int out_samplerate;
  int out_numchannels;

  struct ices_stream_config_St* next;
} ices_stream_config_t;

typedef struct {
  playlist_type_t playlist_type;
  int randomize;
  char* playlist_file;
  char* module;

  char* (*get_next) (void);     /* caller frees result */
  char* (*get_metadata) (void); /* caller frees result */
  int (*get_lineno) (void);
  void (*shutdown) (void);
} playlist_module_t;

typedef struct {
  char *host;
  int port;
  char *password;
  header_protocol_t header_protocol;
  int daemon;
  int verbose;
  int reencode;
  char *dumpfile;
  char *configfile;
  char *base_directory;
  FILE *logfile;

  ices_stream_config_t* streams;
  playlist_module_t pm;
} ices_config_t;

/* -- input stream types -- */
typedef enum {
  ICES_INPUT_VORBIS,
  ICES_INPUT_MP3
} input_type_t;

typedef struct _input_stream_t {
  input_type_t type;

  const char* path;
  int fd;
  int canseek;
  ssize_t filesize;
  unsigned int bitrate;

  void* data;

  ssize_t (*read)(struct _input_stream_t* self, void* buf, size_t len);
  /* len is the size in bytes of left or right. The two buffers must be
   * the same size. */
  ssize_t (*readpcm)(struct _input_stream_t* self, size_t len, int16_t* left,
		     int16_t* right);
  int (*close)(struct _input_stream_t* self);
  int (*get_metadata)(struct _input_stream_t* self, char* buf, size_t len);
} input_stream_t;
#endif
