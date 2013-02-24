/*
 *
 *  gtk2 waveform viewer
 *
 *      Copyright (C) 2004-2012 Monty
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
#include "wave_plot.h"

GtkWidget *twirlimage;
GdkPixmap *ff[19];
GdkBitmap *fb[19];

GtkAccelGroup *group;
GtkWidget *toplevel;

guint fishframe_timer;
int fishframe_init;
int fishframe;

GtkWidget *rightbox;
GtkWidget *plot;
GtkWidget *run;
GtkWidget **chbuttons;
GtkWidget **groupboxes;
GtkWidget *scalemenu;
GtkWidget *rangemenu;

int plot_ch=0;
int plot_inputs=0;

int plot_scale=0;
int plot_span=0;
float plot_range=0;
int plot_rchoice=0;
int plot_schoice=0;
int plot_spanchoice=0;
int plot_interval=0;
int plot_trigger=0;
int plot_type=0;
int plot_hold=0;
int plot_bold=1;
int plot_sep=0;

/* first up... the Fucking Fish */
sig_atomic_t increment_fish=0;

static int reanimate_fish(void){
  if(process_active || (fishframe>0 && fishframe<12)){
    /* continue spinning */
    if(increment_fish || fishframe>0)fishframe++;
    if(fishframe==1)increment_fish=0;
    if(fishframe>=12)fishframe=0;

    gtk_image_set_from_pixmap(GTK_IMAGE(twirlimage),
			      ff[fishframe],
			      fb[fishframe]);

    if(fishframe==0 && !process_active){
      /* reschedule to blink */
      fishframe_timer=
	g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,NULL);
      return FALSE;
    }

  }else{
    fishframe++;
    if(fishframe<=1)fishframe=12;
    if(fishframe>=19)fishframe=0;

    gtk_image_set_from_pixmap(GTK_IMAGE(twirlimage),
			      ff[fishframe],
			      fb[fishframe]);


    if(fishframe==12){
      /* reschedule to animate */
      fishframe_timer=
	g_timeout_add(10,(GSourceFunc)reanimate_fish,NULL);
      return FALSE;
    }
    if(fishframe==0){
      /* reschedule to blink */
      fishframe_timer=
	g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,NULL);
      return FALSE;
    }
  }
  return TRUE;
}

static void animate_fish(void){
  if(fishframe_init){
    g_source_remove(fishframe_timer);
    fishframe_timer=
      g_timeout_add(80,(GSourceFunc)reanimate_fish,NULL);
  }else{
    fishframe_init=1;
    fishframe_timer=
      g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,NULL);
  }
}

static void override_base(GtkWidget *w, int active){
  gtk_widget_modify_base
    (w, GTK_STATE_NORMAL,
     &w->style->bg[active?GTK_STATE_ACTIVE:GTK_STATE_NORMAL]);
}

static void chlabels(GtkWidget *widget,gpointer in){
  replot();
}

static int rangechange_ign=0;
static void rangechange(GtkWidget *widget,gpointer in){
  if(!rangechange_ign){
    int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    plot_rchoice=choice;
    switch(choice){
    case 0:
      plot_range=16;
      break;
    case 1:
      plot_range=8;
      break;
    case 2:
      plot_range=4;
      break;
    case 3:
      plot_range=2;
      break;
    case 4:
      plot_range=1;
      break;
    case 5:
      plot_range=.5;
      break;
    case 6:
      plot_range=.2;
      break;
    case 7:
      plot_range=.1;
      break;
    case 8:
      plot_range=.01;
      break;
    case 9:
      plot_range=.001;
      break;
    case 10:
      plot_range=.0001;
      break;
    }
    replot();
  }
}

static void scalechange(GtkWidget *widget,gpointer in){
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
    char *entries[]={
      "16.0/div",
      "8.0/div",
      "4.0/div",
      "2.0/div",
      "1.0/div",
      "0.5/div",
      "0.2/div",
      "0.1/div",
      "0.01/div",
      "0.001/div",
      "0.0001/div"};
    for(i=0;i<11;i++){
      gtk_combo_box_remove_text (GTK_COMBO_BOX (rangemenu), i);
      gtk_combo_box_insert_text (GTK_COMBO_BOX (rangemenu), i, entries[i]);
    }

  }else{
    char *entries[]={
      "24dB@div",
      "18dB@div",
      "12dB@div",
      "6dB@div",
      "0dB@div",
      "-6dB@div",
      "-14dB@div",
      "-20dB@div",
      "-40dB@div",
      "-60dB@div",
    };
    for(i=0;i<10;i++){
      gtk_combo_box_remove_text (GTK_COMBO_BOX (rangemenu), i);
      gtk_combo_box_insert_text (GTK_COMBO_BOX (rangemenu), i, entries[i]);
    }
    gtk_combo_box_remove_text (GTK_COMBO_BOX (rangemenu), i);
  }
  if(plot_rchoice==10){
    plot_rchoice=9;
    plot_range=.001;
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(rangemenu),plot_rchoice);
  rangechange_ign=0;
  replot();
}

