/*
 *
 *  gtk2 waveform viewer
 *    
 *      Copyright (C) 2004 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  The analyzer is distributed in the hope that it will be useful,
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

#include "waveform.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "fisharray.h"
#include "io.h"
#include "wave_plot.h"

sig_atomic_t increment_fish=0;

static struct panel {
  GtkWidget *twirlimage;
  GdkPixmap *ff[19];
  GdkBitmap *fb[19];

  GtkAccelGroup *group;
  GtkWidget *toplevel;


  guint fishframe_timer;
  int fishframe_init;
  int fishframe;

  GtkWidget *plot;
  GtkWidget *run;
  GtkWidget **chbuttons;
  GtkWidget *rangemenu;
} p;

float plot_range=0;
int plot_scale=0;
int plot_span=0;
int plot_rchoice=0;
int plot_schoice=0;
int plot_spanchoice=0;
int plot_interval=0;
int plot_trigger=0;
int plot_hold=0;
int plot_type=0;
int plot_last_update=0;
int *active;

int overslice[MAX_FILES]= {-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1};

static void set_slices(int interval, int span){

  /* update interval limited to < 25fps */
  int temp = (interval < 50000 ? 50000:interval),fi;

  pthread_mutex_lock(&ioparam_mutex);
  if(temp <= span){
    /* if the fps-limited update interval is shorter than or equal to
       the span, we simply frame limit */
    for(fi=0;fi<inputs;fi++){
      blockslice[fi]=rint(rate[fi]/1000000.*temp);
      overslice[fi]=rint(rate[fi]/1000000.*temp);
    }
  }else{
    /* if the limited update interval is longer than the span, we
       overdraw */
    for(fi=0;fi<inputs;fi++)
      blockslice[fi]=rint(rate[fi]/1000000.*temp);
    for(fi=0;fi<inputs;fi++){
      overslice[fi]=rint(rate[fi]/1000000.*interval);
    }
  }
  pthread_mutex_unlock(&ioparam_mutex);
}

static void replot(struct panel *p){
  /* update the waveform display; send new data */
  if(!plot_hold){
    pthread_mutex_lock(&feedback_mutex);
    plot_refresh(PLOT(p->plot));
    plot_last_update=feedback_increment;
    pthread_mutex_unlock(&feedback_mutex);
  }
}

static void shutdown(void){
  gtk_main_quit();
}

/* gotta have the Fucking Fish */
static int reanimate_fish(struct panel *p){
  if(process_active || (p->fishframe>0 && p->fishframe<12)){
    /* continue spinning */
    if(increment_fish)p->fishframe++;
    if(p->fishframe>=12)p->fishframe=0;
    
    gtk_image_set_from_pixmap(GTK_IMAGE(p->twirlimage),
			      p->ff[p->fishframe],
			      p->fb[p->fishframe]);
    
    if(p->fishframe==0 && !process_active){
      /* reschedule to blink */
      p->fishframe_timer=
	g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,p);
      return FALSE;
    }

  }else{
    p->fishframe++;
    if(p->fishframe<=1)p->fishframe=12;
    if(p->fishframe>=19)p->fishframe=0;

    gtk_image_set_from_pixmap(GTK_IMAGE(p->twirlimage),
			      p->ff[p->fishframe],
			      p->fb[p->fishframe]);


    if(p->fishframe==12){
      /* reschedule to animate */
      p->fishframe_timer=
	g_timeout_add(10,(GSourceFunc)reanimate_fish,p);
      return FALSE;
    }
    if(p->fishframe==0){
      /* reschedule to blink */
      p->fishframe_timer=
	g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,p);
      return FALSE;
    }
  }
  return TRUE;
}

static void animate_fish(struct panel *p){
  if(p->fishframe_init){
    g_source_remove(p->fishframe_timer);
    p->fishframe_timer=
      g_timeout_add(80,(GSourceFunc)reanimate_fish,p);
  }else{
    p->fishframe_init=1;
    p->fishframe_timer=
      g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,p);
  }
}

