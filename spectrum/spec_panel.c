/*
 *
 *  gtk2 spectrum analyzer
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

#include "analyzer.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "fisharray.h"
#include "spec_plot.h"

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
GtkWidget *bwtable;
GtkWidget *bwbutton;
GtkWidget *detectorbutton;
GtkWidget *modebutton;
GtkWidget *clearbutton;
GtkWidget *plot_label_al;

int plot_ch=0;
int plot_inputs=0;

int plot_scale=0;
int plot_mode=0;
int plot_mode_save=0;
int plot_modes=4;
int plot_link=0;
int plot_hold=0;
int plot_lock_y=0;
int plot_depth=90;
int plot_noise=0;
int plot_bwchoice=0;
int plot_detchoice=0;
int plot_bold=0;

int no_replot=0;

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
      g_timeout_add(70,(GSourceFunc)reanimate_fish,NULL);
  }else{
    fishframe_init=1;
    fishframe_timer=
      g_timeout_add(rand()%1000*30,(GSourceFunc)reanimate_fish,NULL);
  }
}

/* autoscale movement calculation and damping */

typedef struct {
  float a;
  float b1;
  float b2;
  float x[2];
  float y[2];
} pole2;

static float plot_ymax_target;
static int plot_ymaxtimer;
static pole2 plot_ymax_damp;
static float plot_pmax_target;
static pole2 plot_pmax_damp;
static int plot_pmaxtimer;
static float plot_pmin_target;
static pole2 plot_pmin_damp;
static int plot_pmintimer;

static void filter_reset(pole2 *p, float val){
  p->x[0]=p->x[1]=val;
  p->y[0]=p->y[1]=val;
}

static void filter_make_critical(float w, pole2 *f){
  float w0 = tan(M_PI*w*pow(pow(2,.5)-1,-.5));
  f->a  = w0*w0/(1+(2*w0)+w0*w0);
  f->b1 = 2*f->a*(1/(w0*w0)-1);
  f->b2 = 1-(4*f->a+f->b1);
  filter_reset(f,0);
}

static float filter_filter(float x, pole2 *p){
  float y =
    p->a*x + 2*p->a*p->x[0] + p->a*p->x[1] +
    p->b1*p->y[0] + p->b2*p->y[1];
  p->y[1] = p->y[0]; p->y[0] = y;
  p->x[1] = p->x[0]; p->x[0] = x;
  return y;
}

#define HYSTERESIS_THRESHOLD .25
#define HYSTERESIS_TIMERFRAMES 30

