/*
 *
 *  gtk2 spectrum analyzer
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

#include "analyzer.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "fisharray.h"
#include "spec_plot.h"
#include "io.h"

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
  GtkWidget *bwtable;
  GtkWidget *bwbutton;
  GtkWidget *bwmodebutton;
} p;

int plot_scale=0;
int plot_mode=0;
int plot_link=0;
int plot_hold=0;
int plot_depth=90;
int plot_noise=0;
int plot_last_update=0;
int plot_bw=0;
int plot_bwmode=0;
int *active;

static void replot(struct panel *p){
  int i,lactive[total_ch];
  for(i=0;i<total_ch;i++)
    lactive[i]=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->chbuttons[i]));

  /* update the spectral display; send new data */
  if(!plot_hold){
    pthread_mutex_lock(&feedback_mutex);
    plot_refresh(PLOT(p->plot),lactive);
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
      g_timeout_add(70,(GSourceFunc)reanimate_fish,p);
  }else{
    p->fishframe_init=1;
    p->fishframe_timer=
      g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,p);
  }
}

static void dump(GtkWidget *widget,struct panel *p){
  process_dump(plot_mode);
}

#if 0
static void noise(GtkWidget *widget,struct panel *p){
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))){
    if(plot_noise){
      plot_noise=0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),0);
      clear_noise_floor();
      plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,0);
    }else{
      plot_noise=1;
      plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,0);
    }
  }else{
    if(plot_noise){
      gtk_button_set_label(GTK_BUTTON(widget),"clear _noise floor");
      plot_noise=2;
      plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,1);
    }else
      gtk_button_set_label(GTK_BUTTON(widget),"sample _noise floor");
  }
}
#endif

static void depthchange(GtkWidget *widget,struct panel *p){
  int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  switch(choice){
  case 0: /* 1dB */
    plot_depth=1;
    break;
  case 1: /* 10dB */
    plot_depth=10;
    break;
  case 2: /* 20dB */
    plot_depth=20;
    break;
  case 3: /* 45dB */
    plot_depth=45;
    break;
  case 4: /* 90dB */
    plot_depth=90;
    break;
  case 5: /*140dB */
    plot_depth=140;
    break;
  case 6: /*200dB */
    plot_depth=200;
    break;
  }
  plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,plot_noise);
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

static void set_via_active(struct panel *p, int *active, int *bactive){
  int fi,i;
  int ch=0;
  for(fi=0;fi<inputs;fi++){
    for(i=ch;i<ch+channels[fi];i++)
      gtk_widget_set_sensitive(p->chbuttons[i],1);
    ch+=channels[fi];
  }
  plot_set_active(PLOT(p->plot),active,bactive);  
}

