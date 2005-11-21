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


#include "gui.h"


static gboolean
keypress_cb(GtkWidget *src,
	    GdkEventKey *event,
	    Gui *gui) {

  gdouble adjust = 5.0;
  gdouble level = hildon_volumebar_get_level(HILDON_VOLUMEBAR(gui->volumebar));

  switch (event->keyval) {
  case GDK_F6: /* fullscreen button */
    hildon_appview_set_fullscreen(gui->appview,
			       ! hildon_appview_get_fullscreen(gui->appview));
    return TRUE;

  case GDK_F7: /* increase button */
    hildon_volumebar_set_level(HILDON_VOLUMEBAR(gui->volumebar),
			       level + adjust);
    return TRUE;

  case GDK_F8: /* decrease button */
    hildon_volumebar_set_level(HILDON_VOLUMEBAR(gui->volumebar),
			       level - adjust);
    return TRUE;

  default:
    return FALSE;
  }

}


static void
volume_cb(GtkWidget *src,
	  Gui *gui) {

  (gui->volume_cb)(gui->volume_cb_data,
		   (int) hildon_volumebar_get_level(HILDON_VOLUMEBAR(src)));

}


static void
mute_cb(GtkWidget *src,
	Gui *gui) {

  if (hildon_volumebar_get_mute(HILDON_VOLUMEBAR(src))) {
    (gui->volume_cb)(gui->volume_cb_data, 0);
  } else {
    (gui->volume_cb)(gui->volume_cb_data,
		     (int) hildon_volumebar_get_level(HILDON_VOLUMEBAR(src)));
  }

}


static void
seek_cb(GtkWidget *src,
	Gui *gui) {

  int position;

  position = hildon_seekbar_get_position(HILDON_SEEKBAR(src));
  if (position != gui->seek_position) {
    gui->seek_position = position;
    (gui->seek_cb)(gui->seek_cb_data, position);
  }

}



static void
open_cb(GtkWidget *src,
	Gui *gui) {

  GtkWidget *dialog;
  char *filename;

  dialog = hildon_file_chooser_dialog_new(GTK_WINDOW(gui->appwindow),
					  GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    (gui->open_cb)(gui->open_cb_data, filename);
  }

  gtk_widget_destroy(dialog);

}


static void
play_cb(GtkWidget *src,
	Gui *gui) {

  (gui->control_cb)(gui->control_cb_data, PLAY);

}


static void
stop_cb(GtkWidget *src,
	Gui *gui) {

  (gui->control_cb)(gui->control_cb_data, STOP);

}



Gui *
gui_new() {

  Gui *gui = g_new(Gui, 1);
  GtkWidget *hbox;
  GtkWidget *toolbar;
  GtkToolItem *tb_open;
  GtkToolItem *tb_play;
  GtkToolItem *tb_stop;
  GtkToolItem *tb_seekbar;
  GtkToolItem *tb_timelabel;


  gui->appwindow = HILDON_APP(hildon_app_new());
  hildon_app_set_title(gui->appwindow, "OggPlay");
  hildon_app_set_two_part_title(gui->appwindow, FALSE);

  gui->appview = HILDON_APPVIEW(hildon_appview_new(NULL));
  hildon_app_set_appview(gui->appwindow, gui->appview);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(gui->appview), hbox);

  gui->volumebar = hildon_vvolumebar_new();
  gtk_box_pack_start(GTK_BOX(hbox), gui->volumebar, FALSE, FALSE, 0);

  gui->songlabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), gui->songlabel, TRUE, TRUE, 0);

  toolbar = gtk_toolbar_new();
  gtk_box_pack_end(GTK_BOX(gui->appview->vbox), toolbar, TRUE, TRUE, 0);

  tb_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  gui->tb_play = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
  tb_stop = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);

  gui->seekbar = hildon_seekbar_new();
  gtk_widget_set_size_request(gui->seekbar, 400, -1);
  tb_seekbar = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(tb_seekbar), gui->seekbar);

  gui->timelabel = gtk_label_new("");
  tb_timelabel = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(tb_timelabel), gui->timelabel);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_open, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gui->tb_play, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_stop, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_seekbar, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_timelabel, -1);


  /* connect signals */
  g_signal_connect(G_OBJECT(gui->appwindow), "key-press-event",
		   G_CALLBACK(keypress_cb), gui);

  g_signal_connect(G_OBJECT(gui->volumebar), "level-changed",
		   G_CALLBACK(volume_cb), gui);
  g_signal_connect(G_OBJECT(gui->volumebar), "mute-toggled",
		   G_CALLBACK(mute_cb), gui);


  g_signal_connect(G_OBJECT(gui->seekbar), "value-changed",
		   G_CALLBACK(seek_cb), gui);

  g_signal_connect(G_OBJECT(tb_open), "clicked",
		   G_CALLBACK(open_cb), gui);

  g_signal_connect(G_OBJECT(gui->tb_play), "clicked",
		   G_CALLBACK(play_cb), gui);

  g_signal_connect(G_OBJECT(tb_stop), "clicked",
		   G_CALLBACK(stop_cb), gui);

  
  gtk_widget_show_all(GTK_WIDGET(gui->appwindow));

  return gui;
  
}


void
gui_run() {

  gtk_main();

}


void
gui_quit() {

  gtk_main_quit();

}


void
gui_set_open_cb(Gui *gui,
		OPEN_CB,
		void *userdata) {

  gui->open_cb = open_cb;
  gui->open_cb_data = userdata;

}


void
gui_set_seek_cb(Gui *gui,
		SEEK_CB,
		void *userdata) {

  gui->seek_cb = seek_cb;
  gui->seek_cb_data = userdata;

}


void
gui_set_volume_cb(Gui *gui,
		  VOLUME_CB,
		  void *userdata) {

  gui->volume_cb = volume_cb;
  gui->volume_cb_data = userdata;

}


void
gui_set_control_cb(Gui *gui,
		   CONTROL_CB,
		   void *userdata) {

  gui->control_cb = control_cb;
  gui->control_cb_data = userdata;

}


void
gui_set_title(Gui *gui,
	      const char *title,
	      const char *artist,
	      const char *album) {

  char *text;

  text = g_strdup_printf("%s\n<b>%s</b>\n%s", artist, title, album);
  gtk_label_set_markup(gui->songlabel, text);
  g_free(text);

}


void
gui_set_time(Gui *gui,
	     int seconds,
	     int total) {

  int mins, secs;
  char *time;

  gui->seek_position = seconds;
  hildon_seekbar_set_total_time(HILDON_SEEKBAR(gui->seekbar), total);
  hildon_seekbar_set_fraction(HILDON_SEEKBAR(gui->seekbar), total);
  hildon_seekbar_set_position(HILDON_SEEKBAR(gui->seekbar), seconds);
  
  secs = seconds % 60;
  seconds /= 60;
  mins = seconds;
  time = g_strdup_printf("   %2d:%02d", mins, secs);
  gtk_label_set_text(GTK_LABEL(gui->timelabel), time);
  g_free(time);

}


void
gui_set_paused(Gui *gui,
	       gboolean value) {

  if (value)
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(gui->tb_play),
				 GTK_STOCK_MEDIA_PAUSE);
  else
    gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(gui->tb_play),
				 GTK_STOCK_MEDIA_PLAY);
      
}
