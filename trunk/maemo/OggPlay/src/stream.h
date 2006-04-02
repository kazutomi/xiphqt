/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef STREAM_H
#define STREAM_H

#include <libgnomevfs/gnome-vfs.h>
#include <stdio.h>
#include <glib.h>
#include <pthread.h>

#include "ringbuffer.h"


enum streamcommand { STREAM_NONE, STREAM_OPEN, STREAM_CLOSE,
		     STREAM_SEEK, STREAM_READ };

struct _Stream {

  char *uri;
  GnomeVFSHandle *handle;
  enum streamcommand cmd;

  int error;

  long positiontag;
  long seek_position;
  int seek_whence;

  RingBuffer *buffer;

};

typedef struct _Stream Stream;


Stream *stream_new_from_uri(const char *uri);
void stream_free(Stream *stream);
int stream_read(Stream *stream, char *buffer, int size);
long stream_tell(Stream *stream);
void stream_seek(Stream *stream, long position, int whence);

#endif
