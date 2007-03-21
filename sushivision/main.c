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

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mc = PTHREAD_COND_INITIALIZER;
sig_atomic_t _sv_exiting=0;
static int wake_pending = 0;
static int num_threads;

static int instances=0;
static sv_instance_t **instance_list;

void _sv_wake_workers(){
  if(instances){
    pthread_mutex_lock(&m);
    wake_pending = num_threads;
    pthread_cond_broadcast(&mc);
    pthread_mutex_unlock(&m);
  }
}

void _sv_clean_exit(int sig){
  _sv_exiting = 1;
  _sv_wake_workers();

  //signal(sig,SIG_IGN);
  if(sig!=SIGINT)
    fprintf(stderr,
            "\nTrapped signal %d; exiting!\n",sig);

  gtk_main_quit();
}

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
  /* set up temporary working space for function rendering; this saves
     continuously recreating it in the loop below */
  _sv_bythread_cache_t **c; // [instance][panel]
  int i,j;
  
  c = calloc(instances,sizeof(*c));
  for(j=0;j<instances;j++){
    sv_instance_t *s = instance_list[j];
    c[j] = calloc(s->panels,sizeof(**c));
  }

  while(1){
    if(_sv_exiting)break;
    
    // look for work
    {
      int flag=0;
      // by instance
      for(j=0;j<instances;j++){
	sv_instance_t *s = instance_list[j];
				 
	for(i=0;i<s->panels;i++){
	  sv_panel_t *p = s->panel_list[i];

	  if(_sv_exiting)break;

	  // pending remap work?
	  gdk_threads_enter();
	  if(p && p->private && p->private->realized && p->private->graph){

	    // pending computation work?
	    if(p->private->plot_active){
	      _sv_spinner_set_busy(p->private->spinner);
	      flag |= p->private->compute_action(p,&c[j][i]); // may drop lock internally
	    }
	    
	    if(p->private->map_active){
	      int ret = 1;
	      while(ret){ // favor completing remaps over other ops
		_sv_spinner_set_busy(p->private->spinner);
		flag |= ret = p->private->map_action(p,&c[j][i]); // may drop lock internally
		if(!p->private->map_active)
		  _sv_map_set_throttle_time(p);
	      }
	    }
	    
	    // pending legend work?
	    if(p->private->legend_active){
	      _sv_spinner_set_busy(p->private->spinner);
	      flag |= p->private->legend_action(p); // may drop lock internally
	    }
	    
	    if(!p->private->plot_active &&
	       !p->private->legend_active &&
	       !p->private->map_active)
	      _sv_spinner_set_idle(p->private->spinner);
	  }
	  gdk_threads_leave ();
	}
      }
      if(flag==1)continue;
    }
    
    // nothing to do, wait
    pthread_mutex_lock(&m);
    if(!wake_pending)
      pthread_cond_wait(&mc,&m);
    else
      wake_pending--;
    pthread_mutex_unlock(&m);
  }
  
  pthread_mutex_unlock(&m);
  return 0;
}

static char * gtkrc_string(){
  return _SUSHI_GTKRC_STRING;
}

static void _sv_instance_realize(sv_instance_t *s){
  int i;
  for(i=0;i<s->panels;i++)
    _sv_panel_realize(s->panel_list[i]);
  for(i=0;i<s->panels;i++)
    if(s->panel_list[i])
      s->panel_list[i]->private->request_compute(s->panel_list[i]);
}

static void _sv_realize_all(void){
  int i;
  for(i=0;i<instances;i++)
    _sv_instance_realize(instance_list[i]);
}

/* externally visible interface */

sv_instance_t *sv_new(int number, char *name) {
  sv_instance_t *ret;

  if(number<0){
    fprintf(stderr,"Instance number must be >= 0\n");
    errno = -EINVAL;
    return NULL;
  }
  
  if(number<instances){
    if(instance_list[number]!=NULL){
      fprintf(stderr,"Instance number %d already exists\n",number);
      errno = -EINVAL;
      return NULL;
    }
  }else{
    if(instances == 0){
      instance_list = calloc (number+1,sizeof(*instance_list));
    }else{
      instance_list = realloc (instance_list,(number+1) * sizeof(*instance_list));
      memset(instance_list + instances, 0, sizeof(*instance_list)*(number+1 - instances));
    }
    instances = number+1; 
  }

  ret = instance_list[number] = calloc(1, sizeof(**instance_list));
  ret->private = calloc(1,sizeof(*ret->private));
  if(name)
    ret->name = strdup(name);

  return ret;
}

char *_sv_appname = NULL;
char *_sv_filename = NULL;
char *_sv_filebase = NULL;
char *_sv_dirname = NULL;
char *_sv_cwdname = NULL;

