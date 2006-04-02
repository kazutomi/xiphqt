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


#include "playlist.h"


/*
 * Creates and returns a new playlist object.
 */
Playlist *
playlist_new() {

  Playlist *pl;

  pl = g_new(Playlist, 1);
  pl->list = g_ptr_array_new();
  pl->position = 0;

  return pl;

}


void
playlist_free(Playlist *pl) {

  g_ptr_array_free(pl->list, TRUE);
  g_free(pl);

}


void
playlist_clear(Playlist *pl) {

  if (pl->list->len > 0) {
    g_ptr_array_foreach (pl->list, g_free, NULL);
    g_ptr_array_remove_range(pl->list, 0, pl->list->len);
    (pl->change_cb)(pl->change_cb_data);
  }

}


void
playlist_append(Playlist *pl,
		const char *uri) {

  g_ptr_array_add(pl->list, g_strdup(uri));
  (pl->change_cb)(pl->change_cb_data);

}


void
playlist_previous(Playlist *pl) {

  char *uri;

  if (pl->list->len == 0) return;

  pl->position = MAX(0, pl->position - 1);  
  uri = g_ptr_array_index(pl->list, pl->position);
  (pl->play_cb)(pl->play_cb_data, uri);

}


void
playlist_next(Playlist *pl) {

  char *uri;
  
  if (pl->list->len == 0) return;

  if (pl->position == pl->list->len - 1) {

    (pl->play_cb)(pl->play_cb_data, NULL);

  } else {

    pl->position = MIN(pl->list->len - 1, pl->position + 1);
    uri = g_ptr_array_index(pl->list, pl->position);
    (pl->play_cb)(pl->play_cb_data, uri);

  }

}


void
playlist_jump_to(Playlist *pl,
		 int index) {

  char *uri;

  if (index < pl->list->len) {
    pl->position = index;
    uri = g_ptr_array_index(pl->list, pl->position);
    (pl->play_cb)(pl->play_cb_data, uri);
  }
    
}


void
playlist_set_play_cb(Playlist *pl,
		     PLAY_CB,
		     void *userdata) {

  pl->play_cb = play_cb;
  pl->play_cb_data = userdata;

}


void
playlist_set_change_cb(Playlist *pl,
		       CHANGE_CB,
		       void *userdata) {
  
  pl->change_cb = change_cb;
  pl->change_cb_data = userdata;

}