static void calculate_autoscale (fetchdata *f,
                                 plotparams *pp,
                                 int request_reset){
  int phase = f->phase_active;
  int height = f->height;
  int plot_ymax_limit = (plot_depth>140 ? plot_depth : 140);
  float ymax = f->ymax;
  float pmax = f->pmax;
  float pmin = f->pmin;

  /* graph limit updates are conditional depending on mode/link */
  switch(f->link){
  case LINK_INDEPENDENT:
  case LINK_SUMMED:
  case LINK_PHASE:
    {
      float dBpp = (float)plot_depth/height;
      ymax += dBpp*25;
    }
    break;
  }

  if(ymax<plot_depth - plot_ymax_limit) ymax=plot_depth-plot_ymax_limit;
  if(ymax>plot_ymax_limit)ymax=plot_ymax_limit;

  pmax+=10;
  pmin-=10;
  if(pmax<10)pmax=10;
  if(pmax>190)pmax=190;
  if(pmin>-20)pmin=-20;
  if(pmin<-190)pmin=-190;

  /* phase/response zeros align on phase graphs; verify targets
     against phase constraints */
  if(phase){
    float pzero,mzero = height/plot_depth*ymax;

    /* move mag zero back onscreen if it's off */
    if(mzero < height*HYSTERESIS_THRESHOLD){
      ymax = (plot_depth*height*HYSTERESIS_THRESHOLD)/height;
    }
    if(mzero > height*(1-HYSTERESIS_THRESHOLD)){
      ymax = (plot_depth*height*(1-HYSTERESIS_THRESHOLD))/height;
    }

    mzero = height/plot_depth*ymax;
    pzero = height/(pmax-pmin)*pmax;

    if(mzero<pzero){
      /* straightforward; move the dB range down */
      ymax = pzero*plot_depth/height;
    }else{
      /* a little harder as phase has a min and a max.
         First increase the pmax to match the dB zero. */

      pmax = pmin/(1-height/mzero);
      pzero = height/(pmax-pmin)*pmax;

      /* That worked, but might have run pmax overrange */
      if(pmax>190.){
        /* reconcile by allowing mag to overrange */
        pmax = 190.;
        pzero = height/(pmax-pmin)*pmax;
        ymax = plot_depth*pzero/height;
        plot_ymaxtimer=0;
      }
    }
  }

  if(request_reset){
    pp->ymax=plot_ymax_target=ymax;
    filter_reset(&plot_ymax_damp,pp->ymax);
    plot_ymaxtimer=HYSTERESIS_TIMERFRAMES;

    if(phase){
      pp->pmax=plot_pmax_target=pmax;
      pp->pmin=plot_pmin_target=pmin;
      filter_reset(&plot_pmax_damp,pp->pmax);
      filter_reset(&plot_pmin_damp,pp->pmin);
      plot_pmaxtimer=HYSTERESIS_TIMERFRAMES;
      plot_pmintimer=HYSTERESIS_TIMERFRAMES;
    }

  }else{
    /* conditionally set new damped ymax target */
    if(plot_ymaxtimer>0)
      plot_ymaxtimer--;

    if(ymax > plot_ymax_target-plot_depth*HYSTERESIS_THRESHOLD)
      plot_ymaxtimer=HYSTERESIS_TIMERFRAMES;

    if(ymax > plot_ymax_target || plot_ymaxtimer<=0)
      plot_ymax_target=ymax;

    /* update ymax through scale damping filter */
    pp->ymax = filter_filter(plot_ymax_target,&plot_ymax_damp);

    /* apply same hyteresis and update to phase */
    if(phase){

      if(plot_pmaxtimer>0)
        plot_pmaxtimer--;
      if(plot_pmintimer>0)
        plot_pmintimer--;

      if(pmax > plot_pmax_target*(1-HYSTERESIS_THRESHOLD))
        plot_pmaxtimer=HYSTERESIS_TIMERFRAMES;
      if(pmax > plot_pmax_target || plot_pmaxtimer<=0)
        plot_pmax_target=pmax;

      if(pmin < plot_pmin_target*(1-HYSTERESIS_THRESHOLD))
        plot_pmintimer=HYSTERESIS_TIMERFRAMES;
      if(pmin < plot_pmin_target || plot_pmintimer<=0)
        plot_pmin_target=pmin;

      pp->pmax = filter_filter(plot_pmax_target,&plot_pmax_damp);
      pp->pmin = filter_filter(plot_pmin_target,&plot_pmin_damp);

    }
  }

  if(phase){
    /* when phase is active, the plot_ymax in use is dictated by
       plot_pmax/plot_pmin, which in turn already took the desired ymax
       into consideration above */
    pp->ymax = plot_depth*pp->pmax/(pp->pmax-pp->pmin);
  }
}

