/*
 *    Graphical user interface for OggPlay
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


#ifndef GUI_H
#define GUI_H

#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-seekbar.h>
#include <hildon-widgets/hildon-vvolumebar.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include "version.h"
#include "stream.h"
#include "decoder.h"
#include "playlistwidget.h"
#include "playlist.h"


/* size of the cover image */
#define COVERSIZE 112

/* callback handlers */
#define OPEN_CB    void (*open_cb)    (void *userdata, GSList *filenames)
#define SEEK_CB    void (*seek_cb)    (void *userdata, int seconds)
#define VOLUME_CB  void (*volume_cb)  (void *userdata, int volume)
#define CONTROL_CB void (*control_cb) (void *userdata, int command)

enum Command { PLAY, STOP, PREVIOUS, NEXT, CLEAR };


struct _Gui {

  OPEN_CB;
  void *open_cb_data;
  SEEK_CB;
  void *seek_cb_data;
  VOLUME_CB;
  void *volume_cb_data;
  CONTROL_CB;
  void *control_cb_data;

  char *coverpath;
  int seek_position;

  HildonApp *appwindow;
  HildonAppView *appview;
  GtkWidget *volumebar;
  GtkWidget *albumcover;
  GtkWidget *songlabel;
  GtkWidget *seekbar;
  GtkWidget *timelabel;
  GtkWidget *tb_play;

};
typedef struct _Gui Gui;


Gui *gui_new(Playlist *playlist);
void gui_run();
void gui_quit();

void gui_set_open_cb(Gui *gui, OPEN_CB, void *userdata);
void gui_set_seek_cb(Gui *gui, SEEK_CB, void *userdata);
void gui_set_volume_cb(Gui *gui, VOLUME_CB, void *userdata);
void gui_set_control_cb(Gui *gui, CONTROL_CB, void *userdata);

void gui_set_title(Gui *gui, const char *title, const char *artist,
		   const char *album);
void gui_set_time(Gui *gui, int seconds, int total);
void gui_set_paused(Gui *gui, gboolean value);

void gui_load_cover(Gui *gui, const char *path);

void gui_show_error(Gui *gui, const char *message);

#endif
