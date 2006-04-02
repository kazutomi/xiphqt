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


#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <glib.h>


#define PLAY_CB    void (*play_cb)    (void *userdata, const char *uri)
#define CHANGE_CB    void (*change_cb)    (void *userdata, gboolean renew)


struct _Playlist {

  PLAY_CB;
  void *play_cb_data;

  CHANGE_CB;
  void *change_cb_data;


  GPtrArray *list;
  int position;

};
typedef struct _Playlist Playlist;



Playlist *playlist_new();
void playlist_free(Playlist *pl);
void playlist_clear(Playlist *pl);
int playlist_get_length(Playlist *pl);
void playlist_append(Playlist *pl, const char *uri);
void playlist_previous(Playlist *pl);
void playlist_next(Playlist *pl);
void playlist_jump_to(Playlist *pl, int index);

void playlist_set_play_cb(Playlist *pl, PLAY_CB, void *userdata);
void playlist_set_change_cb(Playlist *pl, CHANGE_CB, void *userdata);

#endif
