/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License. The original
 *  author of the programm (Tino H. Seifert) will decide if later versions
 *  are also acceptable.
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


#include "stream.h"

#define BUFFER_SIZE 1024

static pthread_mutex_t cmdmutex = PTHREAD_MUTEX_INITIALIZER;


static void *
reader_thread(Stream *stream) {

  char buffer[BUFFER_SIZE];
  char *tmp;
  
  GnomeVFSHandle *handle;
  gboolean is_open = FALSE;
  gboolean running = TRUE;
  long position = 0;
  GnomeVFSSeekPosition vfswhence;

  GnomeVFSResult result;
  GnomeVFSFileSize bytes_read;
  
  tmp = g_new0(char, 8);

  while (running) {
    /* process commands */
    switch (stream->cmd) {
    case (STREAM_OPEN):
      printf("Opening stream %s\n", stream->uri);
      result = gnome_vfs_open(&handle, stream->uri, GNOME_VFS_OPEN_READ);
      if (result == GNOME_VFS_OK) {
	is_open = TRUE;
	position = 0;
      }
      break;
    case (STREAM_CLOSE):
      printf("Closing stream %s\n", stream->uri);      
      gnome_vfs_close(handle);
      is_open = FALSE;
      running = FALSE;
      break;
    case (STREAM_SEEK):
      printf("Seeking %d %d\n", stream->seek_whence, stream->seek_position);
      if (is_open) {
	if (stream->seek_whence == SEEK_SET)
	  vfswhence = GNOME_VFS_SEEK_START;
	else if (stream->seek_whence == SEEK_CUR)
	  vfswhence = GNOME_VFS_SEEK_CURRENT;
	else if (stream->seek_whence == SEEK_END)
	  vfswhence = GNOME_VFS_SEEK_END;

	result = gnome_vfs_seek(handle, vfswhence,
				(GnomeVFSFileOffset) stream->seek_position);
	ringbuffer_clear(stream->buffer);
	gnome_vfs_tell(handle, &bytes_read);
	position = (long) bytes_read;
	stream->positiontag = position;
      }
      break;
    default:
      break;
    }
    stream->cmd = STREAM_NONE;
    pthread_mutex_unlock(&cmdmutex);


    if (is_open && ! ringbuffer_is_full(stream->buffer)) {
      result = gnome_vfs_read(handle, buffer, sizeof(buffer), &bytes_read);
      if (result == GNOME_VFS_OK) {

	position += (long) bytes_read;
	((long *) tmp)[0] = position;
	ringbuffer_put(stream->buffer, tmp, 8);
	ringbuffer_put(stream->buffer, buffer, (int) bytes_read);

      } else {

	ringbuffer_put(stream->buffer, tmp, 8);
	ringbuffer_put(stream->buffer, "", 0);

      }
    }

    usleep(1000);
  }

  pthread_mutex_unlock(&cmdmutex);

}


Stream *
stream_new_from_uri(const char *uri) {

  pthread_t th;
  Stream *stream = g_new(Stream, 1);
  GnomeVFSResult result;

  if (! gnome_vfs_initialized())
    gnome_vfs_init();

  stream->uri = (char *) uri;
  stream->buffer = ringbuffer_new(10, BUFFER_SIZE);
  stream->cmd = STREAM_OPEN;
  stream->positiontag = 0;

  pthread_mutex_lock(&cmdmutex);
  pthread_create(&th, NULL, reader_thread, (void *) stream);

  return stream;

}

void
stream_free(Stream *stream) {

  pthread_mutex_lock(&cmdmutex);
  stream->cmd = STREAM_CLOSE;

  pthread_mutex_lock(&cmdmutex);
  g_free(stream->uri);
  ringbuffer_free(stream->buffer);
  g_free(stream);

}


int
stream_read(Stream *stream,
	    char *buffer,
	    int size) {

  pthread_mutex_lock(&cmdmutex);
  stream->cmd = STREAM_READ;  /* just to ensure the correct order */
  ringbuffer_get(stream->buffer, buffer);
  stream->positiontag = ((long *) buffer)[0];

  return ringbuffer_get(stream->buffer, buffer);

}


long
stream_tell(Stream *stream) {

  pthread_mutex_lock(&cmdmutex);
  printf("Tell %d\n", stream->positiontag);
  return stream->positiontag;

}


void
stream_seek(Stream *stream,
	    long position,
	    int whence) {

  pthread_mutex_lock(&cmdmutex);

  if (whence == SEEK_CUR) {
    whence = SEEK_SET;
    position = stream->positiontag + position;
  }

  stream->seek_position = position;
  stream->seek_whence = whence;
  stream->cmd = STREAM_SEEK;

}
