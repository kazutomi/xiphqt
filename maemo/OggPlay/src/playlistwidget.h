/*
 *    Playlist widget
 *    Copyright (c) 2005, 2006 Martin Grimme  <martin.grimme@lintegra.de>
 *
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


#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <gtk/gtk.h>
#include <glib.h>
#include "playlist.h"

/* callback handlers */
//#define OPEN_CB    void (*open_cb)    (void *userdata, GSList *filenames)
//#define SEEK_CB    void (*seek_cb)    (void *userdata, int seconds)
//#define VOLUME_CB  void (*volume_cb)  (void *userdata, int volume)
//#define CONTROL_CB void (*control_cb) (void *userdata, int command)

//enum Command { PLAY, STOP, PREVIOUS, NEXT };


struct _PLWidget {

  GtkListStore *liststore;
  GtkWidget *treeview;

  Playlist *playlist;
  int current_selection;

};
typedef struct _PLWidget PLWidget;


PLWidget *plwidget_new();
GtkWidget *plwidget_get_widget(PLWidget *plw);
void plwidget_set_playlist(PLWidget *plw, Playlist *pl);

#endif
