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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "wave_plot.h"

static GtkDrawingAreaClass *parent_class = NULL;

static void compute_xgrid(Plot *p){
  GtkWidget *widget=GTK_WIDGET(p);
  int width=widget->allocation.width-p->padx;
  if(p->width != width){
    int i,j;

    p->xgrids=11;
    p->xtics=30;

    for(i=0;i<p->xgrids;i++)
      p->xgrid[i]=rintf(i/(float)(p->xgrids-1) * (width-1))+p->padx;

    for(i=0,j=0;i<p->xtics;i++,j++){
      if(j%4==0)j++;
      p->xtic[i]=rintf(j/(float)((p->xgrids-1)*4) * (width-1))+p->padx;
    }
    p->width=width;
  }
}

GdkColor chcolor(int ch){
  GdkColor rgb={0,0,0,0};

  switch(ch%7){
  case 0:
    rgb.red=0x4000;
    rgb.green=0x4000;
    rgb.blue=0x4000;
    break;
  case 1:
    rgb.red=0xd000;
    rgb.green=0x0000;
    rgb.blue=0x0000;
    break;
  case 2:
    rgb.red=0x0000;
    rgb.green=0xb000;
    rgb.blue=0x0000;
    break;
  case 3:
    rgb.red=0x0000;
    rgb.green=0x0000;
    rgb.blue=0xf000;
    break;
  case 4:
    rgb.red=0xc000;
    rgb.green=0xc000;
    rgb.blue=0x0000;
    break;
  case 5:
    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    break;
  case 6:
    rgb.red=0xc000;
    rgb.green=0x0000;
    rgb.blue=0xe000;
    break;
  }

  return rgb;
}

