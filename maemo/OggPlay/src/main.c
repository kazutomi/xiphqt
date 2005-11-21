/*
 *    Ogg Vorbis player for the Nokia 770
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
#include "decoder.h"
#include "stream.h"
#include <libosso.h>


#define SERVICE_NAME "oggplay"
#define VERSION "0.12"


struct _AppData {

  Decoder *decoder;
  Stream *stream;
  Gui *gui;
  int current_position;

};
typedef struct _AppData AppData;




/* callback for updating status information */
static int
status_cb(AppData *appdata) {

  int p, t;
  
  p = decoder_get_position(appdata->decoder);
  t = decoder_get_total(appdata->decoder);
  gui_set_time(appdata->gui, p, t);

  return TRUE;

}



/* callback for opening a file */
static void
open_cb(AppData *appdata,
	const char *uri) {

  /* close old stream */
  if (appdata->stream)
    decoder_close_stream(appdata->decoder);

  /* open new stream */
  appdata->stream = stream_new_from_uri(uri);
  decoder_open_stream(appdata->decoder, appdata->stream);

  gui_set_title(appdata->gui,
		appdata->decoder->tag_title,
		appdata->decoder->tag_artist,
		appdata->decoder->tag_album);

}


/* callback for seeking */
static void
seek_cb(AppData *appdata,
	int seconds) {

  decoder_seek(appdata->decoder, seconds * 1000);

}


/* callback for volume changes */
static void
volume_cb(AppData *appdata,
	  int volume) {

  decoder_set_volume(appdata->decoder, volume);

}



int
main(int argc,
     char *argv[]) {

  osso_context_t *osso_context;
  AppData *appdata = g_new(AppData, 1);


  gtk_init(&argc, &argv);

  /* register OSSO service */
  osso_context = osso_initialize(SERVICE_NAME, VERSION, TRUE, NULL);
  if (! osso_context)
    fprintf(stderr, "Could not initialize OSSO.");

  appdata->decoder = decoder_new();
  appdata->stream = NULL;

  /* setup GUI callbacks */
  appdata->gui = gui_new();
  gui_set_open_cb(appdata->gui, open_cb, (void *) appdata);
  gui_set_volume_cb(appdata->gui, volume_cb, (void *) appdata);
  gui_set_seek_cb(appdata->gui, seek_cb, (void *) appdata);

  g_timeout_add(500, (GSourceFunc) status_cb, appdata);

  gui_run();


  return 0;

}