/* a gtk2 hack to override checkbox background color */
static void override_base(GtkWidget *w, int active){
  gtk_widget_modify_base
    (w, GTK_STATE_NORMAL,
     &w->style->bg[active?GTK_STATE_ACTIVE:GTK_STATE_NORMAL]);
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

static void chlabels(GtkWidget *widget,gpointer in){
  /* scan state, update labels on channel buttons, set sensitivity
     based on grouping and mode */
  int fi,ch,i;
  char buf[80];

  /* set sensitivity */
  switch(plot_link){
  case LINK_SUMMED: /* summing mode */
  case LINK_INDEPENDENT: /* normal/independent mode */

    ch=0;
    for(fi=0;fi<plot_inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++)
        gtk_widget_set_sensitive(chbuttons[i],1);
      ch+=channels[fi];
    }
    break;

  case LINK_PHASE: /* response/phase */
    ch=0;
    for(fi=0;fi<plot_inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++)
	if(channels[fi]<2){
	  gtk_widget_set_sensitive(chbuttons[i],0);
	}else{
	  if(i<ch+2){
	    gtk_widget_set_sensitive(chbuttons[i],1);
	  }else{
	    gtk_widget_set_sensitive(chbuttons[i],0);
	  }
	}
      ch+=channels[fi];
    }
    break;
  }

  /* set labels */
  switch(plot_link){

  case LINK_INDEPENDENT:
  case LINK_SUMMED:

    ch=0;
    for(fi=0;fi<plot_inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	sprintf(buf,"channel %d", i-ch);
	gtk_button_set_label(GTK_BUTTON(chbuttons[i]),buf);
      }
      ch+=channels[fi];
    }
    break;

  case LINK_PHASE:

    ch=0;
    for(fi=0;fi<plot_inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	if(channels[fi]<2){
	  gtk_button_set_label(GTK_BUTTON(chbuttons[i]),"unused");
	}else if(i==ch){
	  gtk_button_set_label(GTK_BUTTON(chbuttons[i]),"response");
	}else if(i==ch+1){
	  gtk_button_set_label(GTK_BUTTON(chbuttons[i]),"phase");
	}else{
	  gtk_button_set_label(GTK_BUTTON(chbuttons[i]),"unused");
	}
      }
      ch+=channels[fi];
    }
    break;
  }

  /* set colors */
  switch(plot_link){
  case LINK_SUMMED:
    ch=0;
    for(fi=0;fi<plot_inputs;fi++){
      GdkColor rgb = chcolor(ch);

      for(i=ch;i<ch+channels[fi];i++){
	GtkWidget *button=chbuttons[i];
	set_fg(button,&rgb);
      }
      ch+=channels[fi];
    }
    break;

  default:
    ch=0;
    for(fi=0;fi<plot_inputs;fi++){
      for(i=ch;i<ch+channels[fi];i++){
	GdkColor rgb = chcolor(i);
	GtkWidget *button=chbuttons[i];
	set_fg(button,&rgb);
      }
      ch+=channels[fi];
    }
    break;
  }

  if(widget)replot(0,1,0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(plot_label_al),0,0,0,
                            plot_get_right_pad(PLOT(plot)));
}

static void depthchange(GtkWidget *widget,gpointer in){
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
  replot(1,1,0);
}

static void scalechange(GtkWidget *widget,gpointer in){
  plot_scale=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  replot(0,0,0);
}

static void modechange(GtkWidget *widget,gpointer in){
  int ret=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  if(ret>=0)
    plot_mode_save=plot_mode=ret;

  replot(0,1,0);
}

static void linkchange(GtkWidget *widget,gpointer in){
  plot_link=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  chlabels(widget,NULL);
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
  override_base(widget,plot_hold);
  replot(0,1,0);
}

static void lockchange(GtkWidget *widget,gpointer in){
  plot_lock_y=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  override_base(widget,plot_lock_y);
  if(!plot_lock_y)replot(0,1,0);
}

static void boldchange(GtkWidget *widget,gpointer in){
  plot_bold=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  override_base(widget,plot_bold);
  replot(0,0,0);
}

static void loopchange(GtkWidget *widget,gpointer in){
  acc_loop=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void clearchange(GtkWidget *widget,gpointer in){
  rundata_clear();
  replot(0,0,0);
}

static void rewindchange(GtkWidget *widget,gpointer in){
  acc_rewind=1;
}

static void detectorchange(GtkWidget *widget,gpointer in){
  plot_detchoice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  replot(0,1,0);
}

static void bwchange(GtkWidget *widget,gpointer in){
  plot_bwchoice=gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  set_bandwidth(plot_bwchoice);
  replot(0,1,0);
}

/* rate, channels, bits, channel active buttons */
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
  chlabels(NULL,NULL);
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

/* scale_reset != 0  requests an instant optimal rescale */
/* inactive_reset    performs a reset only if processing is not active */
/*  scale_damp != 0  requests a normal animated rescaling operation.
                     It processing is not active, this is promoted to
                     an instant optimal rescale */
/* plot_lock_y overrides all scaling requests */

static plotparams pp;
static int oldphase=0;
void replot(int scale_reset, int inactive_reset, int scale_damp){
  int i,process[plot_ch],old_ch = plot_ch;
  if(no_replot)return;

  for(i=0;i<plot_ch;i++)
    process[i]=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chbuttons[i]));

  /* update the spectral display; send new data */
  fetchdata *f = process_fetch
    (plot_scale, plot_mode, plot_link, plot_detchoice,
     process, PLOT(plot));

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

  if(!plot_lock_y){
    if(scale_reset ||
       (f->phase_active != oldphase) ||
       (!process_active && inactive_reset))
      calculate_autoscale(f,&pp,1);
    else if(scale_damp && process_active)
      calculate_autoscale(f,&pp,0);
  }
  oldphase = f->phase_active;

  pp.depth=plot_depth;
  pp.bold=plot_bold;
  plot_draw(PLOT(plot),f,&pp);
}

