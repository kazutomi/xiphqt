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
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
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
  return 0;
}