static void spanchange(GtkWidget *widget,gpointer in){
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

  replot();

}

static void intervalchange(GtkWidget *widget,gpointer in){
  int choice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  switch(choice){
  case 0:
    plot_interval=1;
    break;
  case 1:
    plot_interval=2;
    break;
  case 2:
    plot_interval=5;
    break;
  case 3:
    plot_interval=10;
    break;
  case 4:
    plot_interval=20;
    break;
  case 5:
    plot_interval=50;
    break;
  case 6:
    plot_interval=100;
    break;
  case 7:
    plot_interval=200;
    break;
  case 8:
    plot_interval=500;
    break;
  case 9:
    plot_interval=1000;
    break;
  case 10:
    plot_interval=2000;
    break;
  case 11:
    plot_interval=5000;
    break;
  case 12:
    plot_interval=10000;
    break;
  }

  blockslice_frac = plot_interval;
  set_trigger(plot_trigger,0,plot_interval,plot_span);
  //replot();
}

static void triggerchange(GtkWidget *widget,gpointer in){
  plot_trigger=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  set_trigger(plot_trigger,0,plot_interval,plot_span);
}

static void runchange(GtkWidget *widget,gpointer in){
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))){
    if(!process_active){
      pthread_t thread_id;
      process_active=1;
      process_exit=0;
      animate_fish();
      pthread_create(&thread_id,NULL,&process_thread,NULL);
    }
  }else{
    process_exit=1;
  }
}

static void holdchange(GtkWidget *widget,gpointer in){
  plot_hold=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  override_base(widget,plot_bold);
  replot();
}

static void boldchange(GtkWidget *widget,gpointer in){
  plot_bold=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  override_base(widget,plot_bold);
  replot();
}

static void sepchange(GtkWidget *widget,gpointer in){
  plot_sep=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  override_base(widget,plot_sep);
  replot();
}

static void plotchange(GtkWidget *widget,gpointer in){
  plot_type=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  replot();
}

static void loopchange(GtkWidget *widget,gpointer in){
  acc_loop=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void rewindchange(GtkWidget *widget,gpointer in){
  acc_rewind=1;
}

static void create_chbuttons(int *bits, int *rate, int *channels,
                             int *active){
  int i,fi;
  int ch=0;
  char buffer[160];

  groupboxes = calloc(plot_inputs,sizeof(*groupboxes));
  chbuttons = calloc(plot_ch,sizeof(*chbuttons));
  for(fi=0;fi<plot_inputs;fi++){
    GtkWidget *al=groupboxes[fi]=gtk_alignment_new(0,0,1,0);
    GtkWidget *vbox=gtk_vbox_new(0,0);
    GtkWidget *label;

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
      GtkWidget *button=chbuttons[i]=gtk_toggle_button_new();

      sprintf(buffer,"channel %d", i-ch);
      gtk_button_set_label(GTK_BUTTON(button),buffer);

      if(active)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),active[i]);
      else
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),1);
      g_signal_connect (G_OBJECT (button), "clicked",
                        G_CALLBACK (chlabels), NULL);
      gtk_box_pack_start(GTK_BOX(vbox),button,0,0,0);
    }

    gtk_container_add(GTK_CONTAINER(al),vbox);
    gtk_alignment_set_padding(GTK_ALIGNMENT(al),0,10,0,0);
    gtk_box_pack_start(GTK_BOX(rightbox),al,0,0,0);

    ch+=channels[fi];
    gtk_widget_show_all(al);
  }
}

static void destroy_chbuttons(){
  int fi;

  for(fi=0;fi<plot_inputs;fi++){
    gtk_widget_destroy(groupboxes[fi]);
    groupboxes[fi]=NULL;
  }

  free(groupboxes);
  free(chbuttons);
  groupboxes=NULL;
  chbuttons=NULL;
}