static void shutdown(void){
  gtk_main_quit();
}

static void dump(GtkWidget *widget,gpointer in){
  process_dump(plot_mode);
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
  GtkWidget *righttopal=gtk_alignment_new(.5,.5,1.,1.);
  GtkWidget *righttopbox=gtk_vbox_new(0,0);
  GtkWidget *rightframebox=gtk_event_box_new();
  GtkWidget *lefttable=gtk_table_new(4,2,0);
  GtkWidget *plot_control_al;
  GtkWidget *wbold;

  rightbox=gtk_vbox_new(0,0);

  toplevel=gtk_window_new (GTK_WINDOW_TOPLEVEL);
  group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW(toplevel), group);
  gtk_window_set_title(GTK_WINDOW(toplevel),
                       (const gchar *)"Spectrum Analyzer");
  gtk_window_set_default_size(GTK_WINDOW(toplevel),1024,400);
  //gtk_widget_set_size_request(GTK_WIDGET(toplevel),1024,400);
  gtk_container_add (GTK_CONTAINER (toplevel), topbox);
  g_signal_connect (G_OBJECT (toplevel), "delete_event",
		    G_CALLBACK (shutdown), NULL);
  gtk_widget_set_name(topbox,"panel");

  /* underlying boxes/frames */
  gtk_box_pack_start(GTK_BOX(topbox),lefttable,1,1,0);
  gtk_box_pack_start(GTK_BOX(topbox),righttopal,0,0,0);
  gtk_container_add(GTK_CONTAINER(righttopal),righttopbox);

  /* plot control checkboxes */
  {
    GtkWidget *al=plot_control_al=gtk_alignment_new(0,0,0,0);
    GtkWidget *box=gtk_hbox_new(0,6);
    GtkWidget *lock_range=gtk_check_button_new_with_mnemonic("lock _Y range");
    GtkWidget *hold_display=gtk_check_button_new_with_mnemonic("_hold display");
    wbold=gtk_check_button_new_with_mnemonic("_bold");
    gtk_table_attach(GTK_TABLE (lefttable), al,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_container_add(GTK_CONTAINER (al),box);
    gtk_box_pack_start(GTK_BOX(box),lock_range,0,0,0);
    gtk_box_pack_start(GTK_BOX(box),hold_display,0,0,0);
    gtk_box_pack_start(GTK_BOX(box),wbold,0,0,0);

    gtk_widget_set_name(lock_range,"top-control");
    gtk_widget_set_name(hold_display,"top-control");
    gtk_widget_set_name(wbold,"top-control");
    g_signal_connect (G_OBJECT (hold_display), "clicked",
                      G_CALLBACK (holdchange), NULL);
    gtk_widget_add_accelerator (hold_display, "activate",
                                group, GDK_h, 0, 0);
    g_signal_connect (G_OBJECT (lock_range), "clicked",
                      G_CALLBACK (lockchange), NULL);
    gtk_widget_add_accelerator (lock_range, "activate",
                                group, GDK_y, 0, 0);
    gtk_widget_add_accelerator (lock_range, "activate",
                                group, GDK_Y, 0, 0);
    g_signal_connect (G_OBJECT (wbold), "clicked",
                      G_CALLBACK (boldchange), NULL);
    gtk_widget_add_accelerator (wbold, "activate",
                                group, GDK_b, 0, 0);

  }

  /* plot informational labels */
  {
    char buf[80];
    GtkWidget *al=plot_label_al=gtk_alignment_new(1,.5,0,0);
    GtkWidget *box=gtk_hbox_new(0,2);
    GtkWidget *text1=gtk_label_new("window:");
    GtkWidget *text2=gtk_label_new("sin^4  ");
    GtkWidget *text3=gtk_label_new("points:");

    snprintf(buf,80,"%d",blocksize);

    GtkWidget *text4=gtk_label_new(buf);
    gtk_table_attach(GTK_TABLE (lefttable), al,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
    gtk_container_add(GTK_CONTAINER (al),box);

    gtk_box_pack_end(GTK_BOX(box),text4,0,0,0);
    gtk_box_pack_end(GTK_BOX(box),text3,0,0,0);
    gtk_box_pack_end(GTK_BOX(box),text2,0,0,0);
    gtk_box_pack_end(GTK_BOX(box),text1,0,0,0);

    gtk_widget_set_name(text1,"top-label");
    gtk_widget_set_name(text2,"top-readout");
    gtk_widget_set_name(text3,"top-label");
    gtk_widget_set_name(text4,"top-readout");

  }

  /* add the spectrum plot box */
  plot=plot_new();
  gtk_table_attach_defaults (GTK_TABLE (lefttable), plot,0,1,2,3);
  gtk_table_set_row_spacing (GTK_TABLE (lefttable), 2, 4);
  //gtk_table_set_col_spacing (GTK_TABLE (lefttable), 0, 2);

  /* right control frame */
  gtk_alignment_set_padding(GTK_ALIGNMENT(righttopal),6,6,2,6);
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
    gtk_table_attach_defaults (GTK_TABLE (toptable), fishbox,0,1,1,2);
    gtk_table_attach_defaults (GTK_TABLE (toptable), sepbox,0,1,1,2);
    gtk_table_set_row_spacing (GTK_TABLE (toptable), 0, 6);
  }

  create_chbuttons(bits,rate,channels,NULL);

  /* add the action buttons */
  GtkWidget *bbox=gtk_vbox_new(0,0);

  {
    /* X scale */
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"log frequency","ISO log freq","linear freq",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),plot_scale);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (scalechange), NULL);
  }

  {
    /* bandwidth */
    GtkWidget *menu=bwbutton=gtk_combo_box_new_text();
    for(i=0;bw_entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), bw_entries[i]);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    g_signal_connect (G_OBJECT (menu), "changed",
                      G_CALLBACK (bwchange), NULL);
  }

  {
    /* bin display mode */
    GtkWidget *menu=detectorbutton=gtk_combo_box_new_text();
    GList *cell_list =
      gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(menu));
    if(cell_list && cell_list->data){
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(menu),
                                     cell_list->data,"markup",0,NULL);
    }

    for(i=0;det_entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), det_entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    g_signal_connect (G_OBJECT (menu), "changed",
                      G_CALLBACK (detectorchange), NULL);
  }

  {
    /* depth */
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"1dB","10dB","20dB","45dB","90dB",
                     "140dB","200dB",NULL};
    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),4);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    g_signal_connect (G_OBJECT (menu), "changed",
                      G_CALLBACK (depthchange), NULL);
  }

  {
    /* mode */
    GtkWidget *menu=modebutton=gtk_combo_box_new_text();
    for(i=0;mode_entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), mode_entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),plot_mode);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);
    g_signal_connect (G_OBJECT (menu), "changed",
                      G_CALLBACK (modechange), NULL);
  }

  /* link */
  {
    GtkWidget *menu=gtk_combo_box_new_text();
    char *entries[]={"independent",
                     "sum channels",
                     "response/phase",
                     NULL};

    for(i=0;entries[i];i++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (menu), entries[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
    gtk_box_pack_start(GTK_BOX(bbox),menu,0,0,0);

    g_signal_connect (G_OBJECT (menu), "changed",
		      G_CALLBACK (linkchange), NULL);
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
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (runchange), NULL);
    gtk_box_pack_start(GTK_BOX(bbox),button,0,0,0);
    run=button;
  }

  /* loop */
  /* rewind */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("_loop");
    gtk_widget_add_accelerator (button, "activate", group, GDK_l, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (loopchange), NULL);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);
    gtk_widget_set_sensitive(button,global_seekable);


    button=gtk_button_new_with_mnemonic("re_wind");
    gtk_widget_add_accelerator (button, "activate", group, GDK_w, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (rewindchange), NULL);
    gtk_widget_set_sensitive(button,global_seekable);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);

    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }

  /* clear */
  /* dump */
  {
    GtkWidget *box=gtk_hbox_new(1,1);
    GtkWidget *button=clearbutton=gtk_button_new_with_mnemonic("_clear");
    gtk_widget_add_accelerator (button, "activate", group, GDK_c, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (clearchange), NULL);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);


    button=gtk_button_new_with_mnemonic("_export");
    gtk_widget_add_accelerator (button, "activate", group, GDK_e, 0, 0);
    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (dump), NULL);
    gtk_box_pack_start(GTK_BOX(box),button,1,1,0);

    gtk_box_pack_start(GTK_BOX(bbox),box,0,0,0);
  }

  /* noise floor */