int main (int argc, char *argv[]){
  int ret;

  num_threads = num_proccies();

  _gtk_mutex_fixup();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  gdk_threads_init ();
  gtk_rc_parse_string(gtkrc_string());
  gtk_rc_add_default_file("sushi-gtkrc");

  ret = sv_submain(argc,argv);
  if(ret)return ret;
  
  gdk_threads_enter();
  _sv_realize_all();
  _gtk_button3_fixup();
  
  _sv_appname = g_get_prgname ();
  _sv_cwdname = getcwd(NULL,0);
  _sv_dirname = strdup(_sv_cwdname);
  if(argc>1){
    // file to load specified on commandline
    if(argv[argc-1][0] != '-'){
      _sv_filebase = strdup(argv[argc-1]);
      char *base = strrchr(_sv_filebase,'/');
      
      // filebase may include a path; pull it off and apply it toward dirname
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
  }

  if(!_sv_filename || _sv_main_load()){
    if(_sv_appname){
      char *base = strrchr(_sv_appname,'/');
      if(!base) 
	base = _sv_appname;
      else
	base++;

      asprintf(&_sv_filebase, "%s.sushi",base);
    }else
      _sv_filebase = strdup("default.sushi");
    asprintf(&_sv_filename,"%s/%s",_sv_dirname,_sv_filebase);
  }

  {
    pthread_t dummy;
    int threads = num_threads;
    while(threads--)
      pthread_create(&dummy, NULL, &worker_thread,NULL);
  }

  signal(SIGINT,_sv_clean_exit);
  //signal(SIGSEGV,_sv_clean_exit);


  gtk_main ();
  gdk_threads_leave();
  
  {
    int (*optional_exit)(void) = dlsym(RTLD_DEFAULT, "sv_atexit");
    if(optional_exit)
      return optional_exit();
  }

  return 0;
}

static int _sv_instance_save(sv_instance_t *s, xmlNodePtr root){
  if(!s) return 0;
  int i, ret=0;

  xmlNodePtr instance = xmlNewChild(root, NULL, (xmlChar *) "instance", NULL);

  _xmlNewPropI(instance, "number", s->number);
  _xmlNewPropS(instance, "name", s->name);
  
  // dimension values are independent of panel
  for(i=0;i<s->dimensions;i++)
    ret|=_sv_dim_save(s->dimension_list[i], instance);
  
  // objectives have no independent settings

  // panel settings (by panel)
  for(i=0;i<s->panels;i++)
    ret|=_sv_panel_save(s->panel_list[i], instance);

  return ret;
}

void _sv_first_load_warning(int *warn){
  if(!*warn)
    fprintf(stderr,"\nWARNING: The data file to be opened is not a perfect match to\n"
	    "%s.\n\nThis may be because the file is for a different version of\n"
	    "the executable, or because it is is not a save file for \n%s at all.\n\n"
	    "Specific warnings follow:\n\n",_sv_appname,_sv_appname);
  *warn = 1;
}

static int _sv_instance_load(sv_instance_t *s, xmlNodePtr in, int warn){
  int i;

  // piggyback off undo (as it already goes through the trouble of
  // doing correct unrolling, which can be tricky)

  // if this instance has an undo stack, pop it all, then log current state into it
  s->private->undo_level=0;
  _sv_undo_log(s);

  _sv_instance_undo_t *u = s->private->undo_stack[s->private->undo_level];

  // load dimensions
  for(i=0;i<s->dimensions;i++){
    sv_dim_t *d = s->dimension_list[i];
    if(d){
      xmlNodePtr dn = _xmlGetChildI(in,"dimension","number",d->number);
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
  for(i=0;i<s->panels;i++){
    sv_panel_t *p = s->panel_list[i];
    if(p){
      xmlNodePtr pn = _xmlGetChildI(in,"panel","number",p->number);
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
  xmlNodePtr node = in->xmlChildrenNode;
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
  _sv_undo_suspend(s);
  _sv_undo_restore(s);
  _sv_undo_resume(s);

  return warn;
}

int _sv_main_save(){
  xmlDocPtr doc = NULL;
  xmlNodePtr root_node = NULL;
  int i, ret=0;

  LIBXML_TEST_VERSION;

  doc = xmlNewDoc((xmlChar *)"1.0");
  root_node = xmlNewNode(NULL, (xmlChar *)_sv_appname);
  xmlDocSetRootElement(doc, root_node);

  // save each instance 
  for(i=0;i<instances;i++)
    ret |= _sv_instance_save(instance_list[i],root_node);

  xmlSaveFormatFileEnc(_sv_filename, doc, "UTF-8", 1);

  xmlFreeDoc(doc);
  xmlCleanupParser();

  return ret;
}

int _sv_main_load(){
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
    return 1;
  }

  root = xmlDocGetRootElement(doc);

  // load each instance 
  for(i=0;i<instances;i++){
    sv_instance_t *s = instance_list[i];
    if(s){
      xmlNodePtr in = _xmlGetChildI(root,"instance","number",s->number);
      if(!in){ 
	_sv_first_load_warning(&warn);
	fprintf(stderr,"Could not find data for instance \"%s\" in save file.\n",
		s->name);
      }
      // load even if no node; need to set fallbacks
      warn |= _sv_instance_load(s,in,warn);
      if(in)xmlFreeNode(in);
    }
  }

  // if any instances are unclaimed, warn 
  xmlNodePtr node = root->xmlChildrenNode;
  
  while(node){
    if (node->type == XML_ELEMENT_NODE) {
      char *name = NULL;
      _xmlGetPropS(node, "name", &name);
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

  xmlFreeDoc(doc);
  xmlCleanupParser();
  
  return 0;
}
