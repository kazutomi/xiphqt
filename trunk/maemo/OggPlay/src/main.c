/*
 *    Ogg Vorbis player for the Nokia 770
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


#include "version.h"
#include "gui.h"
#include "playlist.h"
#include "decoder.h"
#include "stream.h"
#include <libosso.h>
#include <unistd.h>



enum RunMode { GUI, CMDLINE };


struct _AppData {

  enum RunMode runmode;
  Playlist *playlist;
  Decoder *decoder;
  Stream *stream;
  Gui *gui;
  int current_position;

};
typedef struct _AppData AppData;



/* Looks up a freedesktop.org compliant cover for the given path. */
char *
lookup_cover(const char *path) {

  GKeyFile *keyfile;
  char *keyfilepath;
  char *value;
  char *coverpath;

  /* build path to the .directory desktop file */
  keyfilepath = g_build_path("/", path, ".directory", NULL);

  if (g_file_test(keyfilepath, G_FILE_TEST_EXISTS)) {
  
    /* read desktop file */
    keyfile = g_key_file_new();
    g_key_file_load_from_file(keyfile, keyfilepath, G_KEY_FILE_NONE, NULL);

    /* read icon entry */
    value = g_key_file_get_string(keyfile, "Desktop Entry", "Icon", NULL);

    if (g_path_is_absolute(value))
      coverpath = g_strdup(value);
    else
      coverpath = g_build_path("/", path, value, NULL);

    g_free(value);
    g_key_file_free(keyfile);

  } else {

    coverpath = NULL;

  }

  g_free(keyfilepath);  

  return coverpath;
  
}


void
open_uri(AppData *appdata,
	 const char *uri) {

  char *dirname;
  char *cover;

  /* close old stream */
  if (appdata->stream)
    decoder_close_stream(appdata->decoder);

  /* open new stream */
  appdata->stream = stream_new_from_uri(uri);
  if (! decoder_open_stream(appdata->decoder, appdata->stream)) {
    appdata->stream = NULL;
    gui_show_error(appdata->gui, "Could not open file");
  } else {
    decoder_play(appdata->decoder);
  }

  /* look for cover art */
  dirname = g_path_get_dirname(uri);
  cover = lookup_cover(dirname);
  gui_load_cover(appdata->gui, cover);
  g_free(dirname);
  g_free(cover);

  /* set song label */
  gui_set_title(appdata->gui,
		appdata->decoder->tag_title,
		appdata->decoder->tag_artist,
		appdata->decoder->tag_album);

}




/* callback for updating status information */
static int
status_cb(AppData *appdata) {

  int p, t;

  if (! appdata->stream) {

    return TRUE;

  }
  
  else if (decoder_is_playing(appdata->decoder)) {

    p = decoder_get_position(appdata->decoder);
    t = decoder_get_total(appdata->decoder);
    gui_set_time(appdata->gui, p, t);

  } else if (decoder_has_finished(appdata->decoder)) {

    playlist_next(appdata->playlist);

  }

  return TRUE;

}


/* callback for playing URIs */
static void
play_cb(AppData *appdata,
	const char *uri) {

  if (uri) {
    open_uri(appdata, uri);
  } else {
    decoder_stop(appdata->decoder);
  }

}



/* callback for opening a file */
static void
open_cb(AppData *appdata,
	GSList *filenames) {

  GSList *iter;

  for (iter = filenames; iter != NULL; iter = iter->next) {
    playlist_append(appdata->playlist, (char *) iter->data);
  }
  
  //open_uri(appdata, uri);

}


/* callback for seeking */
static void
seek_cb(AppData *appdata,
	int seconds) {

  decoder_seek(appdata->decoder, seconds * 1000);
  decoder_play(appdata->decoder);
  gui_set_paused(appdata->gui, FALSE);

}


/* callback for volume changes */
static void
volume_cb(AppData *appdata,
	  int volume) {

  decoder_set_volume(appdata->decoder, volume);

}


/* callback for control buttons */
static void
control_cb(AppData *appdata,
	   int command) {

  switch (command) {
  case (PLAY):
    if (decoder_is_playing(appdata->decoder)) {
      decoder_stop(appdata->decoder);
      gui_set_paused(appdata->gui, TRUE);
    } else {
      decoder_play(appdata->decoder);
      gui_set_paused(appdata->gui, FALSE);
    }
    break;

  case (STOP):
    decoder_stop(appdata->decoder);
    decoder_seek(appdata->decoder, 0);
    appdata->current_position = decoder_get_position(appdata->decoder);
    gui_set_time(appdata->gui, 0, decoder_get_total(appdata->decoder));
    gui_set_paused(appdata->gui, FALSE);
    break;

  case (PREVIOUS):
    playlist_previous(appdata->playlist);
    break;

  case (NEXT):
    decoder_stop(appdata->decoder);
    playlist_next(appdata->playlist);
    break;

  case (CLEAR):
    playlist_clear(appdata->playlist);

  default:
    break;
  }

}




void
gui_mode(int argc,
	 char *argv[]) {

  osso_context_t *osso_context;
  char *versionstring;
  AppData *appdata = g_new(AppData, 1);
  appdata->runmode = GUI;


  gtk_init(&argc, &argv);

  /* register OSSO service */
  osso_context = osso_initialize(NAME, VERSION, TRUE, NULL);
  if (! osso_context)
    fprintf(stderr, "Could not initialize OSSO.");

  appdata->playlist = playlist_new();
  appdata->decoder = decoder_new();
  appdata->stream = NULL;
  appdata->gui = gui_new(appdata->playlist);

  versionstring = g_strconcat(FULLNAME, " ", VERSION, NULL);
  gui_set_title(appdata->gui, "", versionstring, "");
  g_free(versionstring);

  /* setup callbacks */
  playlist_set_play_cb(appdata->playlist, play_cb, (void *) appdata);

  /* setup GUI callbacks */
  gui_set_open_cb(appdata->gui, open_cb, (void *) appdata);
  gui_set_volume_cb(appdata->gui, volume_cb, (void *) appdata);
  gui_set_seek_cb(appdata->gui, seek_cb, (void *) appdata);
  gui_set_control_cb(appdata->gui, control_cb, (void *) appdata);

  g_timeout_add(500, (GSourceFunc) status_cb, appdata);

  gui_run();

}


void
cmdline_mode(int argc,
	     char *argv[]) {

  int p, t;

  AppData *appdata = g_new(AppData, 1);
  appdata->runmode = CMDLINE;
  appdata->playlist = playlist_new();
  appdata->decoder = decoder_new();
  appdata->stream = NULL;


  /* setup callbacks */
  playlist_set_play_cb(appdata->playlist, play_cb, (void *) appdata);
  playlist_next(appdata->playlist);

  while (TRUE) {
    if (decoder_is_playing(appdata->decoder)) {
      
      p = decoder_get_position(appdata->decoder);
      t = decoder_get_total(appdata->decoder);
      printf("%s: %d / %d\n", appdata->decoder->tag_title, p, t);

    } else if (decoder_has_finished(appdata->decoder)) {

      playlist_next(appdata->playlist);
    }

    usleep(500);
  }

}



int
main(int argc,
     char *argv[]) {

  gui_mode(argc, argv);

  return 0;

}