void plot_draw(Plot *p, fetchdata *f, plotparams *pp){
  int i;
  GtkWidget *widget=GTK_WIDGET(p);
  GtkWidget *parent=gtk_widget_get_parent(widget);
  int height=widget->allocation.height;
  int width=widget->allocation.width;
  int padx = p->padx;
  int num_active=0;
  float center = (height-p->pady)/2.;

  if(!GDK_IS_DRAWABLE(p->backing))return;
  if(!pp)return;
  if(!f)return;

  /* how many channels actually active right now?  Need to know if
     trace sep is enabled */
  if(pp->trace_sep){
    int fi,ch=0;
    for(fi=0;fi<f->groups;fi++){
      for(i=ch;i<ch+f->channels[fi];i++)
        if(f->active[i])
          num_active++;
      ch+=f->channels[fi];
    }
  }

  if(!p->drawgc){
    p->drawgc=gdk_gc_new(p->backing);
    p->twogc=gdk_gc_new(p->backing);
    gdk_gc_copy(p->drawgc,widget->style->black_gc);
    gdk_gc_copy(p->twogc,widget->style->black_gc);
  }

  if(pp->plotchoice==2){
    gdk_gc_set_line_attributes(p->twogc,pp->bold+1,GDK_LINE_SOLID,
                               GDK_CAP_BUTT,GDK_JOIN_MITER);
  }else{
    gdk_gc_set_line_attributes(p->twogc,pp->bold+1,GDK_LINE_SOLID,
                               GDK_CAP_PROJECTING,GDK_JOIN_MITER);
  }

  /* clear the old rectangle out */
  {
    GdkGC *gc=parent->style->bg_gc[0];
    gdk_draw_rectangle(p->backing,gc,1,0,0,padx,height);
    gdk_draw_rectangle(p->backing,gc,1,0,height-p->pady,width,p->pady);

    gc=parent->style->white_gc;
    gdk_draw_rectangle(p->backing,gc,1,padx,0,width-padx,height-p->pady);
  }

  compute_xgrid(p);

  /* draw the light x grid */
  {
    int i;
    GdkColor rgb={0,0,0,0};

    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<p->xtics;i++)
      gdk_draw_line(p->backing,p->drawgc,p->xtic[i],0,p->xtic[i],height-p->pady-1);
  }

  /* draw the light y grid */
  {
    GdkColor rgb={0,0,0,0};

    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=1;i<8;i++){
      int y1=rintf(center + center*i/9.);
      int y2=rintf(center - center*i/9.);
      gdk_draw_line(p->backing,p->drawgc,padx,y1,width,y1);
      gdk_draw_line(p->backing,p->drawgc,padx,y2,width,y2);
    }
  }

  /* dark y grid */
  {
    GdkColor rgb={0,0,0,0};
    int px,py;

    gdk_draw_line(p->backing,p->drawgc,padx,rintf(center-1),width,rintf(center-1));
    gdk_draw_line(p->backing,p->drawgc,padx,rintf(center+1),width,rintf(center+1));

    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=-8;i<9;i+=4){
      int y=rintf(center + center*i/9.);

      gdk_draw_line(p->backing,p->drawgc,padx,y,width,y);

    }
  }

  /* dark x grid */
  {
    int i;
    GdkColor rgb={0,0,0,0};

    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<p->xgrids;i++)
      gdk_draw_line(p->backing,p->drawgc,p->xgrid[i],0,p->xgrid[i],
                    height-p->pady-1);
  }

  /* center/zero line */
  {
    gdk_draw_line(p->backing,widget->style->black_gc,padx,
                  rintf(center),width,rintf(center));
  }

  /* draw actual data */
  if(f->data){
    int ch=0,ach=0,fi,i,k;
    const GdkRectangle clip = {p->padx,0,width-p->padx,height-p->pady};
    GdkColor rgb;

    gdk_gc_set_clip_rectangle (p->twogc, &clip);

    for(fi=0;fi<f->groups;fi++){
      int spann = ceil(f->rate[fi]/1000000.*pp->span)+1;

      for(i=ch;i<ch+f->channels[fi];i++){

        if(f->active[i]){
          float *data=f->data[i];
          int wp=width-p->padx;
          float spani = 1000000./f->span/f->rate[fi]*wp;
          int hp=height-p->pady;
          float ym=hp*-8./36;
          float cp = pp->trace_sep ?
            (height-p->pady)*(16*i+8)/(float)(18*num_active)+
            (height-p->pady)/18. : center;

          ach++;
          rgb = chcolor(i);
          gdk_gc_set_rgb_fg_color(p->twogc,&rgb);

          switch(pp->plotchoice){
          case 0: /* zero-hold */
            {
              int x0=-1;
              float yH=NAN,yL=NAN,y0=NAN;
              int acc=0;
              for(k=0;k<spann;k++){
                int x1 = rintf(k*spani);
                float y1 = data[k]*ym;

                /* clamp for shorts in the X protocol; gdk does not guard */
                if(y1>20000.f)y1=20000.f;
                if(y1<-20000.f)y1=-20000.f;

                if(x1>x0){
                  if(acc>1){
                    if(!isnan(yL)&&!isnan(yH))
                      gdk_draw_line(p->backing,p->twogc,
                                    x0+padx,rintf(yL+cp),x0+padx,
                                    rintf(yH+cp));
                  }
                  if(!isnan(y0)){
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rintf(y0+cp),x1+padx,rintf(y0+cp));

                    if(!isnan(y1))
                      gdk_draw_line(p->backing,p->twogc,
                                    x1+padx,rintf(y0+cp),x1+padx,rintf(y1+cp));
                  }

                  acc=1;
                  yH=yL=y1;
                }else{
                  acc++;
                  if(!isnan(y1)){
                    if(y1<yL || isnan(yL))yL=y1;
                    if(y1>yH || isnan(yH))yH=y1;
                  }
                }
                x0=x1;
                y0=y1;
              }
              {
                int x1 = rintf(k*spani);

                if(x1<=x0 || acc>1){
                  if(!isnan(yL)&&!isnan(yH))
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rintf(yL+cp),x0+padx,rintf(yH+cp));
                }
              }
            }
            break;
          case 1: /* linear interpolation (first-order hold) */
            {
              int x0=-1;
              float yH=NAN,yL=NAN,y0=NAN;
              int acc=0;
              for(k=0;k<spann;k++){
                int x1 = rintf(k*spani);
                float y1 = data[k]*ym;

                /* clamp for shorts in the X protocol; gdk does not guard */
                if(y1>20000.f)y1=20000.f;
                if(y1<-20000.f)y1=-20000.f;

                if(x1>x0){
                  if(acc>1){
                    if(!isnan(yL) && !isnan(yH))
                      gdk_draw_line(p->backing,p->twogc,
                                    x0+padx,rintf(yL+cp),x0+padx,
                                    rintf(yH+cp));
                  }
                  if(!isnan(y0) && !isnan(y1)){
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rintf(y0+cp),x1+padx,rintf(y1+cp));
                  }

                  acc=1;
                  yH=yL=y1;
                }else{
                  acc++;
                  if(!isnan(y1)){
                    if(y1<yL || isnan(yL))yL=y1;
                    if(y1>yH || isnan(yH))yH=y1;
                  }
                }
                x0=x1;
                y0=y1;
              }
              {
                int x1 = rintf(k*spani);

                if(x1<=x0 || acc>1){
                  if(!isnan(yL) && !isnan(yH))
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rintf(yL+cp),x0+padx,rintf(yH+cp));
                }
              }
            }
            break;
          case 2: /* lollipop */
            {
              int x0=-1;
              float yH=NAN,yL=NAN;

              rgb.red=0x8000;
              rgb.green=0x8000;
              rgb.blue=0x8000;
              gdk_gc_set_rgb_fg_color(p->twogc,&rgb);

              for(k=0;k<spann;k++){
                int x1 = rintf(k*spani);
                float y1 = data[k]*ym;

                /* clamp for shorts in the X protocol; gdk does not guard */
                if(y1>20000.f)y1=20000.f;
                if(y1<-20000.f)y1=-20000.f;

                if(x1>x0){
                  if(!isnan(yL) || !isnan(yH)){
                    if(isnan(yL) || yL>0)yL=pp->bold;
                    if(isnan(yH) || yH<0)yH=0;
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rintf(yL+cp),x0+padx,
                                  rintf(yH+cp));
                  }
                  yH=yL=y1;
                }else{
                  if(!isnan(y1)){
                    if(y1<yL || isnan(yL))yL=y1;
                    if(y1>yH || isnan(yH))yH=y1;
                  }
                }
                x0=x1;
              }
              {
                if(!isnan(yL) || !isnan(yH)){
                  if(isnan(yL) || yL>0)yL=pp->bold;
                  if(isnan(yH) || yH<0)yH=0;
                  gdk_draw_line(p->backing,p->twogc,
                                x0+padx,rintf(yL+cp),x0+padx,
                                rintf(yH+cp));
                }
              }

              rgb = chcolor(i);
              gdk_gc_set_rgb_fg_color(p->twogc,&rgb);

              for(k=0;k<spann;k++){
                int x = rintf(k*spani);
                float y = data[k]*ym;

                /* clamp for shorts in the X protocol; gdk does not guard */
                if(y>20000.f)y=20000.f;
                if(y<-20000.f)y=-20000.f;

                if(!isnan(y)){
                  gdk_draw_arc(p->backing,p->twogc,
                               0,x+padx-5,rintf(y+cp-5),
                               9,9,0,23040);
                }
              }
            }
            break;
          }
        }
      }
      ch+=f->channels[fi];
    }
  }

  gdk_draw_drawable(widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    p->backing,
                    0, 0,
                    0, 0,
                    width, height);
}