static void set_fg(GtkWidget *c, gpointer in){
  GdkColor *rgb = in;
  gtk_widget_modify_fg(c,GTK_STATE_NORMAL,rgb);
  gtk_widget_modify_fg(c,GTK_STATE_ACTIVE,rgb);
  gtk_widget_modify_fg(c,GTK_STATE_PRELIGHT,rgb);
  gtk_widget_modify_fg(c,GTK_STATE_SELECTED,rgb);

  /* buttons usually have internal labels */
  if(GTK_IS_CONTAINER(c))
    gtk_container_forall (GTK_CONTAINER(c),set_fg,in);
}

static int rangechange_ign=0;
static void rangechange(GtkWidget *widget,struct panel *p){
  if(!rangechange_ign){
    int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    plot_rchoice=choice;
    switch(choice){
    case 0:
      plot_range=1;
      break;
    case 1:
      plot_range=.5;
      break;
    case 2:
      plot_range=.2;
      break;
    case 3:
      plot_range=.1;
      break;
    case 4:
      plot_range=.01;
      break;
    case 5:
      plot_range=.001;
      break;
    }
    plot_setting(PLOT(p->plot),plot_range,plot_scale,plot_interval,plot_span,plot_rchoice,plot_schoice,plot_spanchoice,plot_type,NULL,NULL);
  }
}

static void scalechange(GtkWidget *widget,struct panel *p){
  int i;
  int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  plot_schoice=choice;
  switch(choice){
  case 0:
    plot_scale=0;
    break;
  case 1:
    plot_scale=-65;
    break;
  case 2:
    plot_scale=-96;
    break;
  case 3:
    plot_scale=-120;
    break;
  case 4:
    plot_scale=-160;
    break;
  }

  rangechange_ign=1;
  if(choice==0){
    char *entries[]={"1.0",
                     "0.5",
                     "0.2",
                     "0.1",
                     "0.01",
                     "0.001"};
    for(i=0;i<6;i++){
      gtk_combo_box_remove_text (GTK_COMBO_BOX (p->rangemenu), i);
      gtk_combo_box_insert_text (GTK_COMBO_BOX (p->rangemenu), i, entries[i]);
    }

  }else{
    char *entries[]={"0dB",
                     "-6dB",
                     "-14dB",
                     "-20dB",
                     "-40dB",
                     "-60dB"};
    for(i=0;i<6;i++){
      gtk_combo_box_remove_text (GTK_COMBO_BOX (p->rangemenu), i);
      gtk_combo_box_insert_text (GTK_COMBO_BOX (p->rangemenu), i, entries[i]);
    }
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(p->rangemenu),plot_rchoice);
  rangechange_ign=0;

  plot_setting(PLOT(p->plot),plot_range,plot_scale,plot_interval,plot_span,plot_rchoice,plot_schoice,plot_spanchoice,plot_type,NULL,NULL);
}

static void spanchange(GtkWidget *widget,struct panel *p){
  int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  plot_spanchoice=choice;
  switch(choice){
  case 0:
    plot_span=1000000;
    break;
  case 1:
    plot_span=500000;
    break;
  case 2:
    plot_span=200000;
    break;
  case 3:
    plot_span=100000;
    break;
  case 4:
    plot_span=50000;
    break;
  case 5:
    plot_span=20000;
    break;
  case 6:
    plot_span=10000;
    break;
  case 7:
    plot_span=5000;
    break;
  case 8:
    plot_span=2000;
    break;
  case 9:
    plot_span=1000;
    break;
  case 10:
    plot_span=500;
    break;
  case 11:
    plot_span=200;
    break;
  case 12:
    plot_span=100;
    break;
  }

  set_slices(plot_interval,plot_span);

  plot_setting(PLOT(p->plot),plot_range,plot_scale,plot_interval,plot_span,plot_rchoice,plot_schoice,plot_spanchoice,plot_type,blockslice,overslice);
}

