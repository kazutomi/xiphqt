/*
 *    Graphical user interface for OggPlay
 *    Copyright (c) 2005 Martin Grimme  <martin.grimme@lintegra.de>
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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "stream.h"
#include "decoder.h"


#define OPEN_CB   void (*open_cb)   (void *userdata, const char *uri)
#define SEEK_CB   void (*seek_cb)   (void *userdata, int seconds)
#define VOLUME_CB void (*volume_cb) (void *userdata, int volume)



struct _Gui {

  OPEN_CB;
  void *open_cb_data;
  SEEK_CB;
  void *seek_cb_data;
  VOLUME_CB;
  void *volume_cb_data;

  int seek_position;

  HildonApp *appwindow;
  HildonAppView *appview;
  GtkWidget *volumebar;
  GtkWidget *songlabel;
  GtkWidget *seekbar;
  GtkWidget *timelabel;

};
typedef struct _Gui Gui;


Gui *gui_new();
void gui_run();
void gui_quit();

void gui_set_open_cb(Gui *gui, OPEN_CB, void *userdata);
void gui_set_seek_cb(Gui *gui, SEEK_CB, void *userdata);
void gui_set_volume_cb(Gui *gui, VOLUME_CB, void *userdata);

void gui_set_title(Gui *gui, const char *title, const char *artist,
		   const char *album);
void gui_set_time(Gui *gui, int seconds, int total);



#endif
