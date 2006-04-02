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
  GtkFileFilter *filter;
  GSList *filenames;

  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Ogg Vorbis");
  /* TODO: better use the MIME type instead of a bunch of patterns, but then
           the Nokia 770 doesn't recognize Ogg files... */
  gtk_file_filter_add_pattern(filter, "*.ogg");
  gtk_file_filter_add_pattern(filter, "*.Ogg");
  gtk_file_filter_add_pattern(filter, "*.OGG");

  dialog = hildon_file_chooser_dialog_new(GTK_WINDOW(gui->appwindow),
					  GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
    (gui->open_cb)(gui->open_cb_data, filenames);
    g_slist_free(filenames);
  }

  gtk_widget_destroy(dialog);

}


static void
prev_cb(GtkWidget *src,
	Gui *gui) {

  (gui->control_cb)(gui->control_cb_data, PREVIOUS);

}


static void
next_cb(GtkWidget *src,
	Gui *gui) {

  (gui->control_cb)(gui->control_cb_data, NEXT);

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
gui_new(Playlist *playlist) {

  Gui *gui = g_new(Gui, 1);
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  GtkWidget *hrule;
  GtkWidget *scroller;
  PLWidget *plw;
  GtkWidget *toolbar;
  GtkToolItem *tb_open;
  GtkToolItem *tb_prev;
  GtkToolItem *tb_next;
  GtkToolItem *tb_play;
  GtkToolItem *tb_stop;
  GtkToolItem *tb_seekbar;
  GtkToolItem *tb_timelabel;

  gui->coverpath = g_strdup("");

  gui->appwindow = HILDON_APP(hildon_app_new());
  hildon_app_set_title(gui->appwindow, FULLNAME);
  hildon_app_set_two_part_title(gui->appwindow, FALSE);

  gui->appview = HILDON_APPVIEW(hildon_appview_new(NULL));
  hildon_app_set_appview(gui->appwindow, gui->appview);

  /* vbox containing everything */
  vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(gui->appview), vbox);
  
  /* cover and tags */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 6);

  gui->albumcover = gtk_image_new();
  gtk_box_pack_start(GTK_BOX(hbox), gui->albumcover, FALSE, FALSE, 6);

  gui->songlabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), gui->songlabel, FALSE, FALSE, 6);

  /* separator */
  hrule = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hrule, FALSE, FALSE, 0);

  /* volume bar and playlist */
  hbox2 = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(vbox), hbox2);

  gui->volumebar = hildon_vvolumebar_new();
  gtk_box_pack_start(GTK_BOX(hbox2), gui->volumebar, FALSE, FALSE, 0);

  scroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), 
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(hbox2), scroller,
		     TRUE, TRUE, 0);
  
  plw = plwidget_new();
  plwidget_set_playlist(plw, playlist);
  gtk_container_add(GTK_CONTAINER(scroller), plwidget_get_widget(plw));

  /* toolbar */
  toolbar = gtk_toolbar_new();
  gtk_box_pack_end(GTK_BOX(gui->appview->vbox), toolbar, TRUE, TRUE, 0);

  tb_prev = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS);
  tb_next = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_NEXT);

  tb_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  gui->tb_play = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
  tb_stop = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);

  gui->seekbar = hildon_seekbar_new();
  gtk_widget_set_size_request(gui->seekbar, 240, -1);
  tb_seekbar = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(tb_seekbar), gui->seekbar);

  gui->timelabel = gtk_label_new("");
  tb_timelabel = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(tb_timelabel), gui->timelabel);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_open, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_prev, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gui->tb_play, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_stop, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_next, -1);
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

  g_signal_connect(G_OBJECT(tb_prev), "clicked",
		   G_CALLBACK(prev_cb), gui);

  g_signal_connect(G_OBJECT(tb_next), "clicked",
		   G_CALLBACK(next_cb), gui);

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

  int mins, secs, tmins, tsecs;
  char *time;

  gui->seek_position = seconds;
  hildon_seekbar_set_total_time(HILDON_SEEKBAR(gui->seekbar), total);
  hildon_seekbar_set_fraction(HILDON_SEEKBAR(gui->seekbar), total);
  hildon_seekbar_set_position(HILDON_SEEKBAR(gui->seekbar), seconds);
  
  secs = seconds % 60;
  seconds /= 60;
  mins = seconds;
  tsecs = total % 60;
  total /= 60;
  tmins = total;

  time = g_strdup_printf("   %2d:%02d / %2d:%02d", mins, secs, tmins, tsecs);
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


void
gui_load_cover(Gui *gui,
	       const char *path) {

  GdkPixbuf *pbuf;

  if (g_ascii_strcasecmp(path, gui->coverpath) == 0) {

    return;

  } else if (g_file_test(path, G_FILE_TEST_EXISTS)) {

    pbuf = gdk_pixbuf_new_from_file_at_scale(path, COVERSIZE, COVERSIZE,
					     TRUE, NULL);
    g_object_ref(G_OBJECT(pbuf));
    gtk_image_set_from_pixbuf(GTK_IMAGE(gui->albumcover), pbuf);
    g_object_unref(G_OBJECT(pbuf));
    gtk_widget_show(gui->albumcover);

    g_free(gui->coverpath);
    gui->coverpath = g_strdup(path);

  } else {

    gtk_widget_hide(gui->albumcover);

    g_free(gui->coverpath);
    gui->coverpath = g_strdup("");

  }

}


void
gui_show_error(Gui *gui,
	       const char *message) {

  gtk_infoprint_with_icon_stock(GTK_WINDOW(gui->appwindow),
				message, GTK_STOCK_DIALOG_ERROR);
  
}