static void chlabels(GtkWidget *widget,struct panel *p){
  /* scan state, update labels on channel buttons, set sensitivity
     based on grouping and mode */
  int fi,ch,i;
  char buf[80];
  int bactive[total_ch];

  for(i=0;i<total_ch;i++)
    bactive[i]=active[i]=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->chbuttons[i]));

  /* set sensitivity */
  switch(plot_link){
  case LINK_SUB_REF:

    /*  first channel in each group insensitive/inactive, used as a reference */
    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	if(i==ch){
	  gtk_widget_set_sensitive(p->chbuttons[i],0);
	  active[i]=0; /* do not frob widget, only plot settings */
	}else{
	  gtk_widget_set_sensitive(p->chbuttons[i],1);
	}
      }
      ch+=channels[fi];
    }
 
    plot_set_active(PLOT(p->plot),active,bactive);
    break;    

  case LINK_SUMMED: /* summing mode */
  case LINK_SUB_FROM: /* subtract channels from reference */
    ch=0;
    for(fi=0;fi<inputs;fi++){
      int any=0;
      for(i=ch;i<ch+channels[fi];i++){
	if(active[i])any=1;
	active[i]=0;
      }
      active[ch]=any;
      ch+=channels[fi];
    }

    set_via_active(p,active,bactive);
    break;

  case LINK_INDEPENDENT: /* normal/independent mode */
    set_via_active(p,active,bactive);
    break;    

  case LINK_PHASE: /* response/phase */
    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++)
	if(channels[fi]<2){
	  gtk_widget_set_sensitive(p->chbuttons[i],0);
	  active[i]=0;
	}else{
	  if(i<ch+2){
	    gtk_widget_set_sensitive(p->chbuttons[i],1);
	  }else{
	    gtk_widget_set_sensitive(p->chbuttons[i],0);
	    active[i]=0;
	  }
	}
      ch+=channels[fi];
    }
    plot_set_active(PLOT(p->plot),active,bactive);
    break;    
  }

  /* set labels */
  switch(plot_link){
  case LINK_SUB_REF:
    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	if(i==ch){
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),"reference");
	}else{
	  sprintf(buf,"channel %d", i-ch);
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),buf);
	}
      }
      ch+=channels[fi];
    }
    break;    
  case LINK_SUB_FROM:
    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	if(i==ch){
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),"output");
	}else{
	  sprintf(buf,"channel %d", i-ch);
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),buf);
	}
      }
      ch+=channels[fi];
    }
    break;    

  case LINK_INDEPENDENT:
  case LINK_SUMMED:

    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	sprintf(buf,"channel %d", i-ch);
	gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),buf);
      }
      ch+=channels[fi];
    }
    break;    

  case LINK_PHASE:

    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	if(channels[fi]<2){
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),"unused");
	}else if(i==ch){
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),"response");
	}else if(i==ch+1){
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),"phase");
	}else{
	  gtk_button_set_label(GTK_BUTTON(p->chbuttons[i]),"unused");
	}
      }
      ch+=channels[fi];
    }
    break;    
  }

  /* set colors */
  switch(plot_link){
  case LINK_SUMMED:
  case LINK_SUB_FROM:
    ch=0;
    for(fi=0;fi<inputs;fi++){
      GdkColor rgb = chcolor(ch);

      for(i=ch;i<ch+channels[fi];i++){
	GtkWidget *button=p->chbuttons[i];	
	set_fg(button,&rgb);
      }
      ch+=channels[fi];
    }
    break;

  default:
    ch=0;
    for(fi=0;fi<inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	GdkColor rgb = chcolor(i);
	GtkWidget *button=p->chbuttons[i];
	set_fg(button,&rgb);
      }
      ch+=channels[fi];
    }
    break;
  }

}

static void scalechange(GtkWidget *widget,struct panel *p){
  plot_scale=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,plot_noise);
}

static void modechange(GtkWidget *widget,struct panel *p){
  plot_mode=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  replot(p);
  plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,plot_noise);
}