static plotparams pp;
void replot(void){
  int i,process[plot_ch],old_ch=plot_ch;

  for(i=0;i<plot_ch;i++)
    process[i]=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbuttons[i]));

  /* update the waveform display; send new data */
  fetchdata *f = process_fetch (plot_span, plot_scale, plot_range, process);
  if(!f)return;

  /* the fetched data may indicate the underlying file data has
     changed... */
  if(f->reload){

    /* remove old group labels and channel buttons */
    destroy_chbuttons();

    plot_ch=f->total_ch;
    plot_inputs=f->groups;

    /* create new buttons/labels */
    {
      int newprocess[f->total_ch];
      for(i=0;i<plot_ch && i<old_ch;i++)
        newprocess[i]=process[i];
      for(;i<plot_ch;i++)
        newprocess[i]=0;
      create_chbuttons(f->bits,f->rate,f->channels,newprocess);
    }
  }

  pp.bold=plot_bold;
  pp.trace_sep=plot_sep;
  pp.span=plot_span;
  pp.plotchoice=plot_type;
  pp.spanchoice=plot_spanchoice;
  pp.rangechoice=plot_rchoice;
  pp.scalechoice=plot_schoice;

  plot_draw(PLOT(plot),f,&pp);
}

static void shutdown(void){
  gtk_main_quit();
}

static gint watch_keyboard(GtkWidget *grab_widget,
                           GdkEventKey *event,
                           gpointer in){

  if(event->type == GDK_KEY_PRESS){
    if(event->state == GDK_CONTROL_MASK){
      if(event->keyval == GDK_w) { shutdown(); return TRUE; }
      if(event->keyval == GDK_q) { shutdown(); return TRUE; }
    }
  }
  return FALSE;
}