/* intervals that are >= the span and would result in > 25fps are overdrawn */
static void intervalchange(GtkWidget *widget,struct panel *p){
  int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  switch(choice){
  case 0:
    plot_interval=1000000;
    break;
  case 1:
    plot_interval=500000;
    break;
  case 2:
    plot_interval=200000;
    break;
  case 3:
    plot_interval=100000;
    break;
  case 4:
    plot_interval=50000; // 20/sec
    break;
  case 5:
    plot_interval=20000; // 50/sec
    break;
  case 6:
    plot_interval=10000;
    break;
  case 7:
    plot_interval=5000;
    break;
  case 8:
    plot_interval=2000;
    break;
  case 9:
    plot_interval=1000;
    break;
  case 10:
    plot_interval=500;
    break;
  case 11:
    plot_interval=200;
    break;
  case 12:
    plot_interval=100;
    break;
  }

  set_slices(plot_interval,plot_span);

  plot_setting(PLOT(p->plot),plot_range,plot_scale,plot_interval,plot_span,plot_rchoice,plot_schoice,plot_spanchoice,plot_type,blockslice,overslice);
}

static void triggerchange(GtkWidget *widget,struct panel *p){
  int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  /* nothing but free-run supported right now */
}

static void runchange(GtkWidget *widget,struct panel *p){
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))){
    if(!process_active){
      pthread_t thread_id;
      process_active=1;
      process_exit=0;
      animate_fish(p);
      pthread_create(&thread_id,NULL,&process_thread,NULL);
    }
  }else{
    process_exit=1;
    while(process_active)sched_yield();
  }
}

static void holdchange(GtkWidget *widget,struct panel *p){
  plot_hold=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  replot(p);
  plot_draw(PLOT(p->plot));
}

static void plotchange(GtkWidget *widget,struct panel *p){
  plot_type=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  plot_setting(PLOT(p->plot),plot_range,plot_scale,plot_interval,plot_span,plot_rchoice,plot_schoice,plot_spanchoice,plot_type,blockslice,overslice);
}

static void loopchange(GtkWidget *widget,struct panel *p){
  acc_loop=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void rewindchange(GtkWidget *widget,struct panel *p){
  acc_rewind=1;
}

static void chlabels(GtkWidget *widget,struct panel *p){
  /* scan state, update labels on channel buttons, set sensitivity
     based on grouping and mode */
  int fi,ch,i;
  char buf[80];

  for(i=0;i<total_ch;i++)
    active[i]=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->chbuttons[i]));

  plot_set_active(PLOT(p->plot),active);
}

