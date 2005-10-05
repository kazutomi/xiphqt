/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo/cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include "version.h"
#include "graph.h"
#include "gameboard.h"
#include "levelstate.h"
#include "main.h"

#define boardstate "/.gPlanarity/boards/"
#define mainstate "/.gPlanarity/"

char *boarddir;
char *statedir;
Gameboard *gameboard;
GtkWidget *toplevel_window;
graph maingraph;

char *version = "";
static cairo_font_face_t *font_face=0;
static cairo_font_face_t *bold_face=0;
static cairo_font_face_t *ital_face=0;

static float adjust_x_normal;
static float adjust_y_normal;
static float adjust_x_bold;
static float adjust_y_bold;

static int dir_create(char *name){
  if(mkdir(name,0700)){
    switch(errno){
    case EEXIST:
      // this is ok
      return 0;
    default:
      fprintf(stderr,"ERROR:  Could not create directory (%s) to save game state:\n\t%s\n",
	      name,strerror(errno));
      return errno;
    }
  }
  return 0;
}     

void request_resize(int width, int height){
  GtkWidget *w = GTK_WIDGET(gameboard);

  gtk_window_resize(GTK_WINDOW(toplevel_window),width,height);

  // the toplevel resize *could* fail, for example, if the
  // windowmanager has forced 'maximize'.  In this case, our graph
  // size is set to what it wanted to be, but the gameboard window size
  // is unchanged.  Force the graph to resize itself to the window in
  // this case.

  if(w->allocation.width != width ||
     w->allocation.height != height)
    gameboard_size_allocate (GTK_WIDGET(gameboard),&w->allocation);

}

static void clean_exit(int sig){
  signal(sig,SIG_IGN);
  if(sig!=SIGINT)
    fprintf(stderr,
            "\nTrapped signal %d; saving state and exiting!\n",sig);

  levelstate_write(statedir);
  gtk_main_quit();
  exit(0);
}

static void clean_exit_delete_post(Gameboard *g){
  gameboard = 0;
  gtk_main_quit();
}

static gint clean_exit_delete(gpointer p){
  levelstate_write(statedir);
  undeploy_buttons(gameboard,clean_exit_delete_post);
  return 1;  // we're handling it, don't continue the delete chain
}

/* font handling... still a pain in the ass!  ...but mostly due to
   lack of documentation.  

   Try first to find one of the default fonts we prefer (FontConfig
   can search for a list, Cairo cannot).

   Regardless of what's found, try to determine the approximate best
   aspect ratio of the text; gPlanarity looks best with a relatively
   tall/narrow font that renders with horizontal/vertical strokes of
   approximately identical width */

static cairo_font_face_t *init_font(char *list, int slant,int bold){
  cairo_font_face_t *ret;
  char *fontface;
  FcPattern *fc_pattern;
  FcBool scalable;
  
  fc_pattern = FcNameParse(list);

  // load the system config
  FcConfigSubstitute(0, fc_pattern, FcMatchPattern);

  FcPatternDel(fc_pattern,FC_SLANT);
  FcPatternDel(fc_pattern,FC_WEIGHT);
  if(slant)
    FcPatternAddInteger(fc_pattern,FC_SLANT,FC_SLANT_ITALIC);
  if(bold)
    FcPatternAddInteger(fc_pattern,FC_WEIGHT,FC_WEIGHT_BOLD);

  // fill in missing defaults
  FcDefaultSubstitute(fc_pattern);
  // find a font face on our list if possible
  fc_pattern = FcFontMatch(0, fc_pattern, 0);

  if(!fc_pattern){
    fprintf(stderr,"\nUnable to find any suitable %s fonts!\n"
	    "Continuing, but the the results are likely to be poor.\n\n",

	    (slant?(bold?"bold italic":"italic"):(bold?"bold":"medium")) );
  }

  FcPatternGetString (fc_pattern, FC_FAMILY, 0, (FcChar8 **)&fontface);

  /*
  if(!strstr(list,fontface)){
    fprintf(stderr,"\nUnable to find any of gPlanarity's preferred %s fonts (%s);\n"
	    "Using system default font \"%s\".\n\n",
	    (slant?(bold?"bold italic":"italic"):(bold?"bold":"medium")),
	    list,fontface);
	    }*/

  FcPatternGetBool(fc_pattern, FC_SCALABLE, 0, &scalable);
  if (scalable != FcTrue) {
    fprintf(stderr,"\nSelected %s font \"%s\" is not scalable!  This is almost as bad\n"
	    "as not finding any font at all.  Continuing, but this may look\n"
	    "very poor indeed.\n\n",
	    (slant?(bold?"bold italic":"italic"):(bold?"bold":"medium")), fontface);
  }
  
  /* Set the hinting behavior we want; only the autohinter is giving
     decent results across a wide range of sizes and face options.
     Don't obey the system config with respect to hinting; every one
     of my systems installed a default config that resulted in the
     available fonts looking like ass.  Bitstream Vera Sans especially
     looks bad without the autohinter, and that's the font most
     systems will have as the only sans default. */
  
  // Must remove the preexisting settings!  The settings are a list,
  // and we're tacking ours on the end; Cairo only inspects first hit
  FcPatternDel(fc_pattern,FC_HINT_STYLE);
  FcPatternDel(fc_pattern,FC_HINTING);
  FcPatternDel(fc_pattern,FC_AUTOHINT);

  // Add ours requesting autohint
  FcPatternAddBool(fc_pattern,FC_HINTING,FcTrue);
  FcPatternAddBool(fc_pattern,FC_AUTOHINT,FcTrue);

  // Make the font face
  ret = cairo_ft_font_face_create_for_pattern(fc_pattern);
  FcPatternDestroy(fc_pattern);
  return ret;
}