extern char *version;
void panel_create(void){
  int i;

  GdkWindow *root=gdk_get_default_root_window();
  GtkWidget *topbox=gtk_hbox_new(0,0);
  GtkWidget *rightframe=gtk_frame_new (NULL);
  GtkWidget *righttopbox=gtk_vbox_new(0,0);
  GtkWidget *rightframebox=gtk_event_box_new();
  GtkWidget *lefttable=gtk_table_new(4,3,0);
  GtkWidget *plot_control_al;
  GtkWidget *wbold;
  GtkWidget *triggerbutton;

  rightbox=gtk_vbox_new(0,0);

  toplevel=gtk_window_new (GTK_WINDOW_TOPLEVEL);
  group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW(toplevel), group);
  gtk_window_set_title(GTK_WINDOW(toplevel),(const gchar *)"Waveform Viewer");
  gtk_window_set_default_size(GTK_WINDOW(toplevel),1024,400);
  //gtk_widget_set_size_request(GTK_WIDGET(toplevel),1024,400);
  gtk_container_add (GTK_CONTAINER (toplevel), topbox);
  g_signal_connect (G_OBJECT (toplevel), "delete_event",
		    G_CALLBACK (shutdown), NULL);
  gtk_widget_set_name(topbox,"panel");

  /* underlying boxes/frames */
  gtk_box_pack_start(GTK_BOX(topbox),lefttable,1,1,0);
  gtk_box_pack_start(GTK_BOX(topbox),righttopbox,0,0,0);

  /* plot control checkboxes */
  {
    GtkWidget *al=plot_control_al=gtk_alignment_new(0,0,0,0);
    GtkWidget *box=gtk_hbox_new(0,6);
    GtkWidget *hold_display=
      gtk_check_button_new_with_mnemonic("_hold display");
    GtkWidget *trace_sep=
      gtk_check_button_new_with_mnemonic("trace _separation");
    wbold=gtk_check_button_new_with_mnemonic("_bold");
    gtk_table_attach(GTK_TABLE (lefttable), al,1,2,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_container_add(GTK_CONTAINER (al),box);
    gtk_box_pack_start(GTK_BOX(box),hold_display,0,0,0);
    gtk_box_pack_start(GTK_BOX(box),wbold,0,0,0);
    gtk_box_pack_start(GTK_BOX(box),trace_sep,0,0,0);

    gtk_widget_set_name(hold_display,"top-control");
    gtk_widget_set_name(wbold,"top-control");
    gtk_widget_set_name(trace_sep,"top-control");
    g_signal_connect (G_OBJECT (hold_display), "clicked",
                      G_CALLBACK (holdchange), NULL);
    gtk_widget_add_accelerator (hold_display, "activate", group, GDK_h, 0, 0);
    g_signal_connect (G_OBJECT (wbold), "clicked",
                      G_CALLBACK (boldchange), NULL);
    gtk_widget_add_accelerator (wbold, "activate", group, GDK_b, 0, 0);
    g_signal_connect (G_OBJECT (trace_sep), "clicked",
                      G_CALLBACK (sepchange), NULL);
    gtk_widget_add_accelerator (trace_sep, "activate", group, GDK_s, 0, 0);
  }

  /* add the waveform plot box */
  plot=plot_new();
  gtk_table_attach_defaults (GTK_TABLE (lefttable), plot,1,3,2,3);
  gtk_table_set_row_spacing (GTK_TABLE (lefttable), 2, 6);
  gtk_table_set_col_spacing (GTK_TABLE (lefttable), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (lefttable), 2, 2);

  /* right control frame */
  gtk_container_set_border_width (GTK_CONTAINER (righttopbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (rightbox), 6);
  gtk_frame_set_shadow_type(GTK_FRAME(rightframe),GTK_SHADOW_ETCHED_IN);
  gtk_widget_set_name(rightframebox,"controlpanel");
  gtk_box_pack_end(GTK_BOX (righttopbox),rightframebox,1,1,0);
  gtk_container_add (GTK_CONTAINER (rightframebox),rightframe);
  gtk_container_add (GTK_CONTAINER (rightframe), rightbox);

  /* the Fucking Fish */
  {
    GtkWidget *toptable = gtk_table_new(2,1,0);
    GtkWidget *fishbox=gtk_alignment_new(.5,.5,0,0);
    GtkWidget *sepbox=gtk_alignment_new(.5,.85,.7,0);
    GtkWidget *topsep=gtk_hseparator_new();
    GdkPixmap *tb;
    GdkPixmap *tp=gdk_pixmap_create_from_xpm_d(root,&tb,NULL,fisharray_xpm);
    GdkGC *cgc=gdk_gc_new(tp);
    GdkGC *bgc=gdk_gc_new(tb);
    int w, h;

    gdk_drawable_get_size(tp,&w,&h);
    w/=19;

    for(i=0;i<19;i++){
      ff[i]=gdk_pixmap_new(tp,w,h,-1);
      fb[i]=gdk_pixmap_new(tb,w,h,-1);
      gdk_draw_drawable(ff[i],cgc,tp,i*w,0,0,0,w,h);
      gdk_draw_drawable(fb[i],bgc,tb,i*w,0,0,0,w,h);
    }

    g_object_unref(cgc);
    g_object_unref(bgc);
    g_object_unref(tp);
    g_object_unref(tb);

    twirlimage=gtk_image_new_from_pixmap(ff[0],fb[0]);

    gtk_container_set_border_width (GTK_CONTAINER (toptable), 1);
    gtk_box_pack_start(GTK_BOX(righttopbox),toptable,0,0,0);
    gtk_container_add (GTK_CONTAINER (sepbox), topsep);
    gtk_container_add(GTK_CONTAINER(fishbox),twirlimage);
    gtk_table_attach_defaults (GTK_TABLE (toptable), fishbox,0,1,0,1);
    gtk_table_attach_defaults (GTK_TABLE (toptable), sepbox,0,1,0,1);
    gtk_table_set_row_spacing (GTK_TABLE (toptable), 0, 6);
  }

  create_chbuttons(bits,rate,channels,NULL);

  /* add the action buttons */
  GtkWidget *bbox=gtk_vbox_new(0,0);

  {
    /* range */
    GtkWidget *menu=rangemenu=gtk_combo_box_new_text();
    char *entries[]={"","","","","","","","","","",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_box_pack_start(GTK_BOX(bbox),menu,1,1,0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (rangechange), NULL);

  }

  {
    /* scale */
    GtkWidget *menu=scalemenu=gtk_combo_box_new_text();
    char *entries[]={"linear","-65dB","-96dB","-120dB","-160dB",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_box_pack_start(GTK_BOX(bbox),menu,1,1,0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (scalechange), NULL);
  }

  gtk_combo_box_set_active(GTK_COMBO_BOX(scalemenu),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(rangemenu),4);

  /* plot type */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"zero-hold","interpolated","lollipop",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (plotchange), NULL);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
  }

  {
    GtkWidget *sep=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(bbox),sep,0,0,4);
  }

  /* span */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"100ms/div",
                     "50ms/div",
                     "20ms/divn",
                     "10ms/div",
                     "5ms/div",
                     "2ms/div",
                     "1ms/div",
                     "500\xCE\xBCs/div",
                     "200\xCE\xBCs/div",
                     "100\xCE\xBCs/div",
                     "50\xCE\xBCs/div",
                     "20\xCE\xBCs/div",
                     "10\xCE\xBCs/div",
                     NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (spanchange), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),8);
    gtk_box_pack_start(GTK_BOX(bbox),menu,1,1,0);
  }

  /* trigger */
  {
    GtkWidget *menu=triggerbutton=gtk_combo_box_new_text();
    char *entries[]={"free run","0\xE2\x86\x91 trigger","0\xE2\x86\x93 trigger",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (triggerchange), NULL);
    gtk_box_pack_start(GTK_BOX(bbox),menu,1,1,0);
  }

  /* interval */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"1s",
                      "500ms","200ms","100ms",
                      "50ms","20ms","10ms",
                      "5ms","2ms","1ms",
                      "500\xCE\xBCs","200\xCE\xBCs",
                      "100\xCE\xBCs",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (intervalchange), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),3);
    gtk_box_pack_start(GTK_BOX(bbox),menu,1,1,0);
  }

  {
    GtkWidget *sep=gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(bbox),sep,0,0,4);
  }

  /* run/pause */
  {
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("_run");
    gtk_widget_add_accelerator (button, "activate", group, GDK_space, 0, 0);
    gtk_widget_add_accelerator (button, "activate", group, GDK_r, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (runchange), NULL);
    gtk_box_pack_start(GTK_BOX(bbox),button,0,0,0);
    run=button;
  }

  /* loop */
  /* rewind */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("_loop");
    gtk_widget_add_accelerator (button, "activate", group, GDK_l, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (loopchange), NULL);
    gtk_widget_set_sensitive(button,global_seekable);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);

    button=gtk_button_new_with_mnemonic("re_wind");
    gtk_widget_add_accelerator (button, "activate", group, GDK_w, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (rewindchange), NULL);
    gtk_widget_set_sensitive(button,global_seekable);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);
    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }

  gtk_box_pack_end(GTK_BOX(rightbox),bbox,0,0,0);
  gtk_widget_show_all(toplevel);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wbold),plot_bold);

  gtk_key_snooper_install(watch_keyboard,NULL);

  gtk_alignment_set_padding(GTK_ALIGNMENT(plot_control_al),0,0,plot_get_left_pad(PLOT(plot)),0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(triggerbutton),1);

}