static void linkchange(GtkWidget *widget,struct panel *p){
  plot_link=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  replot(p);
  plot_setting(PLOT(p->plot),plot_scale,plot_mode,plot_link,plot_depth,plot_noise);
  chlabels(widget,p);
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

static void loopchange(GtkWidget *widget,struct panel *p){
  acc_loop=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void clearchange(GtkWidget *widget,struct panel *p){
  acc_clear=1;
  plot_clear(PLOT(p->plot));
  if(!process_active){
    rundata_clear();
  }
}

static void rewindchange(GtkWidget *widget,struct panel *p){
  acc_rewind=1;
}

static void bwchange(GtkWidget *widget,struct panel *p){
  plot_bw=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  if(plot_bw==0){
    gtk_widget_hide(p->bwmodebutton);
    gtk_container_remove(GTK_CONTAINER(p->bwtable),p->bwbutton);
    gtk_table_attach_defaults(GTK_TABLE(p->bwtable),p->bwbutton,0,2,0,1);
  }else{
    gtk_container_remove(GTK_CONTAINER(p->bwtable),p->bwbutton);
    gtk_table_attach_defaults(GTK_TABLE(p->bwtable),p->bwbutton,0,1,0,1);
    gtk_widget_show(p->bwmodebutton);
  }
}

static void bwmodechange(GtkWidget *widget,struct panel *p){
  plot_bwmode=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
}

static gint watch_keyboard(GtkWidget *grab_widget,
                           GdkEventKey *event,
                           gpointer func_data){
  struct panel *p=(struct panel *)func_data;

  if(event->type == GDK_KEY_PRESS){
    if(event->state == GDK_CONTROL_MASK){
      if(event->keyval == GDK_w) { shutdown(); return TRUE; }
      if(event->keyval == GDK_q) { shutdown(); return TRUE; }
    }
  }
  return FALSE;
}

extern char *version;
void panel_create(struct panel *panel, int bold){
  int i;

  GtkWidget *topbox=gtk_hbox_new(0,0);
  GtkWidget *toplabel=gtk_label_new (NULL);
  GtkWidget *rightframe=gtk_frame_new (NULL);
  GtkWidget *leftlabel=gtk_label_new (NULL);
  GdkWindow *root=gdk_get_default_root_window();
  GtkWidget *righttopbox=gtk_vbox_new(0,0);
  GtkWidget *rightframebox=gtk_event_box_new();
  GtkWidget *rightbox=gtk_vbox_new(0,0);
  GtkWidget *lefttable=gtk_table_new(3,2,0);

  gtk_container_set_border_width (GTK_CONTAINER (righttopbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (rightbox), 6);
  gtk_widget_set_name(rightframebox,"controlpanel");
  gtk_widget_set_name(topbox,"panel");


  panel->toplevel=gtk_window_new (GTK_WINDOW_TOPLEVEL);
  panel->group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW(panel->toplevel), panel->group);

  gtk_window_set_title(GTK_WINDOW(panel->toplevel),(const gchar *)"Spectrum Analyzer");

  /* the Fucking Fish */
  for(i=0;i<19;i++){
    int j,k,lines = sizeof(ff_colormap)/sizeof(*ff_colormap) + sizeof(*ff_xpm)/sizeof(**ff_xpm) + 1;
    char *ff_temp[lines];
    GdkImage *ti;

    ff_temp[0]=ff_header;
    for(j=0,k=1;j<sizeof(ff_colormap)/sizeof(*ff_colormap);j++,k++)
      ff_temp[k]=ff_colormap[j];
    for(j=0;j<sizeof(*ff_xpm)/sizeof(**ff_xpm);j++,k++)
      ff_temp[k]=ff_xpm[i][j];

    panel->ff[i]=gdk_pixmap_create_from_xpm_d(root,panel->fb+i,NULL,ff_temp);
  }
  panel->twirlimage=gtk_image_new_from_pixmap(panel->ff[0],panel->fb[0]);

  active = calloc(total_ch,sizeof(*active));

  gtk_container_add (GTK_CONTAINER (panel->toplevel), topbox);

  gtk_frame_set_shadow_type(GTK_FRAME(rightframe),GTK_SHADOW_ETCHED_IN);

  g_signal_connect (G_OBJECT (panel->toplevel), "delete_event",
		    G_CALLBACK (shutdown), NULL);


  /* underlying boxes/frames */
  gtk_box_pack_start(GTK_BOX(topbox),lefttable,1,1,0);
  gtk_box_pack_start(GTK_BOX(topbox),righttopbox,0,0,0);

  gtk_box_pack_end(GTK_BOX (righttopbox),rightframebox,1,1,0);
  gtk_container_add (GTK_CONTAINER (rightframebox),rightframe);
  gtk_container_add (GTK_CONTAINER (rightframe), rightbox);

  /* add the spectrum plot box */
  panel->plot=plot_new(blocksize/2+1,inputs,channels,rate,bold);
  gtk_table_attach_defaults (GTK_TABLE (lefttable), panel->plot,0,1,1,2);
  gtk_table_set_row_spacing (GTK_TABLE (lefttable), 0, 6);
  gtk_table_set_row_spacing (GTK_TABLE (lefttable), 1, 4);
  gtk_table_set_col_spacing (GTK_TABLE (lefttable), 0, 2);

  /* fish */
  {
    GtkWidget *toptable = gtk_table_new(1,1,0);
    GtkWidget *fishbox=gtk_alignment_new(.5,.5,0,0);
    GtkWidget *sepbox=gtk_alignment_new(.5,.85,.7,0);
    GtkWidget *topsep=gtk_hseparator_new();


    gtk_container_set_border_width (GTK_CONTAINER (toptable), 1);
    gtk_box_pack_start(GTK_BOX(righttopbox),toptable,0,0,0);
    gtk_container_add (GTK_CONTAINER (sepbox), topsep);
    gtk_container_add(GTK_CONTAINER(fishbox),panel->twirlimage);
    gtk_table_attach_defaults (GTK_TABLE (toptable), fishbox,0,1,0,1);
    gtk_table_attach_defaults (GTK_TABLE (toptable), sepbox,0,1,0,1);

  }

  /* rate */
  /* channels */
  /* bits */
  {
    int fi;
    int ch=0;
    char buffer[160];
    GtkWidget *label;

    panel->chbuttons = calloc(total_ch,sizeof(*panel->chbuttons));
    for(fi=0;fi<inputs;fi++){
      GtkWidget *al=gtk_alignment_new(0,0,1,0);
      GtkWidget *vbox=gtk_vbox_new(0,0);
      
      char *lastslash = strrchr(inputname[fi],'/');
      sprintf(buffer,"%s",(lastslash?lastslash+1:inputname[fi]));
      label=gtk_label_new(buffer);
      gtk_widget_set_name(label,"readout");
      gtk_box_pack_start(GTK_BOX(vbox),label,0,0,0);
      
      sprintf(buffer,"%dHz %dbit",rate[fi],bits[fi]);
      label=gtk_label_new(buffer);
      gtk_widget_set_name(label,"readout");
      gtk_box_pack_start(GTK_BOX(vbox),label,0,0,0);

      for(i=ch;i<ch+channels[fi];i++){
	GtkWidget *button=panel->chbuttons[i]=gtk_toggle_button_new();

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),1);  
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (chlabels), panel);
	gtk_box_pack_start(GTK_BOX(vbox),button,0,0,0);
      }

      gtk_container_add(GTK_CONTAINER(al),vbox);
      gtk_alignment_set_padding(GTK_ALIGNMENT(al),0,10,0,0);
      gtk_box_pack_start(GTK_BOX(rightbox),al,0,0,0);


      ch+=channels[fi];

    }
    chlabels(NULL,panel);
  }
  
  /* add the action buttons */
  GtkWidget *bbox=gtk_vbox_new(0,0);

  {
  /* bandwidth mode */
    GtkWidget *tbox=panel->bwtable=gtk_table_new(2,2,0);

    GtkWidget *menu=panel->bwbutton=gtk_combo_box_new_text();
    char *entries[]={"native","display"};
    //"1Hz","3Hz","10Hz","30Hz","100Hz","300Hz","1kHz",
    //"1/24oct","1/12oct","1/6oct","1/3oct"};

    for(i=0;i<2;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    //gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    
    g_signal_connect (G_OBJECT (menu), "changed",
    		      G_CALLBACK (bwchange), panel);

    GtkWidget *menu2=panel->bwmodebutton=gtk_combo_box_new_text();
    char *entries2[]={"RBW","VBW"};
    for(i=0;i<2;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu2), entries2[i]);
    
    g_signal_connect (G_OBJECT (menu2), "changed",
    		      G_CALLBACK (bwmodechange), panel);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu2),0);

    gtk_table_attach_defaults(GTK_TABLE(tbox),menu,0,1,0,1);
    gtk_table_attach_defaults(GTK_TABLE(tbox),menu2,1,2,0,1);

    /* scale */
    /* depth */

    GtkWidget *menu3=gtk_combo_box_new_text();
    char *entries3[]={"log freq","ISO freq","linear freq"};
    for(i=0;i<3;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu3), entries3[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu3),plot_scale);
    plot_setting(PLOT(panel->plot),plot_scale,plot_mode,plot_link,plot_depth,plot_noise);
    
    g_signal_connect (G_OBJECT (menu3), "changed",
		      G_CALLBACK (scalechange), panel);

    GtkWidget *menu4=gtk_combo_box_new_text();
    char *entries4[]={"1dB","10dB","20dB","45dB","90dB","140dB","200dB"};
    for(i=0;i<7;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu4), entries4[i]);
    
    g_signal_connect (G_OBJECT (menu4), "changed",
		      G_CALLBACK (depthchange), panel);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu4),4);

    gtk_table_attach_defaults(GTK_TABLE(tbox),menu3,0,1,1,2);
    gtk_table_attach_defaults(GTK_TABLE(tbox),menu4,1,2,1,2);

    gtk_box_pack_start(GTK_BOX(bbox),tbox,0,0,0);

  }
  
  /* mode */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"realtime","maximum","accumulate","average"};
    for(i=0;i<4;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),plot_mode);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (modechange), panel);
  }
  
  /* link */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    for(i=0;i<LINKS;i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), link_entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (linkchange), panel);
  }
  

  /* run/pause */
  {
    GtkWidget *al=gtk_alignment_new(0,0,1,0);
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("_run");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_space, 0, 0);
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_r, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (runchange), panel);
    gtk_container_add(GTK_CONTAINER(al),button);
    gtk_alignment_set_padding(GTK_ALIGNMENT(al),8,0,0,0);
    gtk_box_pack_start(GTK_BOX(bbox),al,0,0,0);
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
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);
    gtk_widget_set_sensitive(button,global_seekable);
    
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }
  
  /* clear */
  /* rewind */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *button=gtk_button_new_with_mnemonic("_clear");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_c, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (clearchange), panel);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);
    
    button=gtk_button_new_with_mnemonic("re_wind");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_w, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (rewindchange), panel);
    gtk_widget_set_sensitive(button,global_seekable);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);
    
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }
  
  /* dump */
  {
    GtkWidget *button=gtk_button_new_with_mnemonic("_dump data");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_d, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (dump), panel);
    gtk_box_pack_start(GTK_BOX(bbox),button,0,0,0);
  }

  /* noise floor */