static void init_fonts(char *list){

  font_face = init_font(list,0,0);
  bold_face = init_font(list,0,1);
  ital_face = init_font(list,1,0);

  // We've done the best we can, font-choice-wise.  Determine an
  // aspect ratio correction for whatever font we've got.  Arial is
  // declared to be the ideal aspect, correct other fonts to the same
  // rough proportions (it looks better).  This is a rough hack, but
  // yields an unmistakable improvement.

  {
    cairo_text_extents_t ex;
    char *test_string = "Practice Before the Handbasket: Three of Three";
    cairo_surface_t *s = 
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32,16,16);
    cairo_t *c = cairo_create(s);
    cairo_matrix_t m;
    
    cairo_set_font_face (c, font_face);
    cairo_matrix_init_scale (&m, 50.,50.);
    cairo_set_font_matrix (c,&m);
    cairo_text_extents (c, test_string, &ex);
    adjust_x_normal = 1058 / ex.width;
    adjust_y_normal = -37 / ex.y_bearing;


    cairo_set_font_face (c, bold_face);
    cairo_text_extents (c, test_string, &ex);
    adjust_x_bold = 1130 / ex.width;
    adjust_y_bold = -37 / ex.y_bearing;


    cairo_destroy(c);
    cairo_surface_destroy(s);
  }
}

void set_font (cairo_t *c, float w, float h, int slant, int bold){
  cairo_matrix_t m;

  if(slant){
    cairo_set_font_face(c,ital_face);
    cairo_matrix_init_scale (&m, rint(w*adjust_x_normal),rint(h*adjust_y_normal));
  }else if (bold){
    cairo_set_font_face(c,bold_face);
    cairo_matrix_init_scale (&m, rint(w*adjust_x_bold),rint(h*adjust_y_bold));
  }else{
    cairo_set_font_face(c,font_face);
    cairo_matrix_init_scale (&m, rint(w*adjust_x_normal),rint(h*adjust_y_normal));
  }
  cairo_set_font_matrix (c,&m);

}

int main(int argc, char *argv[]){
  char *homedir = getenv("home");
  if(!homedir)
    homedir = getenv("HOME");
  if(!homedir)
    homedir = getenv("homedir");
  if(!homedir)
    homedir = getenv("HOMEDIR");
  if(!homedir){
    fprintf(stderr,"No homedir environment variable set!  gPlanarity will be\n"
	    "unable to permanently save any progress or board state.\n");
    boarddir=NULL;
    statedir=NULL;
  }else{
    boarddir=calloc(strlen(homedir)+strlen(boardstate)+1,1);
    strcat(boarddir,homedir);
    strcat(boarddir,boardstate);

    statedir=calloc(strlen(homedir)+strlen(mainstate)+1,1);
    strcat(statedir,homedir);
    strcat(statedir,mainstate);

    dir_create(statedir);
    dir_create(boarddir);
  }

  version=strstr(VERSION,"version.h");
  if(version){
    char *versionend=strchr(version,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend)versionend=strchr(versionend+1,' ');
    if(versionend){
      int len=versionend-version-9;
      version=strdup(version+10);
      version[len-1]=0;
    }
  }else{
    version="";
  }

  init_fonts("");

  gtk_init (&argc, &argv);

  toplevel_window   = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (toplevel_window), "delete-event",
                    G_CALLBACK (clean_exit_delete), NULL);
  
  gameboard = gameboard_new();
  levelstate_read();

  gtk_container_add (GTK_CONTAINER (toplevel_window), GTK_WIDGET(gameboard));
  gtk_widget_show_all (toplevel_window);
  memset(&maingraph,0,sizeof(maingraph));

  /* get the setup processed before we fire up animations */
  while (gtk_events_pending ())
    gtk_main_iteration ();
  gdk_flush();

  levelstate_resume();
  signal(SIGINT,clean_exit);

  //signal(SIGSEGV,clean_exit); /* would be a bad idea; corrupt state
  //could prevent us from restarting */

  gtk_main ();

  if(gameboard !=0 )
    levelstate_write();

  cairo_font_face_destroy(font_face);
  cairo_font_face_destroy(bold_face);
  cairo_font_face_destroy(ital_face);

  return 0;
}
