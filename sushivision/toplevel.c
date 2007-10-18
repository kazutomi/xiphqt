/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include <pthread.h>
#include <dlfcn.h>
#include "internal.h"
#include "sushi-gtkrc.h"

// All API and GTK/GDK access is locked by a single recursive mutex
// installed into GDK.

// Worker threads exist to handle high latency tasks asynchronously.
// The worker threads (with one exception) never touch the main GDK
// mutex; any data protected by the main mutex needed for a worker
// task is pulled out and encapsulated when that task is set up from a
// GTK or API call.  The one exception to worker threads and the GDK
// lock is expose requests after rendering; in this case, the worker
// waits until it can drop all locks, does so, and then issues an
// expose request locked by GDK.

// All data object memory strutures that are used by the worker
// threads and can be manipulated by API/GTK/GDK and the worker
// threads re locked by r/w locks; any thread 'inside' the abstraction
// read-locks the memory while it is in use.  GDK/API access
// write-locks this memory to manipulate it (allocation, deallocation,
// structural mutation).

// lock acquisition order must move to the right:
// GDK -> panel_m -> status_m -> plot_m

// mutex condm is only for protecting the worker condvar
static pthread_mutex_t worker_condm = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t worker_cond = PTHREAD_COND_INITIALIZER;
static sig_atomic_t sv_exiting=0;
static int wake_pending = 0;
static int num_threads;

static int num_proccies(){
  FILE *f = fopen("/proc/cpuinfo","r");
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  int num=1,arg;

  if (f == NULL) return 1;
  while ((read = getline(&line, &len, f)) != -1) {
    if(sscanf(line,"processor : %d",&arg)==1)
      if(arg+1>num)num=arg+1;
  }
  if (line)
    free(line);

  fprintf(stderr,"Number of processors: %d\n",num);
  fclose(f);
  return num;
}

static void *worker_thread(void *dummy){
  while(1){
    int i,flag=0;
    if(sv_exiting)break;

    pthread_rwlock_rdlock(panellist_m);
    for(i=0;i<_sv_panels;i++){
      sv_panel_t *p = _sv_panel_list[i];
      
      if(sv_exiting)break;
	
      if(p){
	int ret = _sv_panel_work(p);
	if(ret == STATUS_WORKING){
	  flag = 1;
	  sv_wake(); // result of this completion might have
	             // generated more work
	}
      }
    }
    if(flag==1)continue;
    
    // nothing to do, wait
    pthread_mutex_lock(&worker_condm);
    while(!wake_pending)
      pthread_cond_wait(&worker_cond,&worker_condm);

    wake_pending--;
    pthread_mutex_unlock(&worker_condm);
  }
  
  pthread_mutex_unlock(&worker_condm);
  return 0;
}

static char *gtkrc_string(){
  return _SUSHI_GTKRC_STRING;
}

static void _sv_realize_all(void){
  int i;
  for(i=0;i<_sv_panels;i++)
    _sv_panel_realize(_sv_panel_list[i]);
  for(i=0;i<_sv_panels;i++)
    if(_sv_panel_list[i])
      _sv_panel_list[i]->private->request_compute(_sv_panel_list[i]);
}

char *_sv_appname = NULL;
char *_sv_filename = NULL;
char *_sv_filebase = NULL;
char *_sv_dirname = NULL;
char *_sv_cwdname = NULL;

static void *eventloop(void *dummy){

  gdk_lock();
  gtk_main ();
  gdk_unlock();
  
  // in case there's another mainloop in the main app
  gdk_lock();
  if(!gtk_main_iteration_do(FALSE)) // side effect: returns true if
				    // there are no main loops active
    gtk_main_quit();
  gdk_unlock();
  
  return 0;
}

/* externally visible interface */
int sv_init(void){
  int ret=0;
  if((ret=pthread_key_create(&_sv_dim_key,NULL)))
    return ret;
  if((ret=pthread_key_create(&_sv_obj_key,NULL)))
    return ret;
  if((ret=pthread_key_create(&_sv_panel_key,NULL)))
    return ret;
  
  num_threads = ((num_proccies()*3)>>2);

  _gtk_mutex_fixup();
  gtk_init (NULL,NULL);

  _sv_appname = g_get_prgname ();
  _sv_cwdname = getcwd(NULL,0);
  _sv_dirname = strdup(_sv_cwdname);

  if(_sv_appname){
    char *base = strrchr(_sv_appname,'/');
    if(!base) 
      base = _sv_appname;
    else
      base++;

    asprintf(&_sv_filebase, "%s.sushi",base);
  }else
    _sv_filebase = strdup("unnamed.sushi");
  asprintf(&_sv_filename,"%s/%s",_sv_dirname,_sv_filebase);

  if (!g_thread_supported ()) g_thread_init (NULL);
  gdk_threads_init ();

  gtk_rc_parse_string(gtkrc_string());
  gtk_rc_add_default_file("sushi-gtkrc");

  _gtk_button3_fixup();

  // worker threads
  {
    pthread_t dummy;
    int threads = num_threads;
    while(threads--)
      pthread_create(&dummy, NULL, &worker_thread,NULL);
  }

  // event thread for panels in the event the app we're injected into
  // has no gtk main loop
  {
    pthread_t dummy;
    return pthread_create(&dummy, NULL, &event_thread,NULL);
  }

  return 0;
}

int sv_join(void){
  while(!sv_exiting){
    pthread_mutex_lock(&worker_condm);
    pthread_cond_wait(&worker_cond,&worker_condm);
    pthread_mutex_unlock(&worker_condm);
  }
  return 0;
}