#if 0
  {
    GtkWidget *button=gtk_toggle_button_new_with_mnemonic("sample _noise floor");
    gtk_widget_add_accelerator (button, "activate", group, GDK_n, 0, 0);
    g_signal_connect (G_OBJECT (button), "toggled", G_CALLBACK (noise), NULL);
    gtk_box_pack_start(GTK_BOX(bbox),button,0,0,0);
  }
#endif

  gtk_box_pack_end(GTK_BOX(rightbox),bbox,0,0,0);
  gtk_widget_show_all(toplevel);
  gtk_combo_box_set_active(GTK_COMBO_BOX(bwbutton),0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wbold),plot_bold);

  gtk_key_snooper_install(watch_keyboard,NULL);

  gtk_alignment_set_padding(GTK_ALIGNMENT(plot_control_al),0,0,plot_get_left_pad(PLOT(plot)),0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(plot_label_al),0,0,0,plot_get_right_pad(PLOT(plot)));

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

  /* update the spectral display; send new data */
  if(!plot_hold)replot(0,0,1);

  /* if we're near CPU limit, service the rest of Gtk over next async
     update request */
  while (gtk_events_pending())
    gtk_main_iteration();

  return TRUE;
}

static int look_for_gtkrc(char *filename){
  FILE *f=fopen(filename,"r");
  if(!f)return 0;
  fprintf(stderr,"Loading spectrum-gtkrc file found at %s\n",filename);
  gtk_rc_add_default_file(filename);
  return 1;
}

