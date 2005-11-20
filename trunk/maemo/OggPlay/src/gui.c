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

#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-seekbar.h>
#include <hildon-widgets/hildon-vvolumebar.h>
#include <gtk/gtk.h>

#include <libosso.h>

#include "stream.h"
#include "decoder.h"


#define SERVICE_NAME "oggplay"
#define VERSION "0.12"


struct _AppData {

  Decoder *decoder;
  Stream *stream;
  HildonApp *app;
  HildonAppView *appview;
  GtkWidget *seekbar;
  GtkWidget *songlabel;
  GtkWidget *timelabel;
  int current_position;

};
typedef struct _AppData AppData;



static int
idle_cb(AppData *appdata) {

  gint p, t;
  int mins, secs;
  char *time;
  
  p = decoder_get_position(appdata->decoder);
  t = decoder_get_total(appdata->decoder);

  appdata->current_position = p;
  hildon_seekbar_set_total_time(HILDON_SEEKBAR(appdata->seekbar), t);
  hildon_seekbar_set_position(HILDON_SEEKBAR(appdata->seekbar), p);
  hildon_seekbar_set_fraction(HILDON_SEEKBAR(appdata->seekbar), t);

  secs = p % 60;
  p /= 60;
  mins = p;
  time = g_strdup_printf("   %2d:%02d", mins, secs);
  gtk_label_set_text(GTK_LABEL(appdata->timelabel), time);
  g_free(time);

  return TRUE;

}

static void
open_cb(GtkWidget *src,
	AppData *appdata) {

  GtkWidget *dialog;
  gchar *filename;
  char *text;

  dialog = hildon_file_chooser_dialog_new(GTK_WINDOW(appdata->app),
					  GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {

    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    if (appdata->stream) {
      decoder_close_stream(appdata->decoder);
    }

    appdata->current_position = 0;
    hildon_seekbar_set_position(HILDON_SEEKBAR(appdata->seekbar), 0);
    appdata->stream = stream_new_from_uri(filename);
    decoder_open_stream(appdata->decoder, appdata->stream);
    text = g_strdup_printf("%s\n<b>%s</b>\n%s",
			   appdata->decoder->tag_artist,
			   appdata->decoder->tag_title,
			   appdata->decoder->tag_album);
    gtk_label_set_markup(appdata->songlabel, text);
    g_free(text);
  }
  gtk_widget_destroy(dialog);

}


static void
volume_cb(GtkWidget *src,
	  AppData *appdata) {

  decoder_set_volume(appdata->decoder,
		     (int) hildon_volumebar_get_level(HILDON_VOLUMEBAR(src)));

}


static void
mute_cb(GtkWidget *src,
	AppData *appdata) {

  if (hildon_volumebar_get_mute(HILDON_VOLUMEBAR(src))) {
    decoder_set_volume(appdata->decoder, 0);
  } else {
    decoder_set_volume(appdata->decoder,
		     (int) hildon_volumebar_get_level(HILDON_VOLUMEBAR(src)));
  }

}


static void
seek_cb(GtkWidget *src,
	AppData *appdata) {

  int position;

  position = hildon_seekbar_get_position(HILDON_SEEKBAR(appdata->seekbar));
  if (position != appdata->current_position) {
    appdata->current_position = position;
    decoder_seek(appdata->decoder, position * 1000);
  }

}



int main(int argc, char *argv[]) {

  osso_context_t *osso_context;

  HildonApp *app;
  HildonAppView *appview;
  GtkWidget *hbox;
  GtkWidget *volumebar;
  GtkWidget *songlabel;
  GtkWidget *toolbar;
  GtkToolItem *tb_open;
  GtkToolItem *tb_seekbar;
  GtkToolItem *tb_timelabel;
  GtkWidget *seekbar;

  AppData *appdata;

  gtk_init(&argc, &argv);
  
  app = HILDON_APP(hildon_app_new());

  hildon_app_set_title(app, "OggPlay");
  hildon_app_set_two_part_title(app, FALSE);

  appdata = g_new(AppData, 1);
  appdata->app = app;
  appdata->decoder = decoder_new();
  appdata->stream = NULL;

  appview = HILDON_APPVIEW(hildon_appview_new(NULL));
  appdata->appview = appview;
  hildon_app_set_appview(app, appview);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(appview), hbox);

  volumebar = hildon_vvolumebar_new();
  g_signal_connect(G_OBJECT(volumebar), "level-changed",
		   G_CALLBACK(volume_cb), appdata);
  g_signal_connect(G_OBJECT(volumebar), "mute-toggled",
		   G_CALLBACK(mute_cb), appdata);
  gtk_box_pack_start(GTK_BOX(hbox), volumebar, FALSE, FALSE, 0);

  songlabel = gtk_label_new("");
  gtk_box_pack_start(GTK_BOX(hbox), songlabel, TRUE, TRUE, 0);
  appdata->songlabel = songlabel;

  toolbar = gtk_toolbar_new();
  tb_open = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
  g_signal_connect(G_OBJECT(tb_open), "clicked",
		   G_CALLBACK(open_cb), appdata);

  seekbar = hildon_seekbar_new();
  appdata->seekbar = seekbar;
  g_signal_connect(G_OBJECT(seekbar), "value-changed",
		   G_CALLBACK(seek_cb), appdata);
  tb_seekbar = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(tb_seekbar), seekbar);
  gtk_widget_set_size_request(seekbar, 400, -1);

  appdata->timelabel = gtk_label_new("");
  tb_timelabel = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(tb_timelabel), appdata->timelabel);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_open, -1);  
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_seekbar, -1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tb_timelabel, -1);

  gtk_box_pack_end(GTK_BOX(appview->vbox), toolbar, TRUE, TRUE, 0);
  
  gtk_widget_show_all(GTK_WIDGET(app));


  osso_context = osso_initialize(SERVICE_NAME, VERSION, TRUE, NULL);
  if (! osso_context)
    fprintf(stderr, "Could not initialize OSSO.");

  g_timeout_add(500, (GSourceFunc) idle_cb, appdata);


  gtk_main();
  
  return 0;

}