int sv_wake(void){
  pthread_mutex_lock(&worker_condm);
  wake_pending = num_threads;
  pthread_cond_broadcast(&worker_cond);
  pthread_mutex_unlock(&worker_condm);
  return 0;
}

int sv_exit(void){
  sv_exiting = 1;
  sv_wake();

  gdk_threads_enter();
  if(!gtk_main_iteration_do(FALSE)) // side effect: returns true if
				    // there are no main loops active
    gtk_main_quit();
  
  gdk_threads_leave();
  return 0;
}

void _sv_first_load_warning(int *warn){
  if(!*warn)
    fprintf(stderr,"\nWARNING: The data file to be opened is not a perfect match to\n"
	    "%s.\n\nThis may be because the file is for a different version of\n"
	    "the executable, or because it is is not a save file for \n%s at all.\n\n"
	    "Specific warnings follow:\n\n",_sv_appname,_sv_appname);
  *warn = 1;
}

static void set_internal_filename(char *filename){
  // save the filename for internal menu seeding purposes
  char *base = strrchr(filename,'/');
  
  // filename may include a path; pull it off and apply it toward dirname
  if(base){
    base[0] = '\0';
    char *dirbit = strdup(_sv_filebase);
    _sv_filebase = base+1;
    if(g_path_is_absolute(dirbit)){
      // replace dirname
      free(_sv_dirname);
      _sv_dirname = dirbit;
    }else{
      // append to dirname
      char *buf;
      asprintf(&buf,"%s/%s",_sv_dirname,dirbit);
      free(_sv_dirname);
      _sv_dirname = buf;
    }
  }
  asprintf(&_sv_filename,"%s/%s",_sv_dirname,_sv_filebase);
}

int sv_save(char *filename){
  xmlDocPtr doc = NULL;
  xmlNodePtr root_node = NULL;
  int i, ret=0;

  LIBXML_TEST_VERSION;

  doc = xmlNewDoc((xmlChar *)"1.0");
  root_node = xmlNewNode(NULL, (xmlChar *)_sv_appname);
  xmlDocSetRootElement(doc, root_node);

  // dimension values are independent of panel
  for(i=0;i<_sv_dimensions;i++)
    ret|=_sv_dim_save(_sv_dimension_list[i], root_node);
  
  // objectives have no independent settings

  // panel settings (by panel)
  for(i=0;i<_sv_panels;i++)
    ret|=_sv_panel_save(_sv_panel_list[i], root_node);

  ret|=xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

  if(ret==0) set_internal_filename(filename);

  xmlFreeDoc(doc);
  xmlCleanupParser();

  return ret;
}

int sv_load(filename){
  xmlDoc *doc = NULL;
  xmlNode *root = NULL;
  int fd,warn=0;
  int i;

  LIBXML_TEST_VERSION;

  fd = open(_sv_filename, O_RDONLY);
  if(fd<0){
    GtkWidget *dialog = gtk_message_dialog_new (NULL,0,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						"Error opening file '%s': %s",
						_sv_filename, strerror (errno));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return 1;
  }

  doc = xmlReadFd(fd, NULL, NULL, 0);
  close(fd);

  if (doc == NULL) {
    GtkWidget *dialog = gtk_message_dialog_new (NULL,0,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						"Error parsing file '%s'",
						_sv_filename);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    errno = -EINVAL;
    return 1;
  }

  root = xmlDocGetRootElement(doc);

  // piggyback off undo (as it already goes through the trouble of
  // doing correct unrolling, which can be tricky)
  
  // if this instance has an undo stack, pop it all, then log current state into it
  _sv_undo_level=0;
  _sv_undo_log();
  
  _sv_undo_t *u = _sv_undo_stack[_sv_undo_level];

  // load dimensions
  for(i=0;i<_sv_dimensions;i++){
    sv_dim_t *d = _sv_dimension_list[i];
    if(d){
      xmlNodePtr dn = _xmlGetChildI(root,"dimension","number",d->number);
      if(!dn){
	_sv_first_load_warning(&warn);
	fprintf(stderr,"Could not find data for dimension \"%s\" in save file.\n",
		d->name);
      }else{
	warn |= _sv_dim_load(d,u,dn,warn);
	xmlFreeNode(dn);
      }
    }
  }
  
  // load panels
  for(i=0;i<_sv_panels;i++){
    sv_panel_t *p = _sv_panel_list[i];
    if(p){
      xmlNodePtr pn = _xmlGetChildI(root,"panel","number",p->number);
      if(!pn){ 
	_sv_first_load_warning(&warn);
	fprintf(stderr,"Could not find data for panel \"%s\" in save file.\n",
		p->name);
      }else{
	warn |= _sv_panel_load(p,u->panels+i,pn,warn);
	xmlFreeNode(pn);
      }
    }
  }
  
  // if any elements are unclaimed, warn 
  xmlNodePtr node = root->xmlChildrenNode;
  while(node){
    if (node->type == XML_ELEMENT_NODE) {
      xmlChar *name = xmlGetProp(node, (xmlChar *)"name");
      _sv_first_load_warning(&warn);
      if(name){
	fprintf(stderr,"Save file contains data for nonexistant object \"%s\".\n",
		name);
	xmlFree(name);
      }else
	fprintf(stderr,"Save file root node contains an extra unknown elements.\n");
    }
    node = node->next;
  }
  
  // effect the loaded values
  _sv_undo_suspend();
  _sv_undo_restore();
  _sv_undo_resume();

  xmlFreeDoc(doc);
  xmlCleanupParser();
  
  if(ret==0) set_internal_filename(filename);

  return 0;
}