static gboolean async_event_handle(GIOChannel *channel,
				   GIOCondition condition,
				   gpointer data){
  char buf[1];

  /* read all pending */
  while(read(eventpipe[0],buf,1)>0);

  increment_fish=1;

  /* check playback status and update the run button if needed */
  if(process_active && run && 
     !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(run)))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(run),1);
  if(!process_active && run && 
     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(run)))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(run),0);

  /* update the waveform display; send new data */
  if(!plot_hold)replot();

  /* if we're near CPU limit, service the rest of Gtk over next async
     update request */
  while (gtk_events_pending())
    gtk_main_iteration();

  return TRUE;
}

static int look_for_gtkrc(char *filename){
  FILE *f=fopen(filename,"r");
  if(!f)return 0;
  fprintf(stderr,"Loading waveform-gtkrc file found at %s\n",filename);
  gtk_rc_add_default_file(filename);
  return 1;
}

#define iSTR(x) #x
#define STR(x) iSTR(x)

void panel_go(int argc,char *argv[]){
  char *homedir=getenv("HOME");
  int found=0;

  found|=look_for_gtkrc(STR(ETCDIR)"/waveform-gtkrc");
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
  
    fprintf(stderr,
            "Could not find the waveform-gtkrc configuration file normally\n"
	    "installed in one of the following places:\n"

	    "\t./waveform-gtkrc\n"
	    "\t$(SPECTRUM_RCDIR)/waveform-gtkrc\n"
	    "\t~/.spectrum/waveform-gtkrc\n\t"
	    STR(ETCDIR)"/wavegform-gtkrc\n"
	    "This configuration file is used to tune the color, "
            "font and other detail aspects\n"
	    "of the user interface.  Although the viewer will "
            "work without it, the UI\n"
	    "appearence will likely make the application harder to "
            "use due to missing visual\n"
	    "cues.\n");
  }

  gtk_init (&argc, &argv);

  plot_ch = total_ch; /* true now, won't necessarily be true later */
  plot_inputs = inputs; /* true now, won't necessarily be true later */

  panel_create();
  animate_fish();

  /* set up watching the event pipe */
  {
    GIOChannel *channel = g_io_channel_unix_new (eventpipe[0]);
    g_io_channel_set_encoding (channel, NULL, NULL);
    g_io_channel_set_buffered (channel, FALSE);
    g_io_channel_set_close_on_unref (channel, TRUE);
    g_io_add_watch (channel, G_IO_IN, async_event_handle, NULL);
    g_io_channel_unref (channel);
  }

  /* we want to be running by default */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(run),TRUE);
  gtk_main ();

}