#if 0
  {
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("sample _noise floor");
    gtk_widget_add_accelerator (button, "activate", panel->group, GDK_n, 0, 0);
    g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (noise), panel);
    gtk_box_pack_start(GTK_BOX(bbox),button,0,0,0);
  }
#endif

  gtk_box_pack_end(GTK_BOX(rightbox),bbox,0,0,0);
    
  gtk_widget_show_all(panel->toplevel);
  gtk_combo_box_set_active(GTK_COMBO_BOX(panel->bwbutton),0);
  gtk_key_snooper_install(watch_keyboard,panel);

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

  /* update the spectral display; send new data */
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
  fprintf(stderr,"Loading spectrum-gtkrc file found at %s\n",filename);
  gtk_rc_add_default_file(filename);
  return 1;
}

void panel_go(int argc,char *argv[],int bold){
  char *homedir=getenv("HOME");
  int found=0;
  memset(&p,0,sizeof(p));

  found|=look_for_gtkrc(ETCDIR"/spectrum-gtkrc");
  {
    char *rcdir=getenv("HOME");
    if(rcdir){
      char *rcfile="/.spectrum/spectrum-gtkrc";
      char *homerc=calloc(1,strlen(rcdir)+strlen(rcfile)+1);
      strcat(homerc,homedir);
      strcat(homerc,rcfile);
      found|=look_for_gtkrc(homerc);
    }
  }
  {
    char *rcdir=getenv("SPECTRUM_RCDIR");
    if(rcdir){
      char *rcfile="/spectrum-gtkrc";
      char *homerc=calloc(1,strlen(rcdir)+strlen(rcfile)+1);
      strcat(homerc,homedir);
      strcat(homerc,rcfile);
      found|=look_for_gtkrc(homerc);
    }
  }
  found|=look_for_gtkrc("./spectrum-gtkrc");

  if(!found){
  
    fprintf(stderr,"Could not find the spectrum-gtkrc configuration file normally\n"
	    "installed in one of the following places:\n"

	    "\t./spectrum-gtkrc\n"
	    "\t$(SPECTRUM_RCDIR)/spectrum-gtkrc\n"
	    "\t~/.spectrum/spectrum-gtkrc\n\t"
	    ETCDIR"/spectrum-gtkrc\n"
	    "This configuration file is used to tune the color, font and other detail aspects\n"
	    "of the user interface.  Although the analyzer will work without it, the UI\n"
	    "appearence will likely make the application harder to use due to missing visual\n"
	    "cues.\n");
  }

  gtk_rc_add_default_file(ETCDIR"/spectrum-gtkrc");
  if(homedir){
    char *rcfile="/.spectrum-gtkrc";
    char *homerc=calloc(1,strlen(homedir)+strlen(rcfile)+1);
    strcat(homerc,homedir);
    strcat(homerc,rcfile);
    gtk_rc_add_default_file(homerc);
  }
  gtk_rc_add_default_file(".spectrum-gtkrc");
  gtk_rc_add_default_file("spectrum-gtkrc");
  gtk_init (&argc, &argv);

  panel_create(&p, bold);
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