static gboolean expose( GtkWidget *widget, GdkEventExpose *event ){
  Plot *p=PLOT(widget);
  gdk_draw_drawable(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->backing,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);

  return FALSE;
}

static void size_request (GtkWidget *widget,GtkRequisition *requisition){
  Plot *p=PLOT(widget);
  requisition->width = 400;
  requisition->height = 200;
  int axisy=0,axisx=0,pady=0,padx=0,px,py,i;

  if(requisition->width<axisx+padx)requisition->width=axisx+padx;
  if(requisition->height<axisy+pady)requisition->height=axisy+pady;
  p->padx=padx;
  p->pady=pady;
}

static gboolean configure(GtkWidget *widget, GdkEventConfigure *event){
  Plot *p=PLOT(widget);
  if (p->backing)
    g_object_unref(p->backing);

  p->backing = gdk_pixmap_new(widget->window,
			      widget->allocation.width,
			      widget->allocation.height,
			      -1);

  p->configured=1;
  replot();
  return TRUE;
}

static void plot_class_init (PlotClass *class){
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  parent_class = g_type_class_peek_parent (class);

  widget_class->expose_event = expose;
  widget_class->configure_event = configure;
  widget_class->size_request = size_request;
}

static void plot_init (Plot *p){
}

GType plot_get_type (void){
  static GType m_type = 0;
  if (!m_type){
    static const GTypeInfo m_info={
      sizeof (PlotClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) plot_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (Plot),
      0,
      (GInstanceInitFunc) plot_init,
      0
    };

    m_type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "Plot",
                                     &m_info, 0);
  }

  return m_type;
}

GtkWidget* plot_new (void){
  GtkWidget *ret= GTK_WIDGET (g_object_new (plot_get_type (), NULL));
  Plot *p=PLOT(ret);
  int i,j;

  return ret;
}

int plot_get_left_pad (Plot *m){
  return m->padx;
}