extern char *version;
void panel_create(struct panel *panel){
  int i;

  GtkWidget *topplace,*topal,*topalb;

  GtkWidget *topframe=gtk_frame_new (NULL);
  GtkWidget *toplabel=gtk_label_new (NULL);
  GtkWidget *quitbutton=gtk_button_new_with_mnemonic("_quit");
  GtkWidget *mainbox=gtk_hbox_new(0,6);
  GdkWindow *root=gdk_get_default_root_window();
  GtkWidget *rightbox=gtk_vbox_new(0,0);
  GtkWidget *leftbox=gtk_vbox_new(0,6);

  panel->toplevel=gtk_window_new (GTK_WINDOW_TOPLEVEL);
  panel->group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW(panel->toplevel), panel->group);

  char versionmarkup[240];
  snprintf(versionmarkup,240," <span size=\"large\" weight=\"bold\" "
	   "style=\"italic\" foreground=\"dark blue\">"
	   "Waveform Viewer</span>  <span size=\"small\" foreground=\"#606060\">"
	   "revision %s</span> ",
	   version);

  /* the Fucking Fish */
  for(i=0;i<19;i++)
    panel->ff[i]=gdk_pixmap_create_from_xpm_d(root,
					      panel->fb+i,NULL,ff_xpm[i]);
  panel->twirlimage=gtk_image_new_from_pixmap(panel->ff[0],panel->fb[0]);

  active = calloc(total_ch,sizeof(*active));

  topplace=gtk_table_new(1,1,0);
  topalb=gtk_hbox_new(0,0);
  topal=gtk_alignment_new(1,0,0,0);

  gtk_widget_set_name(quitbutton,"quitbutton");

  gtk_box_pack_start(GTK_BOX(topalb),quitbutton,0,0,0);
  gtk_container_add (GTK_CONTAINER(topal),topalb);
  
  gtk_table_attach_defaults(GTK_TABLE(topplace),
			    topal,0,1,0,1);
  gtk_table_attach_defaults(GTK_TABLE(topplace),
			    topframe,0,1,0,1);
    
  gtk_container_add (GTK_CONTAINER (panel->toplevel), topplace);
  gtk_container_set_border_width (GTK_CONTAINER (quitbutton), 3);

  g_signal_connect (G_OBJECT (quitbutton), "clicked",
		    G_CALLBACK (shutdown), NULL);
  gtk_widget_add_accelerator (quitbutton, "activate", panel->group, GDK_q, 0, 0);

  gtk_container_set_border_width (GTK_CONTAINER (topframe), 3);
  gtk_container_set_border_width (GTK_CONTAINER (mainbox), 3);
  gtk_frame_set_shadow_type(GTK_FRAME(topframe),GTK_SHADOW_ETCHED_IN);
  gtk_frame_set_label_widget(GTK_FRAME(topframe),toplabel);
  gtk_label_set_markup(GTK_LABEL(toplabel),versionmarkup);

  gtk_container_add (GTK_CONTAINER(topframe), mainbox);

  g_signal_connect (G_OBJECT (panel->toplevel), "delete_event",
		    G_CALLBACK (shutdown), NULL);

  /* add the waveform plot box */
  panel->plot=plot_new(blocksize,inputs,channels,rate);
  gtk_box_pack_end(GTK_BOX(leftbox),panel->plot,1,1,0);
  gtk_box_pack_start(GTK_BOX(mainbox),leftbox,1,1,0);
  
  /*fish */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *fishbox=gtk_hbox_new(0,0);
    gtk_box_pack_end(GTK_BOX(fishbox),panel->twirlimage,0,0,0);
    gtk_container_set_border_width (GTK_CONTAINER (fishbox), 3);

    gtk_box_pack_start(GTK_BOX(box),fishbox,0,0,0);
    gtk_box_pack_start(GTK_BOX(rightbox),box,0,0,0);
  }

  /* rate */
  /* channels */
  /* bits */
  {
    int fi;
    int ch=0;
    char buffer[160];
    GtkWidget *label;
    //GtkWidget *vbox=gtk_vbox_new(1,1);

    GtkWidget *sep=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(rightbox),sep,0,0,6);

    panel->chbuttons = calloc(total_ch,sizeof(*panel->chbuttons));
    for(fi=0;fi<inputs;fi++){
      
      char *lastslash = strrchr(inputname[fi],'/');
      sprintf(buffer,"%s",(lastslash?lastslash+1:inputname[fi]));
      label=gtk_label_new(buffer);
      gtk_widget_set_name(label,"readout");
      gtk_box_pack_start(GTK_BOX(rightbox),label,0,0,0);

      sprintf(buffer,"%dHz %dbit",rate[fi],bits[fi]);
      label=gtk_label_new(buffer);
      gtk_widget_set_name(label,"readout");
      gtk_box_pack_start(GTK_BOX(rightbox),label,0,0,0);

      for(i=ch;i<ch+channels[fi];i++){
	GtkWidget *button=panel->chbuttons[i]=gtk_toggle_button_new();
        GdkColor rgb = chcolor(i);

        sprintf(buffer,"channel %d", i-ch);
        gtk_button_set_label(GTK_BUTTON(button),buffer);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),1);  
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (chlabels), panel);

        set_fg(button,&rgb);
	gtk_box_pack_start(GTK_BOX(rightbox),button,0,0,0);
      }

      GtkWidget *sep=gtk_hseparator_new();
      gtk_box_pack_start(GTK_BOX(rightbox),sep,0,0,6);

      ch+=channels[fi];

    }
    chlabels(NULL,panel);
  }
  
  GtkWidget *bbox=gtk_vbox_new(0,0);

  /* add the action buttons */
  /* range */
  {
    GtkWidget *box=gtk_hbox_new(1,1);

    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"","","","","",""};
    for(i=0;i<6;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (rangechange), panel);
    panel->rangemenu = menu;

    GtkWidget *menu2=gtk_combo_box_new_text();
    char *entries2[]={"linear","-65dB","-96dB","-120dB","-160dB"};
    for(i=0;i<5;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu2), entries2[i]);
    g_signal_connect (G_OBJECT (menu2), "changed",
		      G_CALLBACK (scalechange), panel);

    gtk_box_pack_start(GTK_BOX(box),menu2,1,1,0);
    gtk_box_pack_start(GTK_BOX(box),menu,1,1,0);
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu2),0);
  }

  /* span */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *label=gtk_label_new ("span: ");
    gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.5f);

    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"1s",
                     "500ms","200ms","100ms",
                     "50ms","20ms","10ms",
                     "5ms","2ms","1ms",
                     "500\xCE\xBCs","200\xCE\xBCs",
                     "100\xCE\xBCs"};
    for(i=0;i<13;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (spanchange), panel);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);


    gtk_box_pack_start(GTK_BOX(box),label,1,1,0);
    gtk_box_pack_start(GTK_BOX(box),menu,1,1,0);
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }

  /* trigger */
  {
    GtkWidget *box=gtk_hbox_new(1,1);

    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"free run"};
    for(i=0;i<1;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (triggerchange), panel);

    /* interval */

    GtkWidget *menu2=gtk_combo_box_new_text();
    char *entries2[]={"1s",
                      "500ms","200ms","100ms",
                      "50ms","20ms","10ms",
                      "5ms","2ms","1ms",
                      "500\xCE\xBCs","200\xCE\xBCs",
                      "100\xCE\xBCs"};
    for(i=0;i<13;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu2), entries2[i]);
    g_signal_connect (G_OBJECT (menu2), "changed",
		      G_CALLBACK (intervalchange), panel);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu2),3);


    gtk_box_pack_start(GTK_BOX(box),menu,1,1,0);
    gtk_box_pack_start(GTK_BOX(box),menu2,1,1,0);
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);


  }

  /* plot type */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"zero-hold","interpolated","lollipop"};
    for(i=0;i<3;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (plotchange), panel);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
  }

  {
    GtkWidget *sep=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(bbox),sep,0,0,4);
  }

  /* run/pause */
  {
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("_run");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_space, 0, 0);
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_r, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (runchange), panel);
    gtk_box_pack_start(GTK_BOX(bbox),button,0,0,0);
    panel->run=button;
  }

  /* hold */
  /* loop */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("_hold");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_h, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (holdchange), panel);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);

    button=gtk_toggle_button_new_with_mnemonic("_loop");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_l, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (loopchange), panel);
    gtk_widget_set_sensitive(button,global_seekable);

    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }

  /* clear */
  /* rewind */
  {
    GtkWidget *button=gtk_button_new_with_mnemonic("re_wind");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_w, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (rewindchange), panel);
    gtk_widget_set_sensitive(button,global_seekable);
    gtk_box_pack_start(GTK_BOX(bbox),button,1,1,0);
  }

  gtk_box_pack_end(GTK_BOX(rightbox),bbox,0,0,0);
  gtk_box_pack_start(GTK_BOX(mainbox),rightbox,0,0,0);

  gtk_widget_show_all(panel->toplevel);
  //gtk_window_set_resizable(GTK_WINDOW(panel->toplevel),0);

}