#define iSTR(x) #x
#define STR(x) iSTR(x)

void panel_go(int argc,char *argv[]){
  char *homedir=getenv("HOME");
  int found=0;

  found|=look_for_gtkrc(STR(ETCDIR)"/spectrum-gtkrc");
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

    fprintf(stderr,
            "Could not find the spectrum-gtkrc configuration file normally\n"
	    "installed in one of the following places:\n"

	    "\t./spectrum-gtkrc\n"
	    "\t$(SPECTRUM_RCDIR)/spectrum-gtkrc\n"
	    "\t~/.spectrum/spectrum-gtkrc\n\t"
	    STR(ETCDIR) "/spectrum-gtkrc\n"
	    "This configuration file is used to tune the color, "
            "font and other detail aspects\n"
	    "of the user interface.  Although the analyzer will "
            "work without it, the UI\n"
	    "appearence will likely make the application harder to "
            "use due to missing visual\n"
	    "cues.\n");
  }

  gtk_init (&argc, &argv);


  plot_ch = total_ch; /* true now, won't necessarily be true later */
  plot_inputs = inputs; /* true now, won't necessarily be true later */

  filter_make_critical(.04,&plot_ymax_damp);
  filter_make_critical(.04,&plot_pmax_damp);
  filter_make_critical(.04,&plot_pmin_damp);

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