static gboolean async_event_handle(GIOChannel *channel,
				   GIOCondition condition,
				   gpointer data){
  struct panel *panel=data;
  char buf[1];

  /* read all pending */
  while(read(eventpipe[0],buf,1)>0);

  increment_fish=1;

  /* check playback status and update the run button if needed */
  if(process_active && panel->run && 
     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel->run)))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel->run),1);
  if(!process_active && panel->run && 
     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel->run)))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel->run),0);

  /* update the waveform display; send new data */
  pthread_mutex_lock(&feedback_mutex);
  if(plot_last_update!=feedback_increment){
    pthread_mutex_unlock(&feedback_mutex);
    replot(panel);
    plot_draw(PLOT(panel->plot));

    while (gtk_events_pending())
      gtk_main_iteration();
  }else
    pthread_mutex_unlock(&feedback_mutex);

  return TRUE;
}

static int look_for_gtkrc(char *filename){
  FILE *f=fopen(filename,"r");
  if(!f)return 0;
  fprintf(stderr,"Loading waveform-gtkrc file found at %s\n",filename);
  gtk_rc_add_default_file(filename);
  return 1;
}

void panel_go(int argc,char *argv[]){
  char *homedir=getenv("HOME");
  int found=0;
  memset(&p,0,sizeof(p));

  found|=look_for_gtkrc(ETCDIR"/waveform-gtkrc");
  {
    char *rcdir=getenv("HOME");
    if(rcdir){
      char *rcfile="/.spectrum/waveform-gtkrc";
      char *homerc=calloc(1,strlen(rcdir)+strlen(rcfile)+1);
      strcat(homerc,homedir);
      strcat(homerc,rcfile);
      found|=look_for_gtkrc(homerc);
    }
  }
  {
    char *rcdir=getenv("SPECTRUM_RCDIR");
    if(rcdir){
      char *rcfile="/waveform-gtkrc";
      char *homerc=calloc(1,strlen(rcdir)+strlen(rcfile)+1);
      strcat(homerc,homedir);
      strcat(homerc,rcfile);
      found|=look_for_gtkrc(homerc);
    }
  }
  found|=look_for_gtkrc("./waveform-gtkrc");

  if(!found){
  
    fprintf(stderr,"Could not find the waveform-gtkrc configuration file normally\n"
	    "installed in one of the following places:\n"

	    "\t./waveform-gtkrc\n"
	    "\t$(SPECTRUM_RCDIR)/waveform-gtkrc\n"
	    "\t~/.spectrum/waveform-gtkrc\n\t"
	    ETCDIR"/wavegform-gtkrc\n"
	    "This configuration file is used to tune the color, font and other detail aspects\n"
	    "of the user interface.  Although the viewer will work without it, the UI\n"
	    "appearence will likely make the application harder to use due to missing visual\n"
	    "cues.\n");
  }

  gtk_rc_add_default_file(ETCDIR"/waveform-gtkrc");
  if(homedir){
    char *rcfile="/.waveform-gtkrc";
    char *homerc=calloc(1,strlen(homedir)+strlen(rcfile)+1);
    strcat(homerc,homedir);
    strcat(homerc,rcfile);
    gtk_rc_add_default_file(homerc);
  }
  gtk_rc_add_default_file(".waveform-gtkrc");
  gtk_rc_add_default_file("waveform-gtkrc");
  gtk_init (&argc, &argv);

  panel_create(&p);
  animate_fish(&p);

  /* set up watching the event pipe */
  {
    GIOChannel *channel = g_io_channel_unix_new (eventpipe[0]);
    guint id;

    g_io_channel_set_encoding (channel, NULL, NULL);
    g_io_channel_set_buffered (channel, FALSE);
    g_io_channel_set_close_on_unref (channel, TRUE);

    id = g_io_add_watch (channel, G_IO_IN, async_event_handle, &p);

    g_io_channel_unref (channel);

  }
  
  /* we want to be running by default */
  {
    pthread_t thread_id;
    animate_fish(&p);
    process_active=1;
    pthread_create(&thread_id,NULL,&process_thread,NULL);
  }

  gtk_main ();

}

