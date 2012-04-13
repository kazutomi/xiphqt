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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "wave_plot.h"

static GtkDrawingAreaClass *parent_class = NULL;

static void compute_metadata(GtkWidget *widget){
  Plot *p=PLOT(widget);
  int width=widget->allocation.width-p->padx;
  int i,j;

  p->xgrids=11;
  p->xtics=30;

  for(i=0;i<p->xgrids;i++)
    p->xgrid[i]=rint(i/(float)(p->xgrids-1) * (width-1))+p->padx;

  for(i=0,j=0;i<p->xtics;i++,j++){
    if(j%4==0)j++;
    p->xtic[i]=rint(j/(float)((p->xgrids-1)*4) * (width-1))+p->padx;
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

static void draw(GtkWidget *widget){
  int i;
  Plot *p=PLOT(widget);
  int height=widget->allocation.height;
  int width=widget->allocation.width;
  GtkWidget *parent=gtk_widget_get_parent(widget);
  int padx = p->padx;

  if(!p->drawgc){
    GdkGCValues values;
    p->drawgc=gdk_gc_new(p->backing);
    p->twogc=gdk_gc_new(p->backing);
    gdk_gc_copy(p->drawgc,widget->style->black_gc);
    gdk_gc_copy(p->twogc,widget->style->black_gc);

    gdk_gc_set_line_attributes(p->twogc,2,GDK_LINE_SOLID,GDK_CAP_PROJECTING,
                               GDK_JOIN_MITER);
  }

  /* clear the old rectangle out */
  {
    GdkGC *gc=parent->style->bg_gc[0];
    gdk_draw_rectangle(p->backing,gc,1,0,0,padx,height);
    gdk_draw_rectangle(p->backing,gc,1,0,height-p->pady,width,p->pady);

    gc=parent->style->white_gc;
    gdk_draw_rectangle(p->backing,gc,1,padx,0,width-padx,height-p->pady);
  }

  /* draw the light x grid */
  {
    int i;
    GdkColor rgb={0,0,0,0};

    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<p->xtics;i++)
      gdk_draw_line(p->backing,p->drawgc,p->xtic[i],0,p->xtic[i],height-p->pady);
  }

  PangoLayout **proper=p->x_layout[p->spanchoice];

  for(i=0;i<p->xgrids;i++){
    int px,py;
    pango_layout_get_pixel_size(proper[i],&px,&py);

    gdk_draw_layout (p->backing,
                     widget->style->black_gc,
                     p->xgrid[i]-(px/2), height-py+2,
                     proper[i]);
  }


  /* draw the light y grid */
  {
    GdkColor rgb={0,0,0,0};
    int center = (height-p->pady)/2;

    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=1;i<8;i++){
      int y1=rint(center + center*i/9);
      int y2=rint(center - center*i/9);
      gdk_draw_line(p->backing,p->drawgc,padx,y1,width,y1);
      gdk_draw_line(p->backing,p->drawgc,padx,y2,width,y2);
    }
  }

  /* dark y grid */
  {
    GdkColor rgb={0,0,0,0};
    int center = (height-p->pady)/2;
    int px,py;

    gdk_draw_line(p->backing,p->drawgc,padx,center-1,width,center-1);
    gdk_draw_line(p->backing,p->drawgc,padx,center+1,width,center+1);

    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=-8;i<9;i+=4){
      int y=rint(center + center*i/9);

      gdk_draw_line(p->backing,p->drawgc,padx,y,width,y);

      pango_layout_get_pixel_size(p->y_layout[p->rchoice][p->schoice][i/4+2],
                                  &px,&py);

      if(i<=0){
        rgb.red=0x0000;
        rgb.green=0x0000;
        rgb.blue=0x0000;
      }else{
        rgb.red=0xc000;
        rgb.green=0x0000;
        rgb.blue=0x0000;
      }
      gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

      gdk_draw_layout (p->backing,
                       p->drawgc,
                       padx-px-2, y-py/2,
                       p->y_layout[p->rchoice][p->schoice][i/4+2]);

      rgb.red=0x0000;
      rgb.green=0xc000;
      rgb.blue=0xc000;
      gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

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
      gdk_draw_line(p->backing,p->drawgc,p->xgrid[i],0,p->xgrid[i],height-p->pady);
  }

  /* zero line */
  {
    int center = (height-p->pady)/2;
    gdk_draw_line(p->backing,widget->style->black_gc,padx,center,width,center);
  }

  /* draw actual data */
  if(p->ydata){
    int ch=0,fi,i,j,k;
    const GdkRectangle clip = {p->padx,0,width-p->padx,height-p->pady};
    const GdkRectangle noclip = {0,0,width,height};
    GdkColor rgb;

    gdk_gc_set_clip_rectangle (p->twogc, &clip);
    //gdk_gc_set_clip_rectangle (p->drawgc, &clip);

    for(fi=0;fi<p->groups;fi++){
      int copies = (int)ceil(p->blockslice[fi]/p->overslice[fi]);
      int spann = ceil(p->rate[fi]/1000000.*p->span)+1;

      for(i=ch;i<ch+p->ch[fi];i++){
        if(p->ch_active[i]){
          int offset=0;
          rgb = chcolor(i);
          gdk_gc_set_rgb_fg_color(p->twogc,&rgb);
          //gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

          for(j=0;j<copies;j++){
            float *data=p->ydata[i]+offset;
            int wp=width-p->padx;
            float spani = 1000000./p->span/p->rate[fi]*wp;
            int hp=height-p->pady;
            int cp=hp/2;
            float ym=hp*8./18./p->range;

            switch(p->type){
            case 0: /* zero-hold */
              {
                int x0=-1;
                float yH=NAN,yL=NAN;
                int acc=0;
                for(k=0;k<spann;k++){
                  int x1 = rint(k*spani);
                  float y1 = data[k]*ym;

                  if(x1>x0){
                    if(acc>1){
                      if(!isnan(yL)&&!isnan(yH))
                        gdk_draw_line(p->backing,p->twogc,
                                      x0+padx,rint(yL)+cp,x0+padx,
                                      rint(yH)+cp);
                    }else{
                      if(!isnan(yL)){
                        gdk_draw_line(p->backing,p->twogc,
                                      x0+padx,rint(yL)+cp,x1+padx,rint(yL)+cp);

                        if(!isnan(yH))
                          gdk_draw_line(p->backing,p->twogc,
                                        x1+padx,rint(yL)+cp,x1+padx,rint(y1)+cp);
                      }
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
                }
                {
                  int x1 = rint(k*spani);

                  if(x1<=x0 || acc>1){
                    if(!isnan(yL)&&!isnan(yH))
                      gdk_draw_line(p->backing,p->twogc,
                                    x0+padx,rint(yL)+cp,x0+padx,rint(yH)+cp);
                  }else{
                    if(!isnan(yL)){
                      gdk_draw_line(p->backing,p->twogc,
                                    x0+padx,rint(yL)+cp,x1+padx,rint(yL)+cp);
                    }
                  }
                }
              }
              break;
            case 1: /* linear interpolation */
              {
                int x0=-1,x1=-1,x2;
                float y0=NAN,y1a=NAN,y1b=NAN,y1;

                for(k=1;k<spann;k++){
                  x2 = rint((k+1)*spani);
                  y1 = data[k]*ym;
                  if(x0<x1){
                    if(!isnan(y1) && !isnan(y0))
                      gdk_draw_line(p->backing,p->twogc,
                                    x0+padx,rint(y0)+cp,x1+padx,rint(y1)+cp);
                    y1a=y1b=y1;
                  }else{
                    if(!isnan(y1)){
                      if(y1<y1a || isnan(y1a))y1a=y1;
                      if(y1>y1b || isnan(y1b))y1b=y1;
                    }
                    if(x1<x2){
                      if(!isnan(y1a) && !isnan(y1b))
                        gdk_draw_line(p->backing,p->twogc,
                                      x1+padx,rint(y1a)+cp,x1+padx,rint(y1b)+cp);
                    }
                  }
                  x0=x1;x1=x2;y0=y1;
                }

                y1 = data[k]*ym;
                if(x0<x1){
                  if(!isnan(y1) && !isnan(y0))
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rint(y0)+cp,x1+padx,rint(y1)+cp);
                  y1a=y1b=y1;
                }else{
                  if(!isnan(y1)){
                    if(y1<y1a || isnan(y1a))y1a=y1;
                    if(y1>y1b || isnan(y1b))y1b=y1;
                  }
                  if(!isnan(y1a) && !isnan(y1b))
                    gdk_draw_line(p->backing,p->twogc,
                                  x1+padx,rint(y1a)+cp,x1+padx,rint(y1b)+cp);
                }
              }
              break;
            case 2: /* lollipop */

              {
                if(spani<1.){
                  int x0=-1;
                  float yH=NAN,yL=NAN;
                  int acc=0;
                  for(k=0;k<spann;k++){
                    int x1 = rint(k*spani);
                    float y1 = data[k]*ym;
                  
                    if(x1>x0){
                      /* once too dense, the lollipop graph drops back
                         to just lines */
                      if(isnan(yL) || yL>0)
                        yL=0;
                      if(isnan(yH) || yH<0)
                        yH=0;
                      gdk_draw_line(p->backing,p->twogc,
                                    x0+padx,rint(yL)+cp,x0+padx,
                                    rint(yH)+cp);
                      acc=1;
                      yH=yL=y1;
                    }else{
                      acc++;
                    }
                    if(!isnan(y1)){
                      if(y1<yL || isnan(yL))yL=y1;
                      if(y1>yH || isnan(yH))yH=y1;
                    }
                    x0=x1;
                  }
                  {
                    int x1 = rint(k*spani);
                    if(isnan(yL) || yL>0)
                      yL=0;
                    if(isnan(yH) || yH<0)
                      yH=0;
                    gdk_draw_line(p->backing,p->twogc,
                                  x0+padx,rint(yL)+cp,x0+padx,rint(yH)+cp);
                  }
                }else{
                  for(k=0;k<spann;k++){
                    int x = rint(k*spani);
                    float y = data[k]*ym;
                    if(!isnan(y)){
                      gdk_draw_line(p->backing,p->twogc,
                                    x+padx,cp,x+padx,rint(y)+cp);

                      gdk_draw_arc(p->backing,p->twogc,
                                   0,x+padx-5,rint(y)+cp-5,
                                   9,9,0,23040);
                    }
                  }
                }
              }
              break;
            }

            offset+=spann;
          }
        }
      }
      ch+=p->ch[fi];
    }
    //gdk_gc_set_clip_rectangle (p->drawgc, &noclip);
  }
}

static void draw_and_expose(GtkWidget *widget){
  Plot *p=PLOT(widget);
  if(!GDK_IS_DRAWABLE(p->backing))return;
  draw(widget);
  if(!GTK_WIDGET_DRAWABLE(widget))return;
  if(!GDK_IS_DRAWABLE(widget->window))return;
  gdk_draw_drawable(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->backing,
		    0, 0,
		    0, 0,
		    widget->allocation.width,
		    widget->allocation.height);
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
  int axisy=0,axisx=0,pady=0,padx=0,phax=0,px,py,i;

  /* find max X layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->x_layout[1][i];i++){
      pango_layout_get_pixel_size(p->x_layout[1][i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max+=maxy*2.;
    max*=i+1;
    if(axisx<max)axisx=max;
  }

  /* find max Y layout */
  {
    int max=0;
    for(i=0;p->y_layout[5][4][i];i++){
      pango_layout_get_pixel_size(p->y_layout[5][4][i],&px,&py);
      if(py>max)max=py;
      if(px>padx)padx=px;
    }
    axisy=(max)*8;
    if(axisy<max)axisy=max;
  }

  if(requisition->width<axisx+padx)requisition->width=axisx+padx;
  if(requisition->height<axisy+pady)requisition->height=axisy+pady;
  p->padx=padx;
  p->pady=pady;
  p->phax=phax;
}

static gboolean configure(GtkWidget *widget, GdkEventConfigure *event){
  Plot *p=PLOT(widget);

  if (p->backing)
    g_object_unref(p->backing);

  p->backing = gdk_pixmap_new(widget->window,
			      widget->allocation.width,
			      widget->allocation.height,
			      -1);

  p->ydata=NULL;
  p->configured=1;

  compute_metadata(widget);
  plot_refresh(p);
  draw_and_expose(widget);

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
    
    m_type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "Plot", &m_info, 0);
  }

  return m_type;
}

GtkWidget* plot_new (int size, int groups, int *channels, int *rate){
  GtkWidget *ret= GTK_WIDGET (g_object_new (plot_get_type (), NULL));
  Plot *p=PLOT(ret);
  int g,i,j;
  int ch=0;
  p->groups = groups;
  for(g=0;g<groups;g++)
    ch+=channels[g];

  p->total_ch = ch;

  p->ch=channels;
  p->rate=rate;
  p->size=size;

  /* generate all the text layouts we'll need */
  /* linear X scale */
  {
    char *labels[13][12]={
      {"","100ms","200ms","300ms","400ms","500ms","600ms","700ms","800ms","900ms","",""},
      {"","50ms","100ms","150ms","200ms","250ms","300ms","350ms","400ms","450ms","",""},
      {"","20ms","40ms","60ms","80ms","100ms","120ms","140ms","160ms","180ms","",""},
      {"","10ms","20ms","30ms","40ms","50ms","60ms","70ms","80ms","90ms","",""},
      {"","5ms","10ms","15ms","20ms","25ms","30ms","35ms","40ms","45ms","",""},
      {"","2ms","4ms","6ms","8ms","10ms","12ms","14ms","16ms","18ms","",""},
      {"","1ms","2ms","3ms","4ms","5ms","6ms","7ms","8ms","9ms","",""},
      {"",".5ms","1ms","1.5ms","2ms","2.5ms","3ms","3.5ms","4ms","4.5ms","",""},
      {"",".2ms",".4ms",".6ms",".8ms","1ms","1.2ms","1.4ms","1.6ms","1.8ms","",""},

      {"","100\xCE\xBCs","200\xCE\xBCs","300\xCE\xBCs","400\xCE\xBCs","500\xCE\xBCs",
       "600\xCE\xBCs","700\xCE\xBCs","800\xCE\xBCs","900\xCE\xBCs","",""},
      {"","50\xCE\xBCs","100\xCE\xBCs","150\xCE\xBCs","200\xCE\xBCs","250\xCE\xBCs",
       "300\xCE\xBCs","350\xCE\xBCs","400\xCE\xBCs","450\xCE\xBCs","",""},
      {"","20\xCE\xBCs","40\xCE\xBCs","60\xCE\xBCs","80\xCE\xBCs","100\xCE\xBCs",
       "120\xCE\xBCs","140\xCE\xBCs","160\xCE\xBCs","180\xCE\xBCs","",""},
      {"","10\xCE\xBCs","20\xCE\xBCs","30\xCE\xBCs","40\xCE\xBCs","50\xCE\xBCs",
       "60\xCE\xBCs","70\xCE\xBCs","80\xCE\xBCs","90\xCE\xBCs","",""}};

    p->x_layout=calloc(13,sizeof(*p->x_layout));
    for(i=0;i<13;i++){
      p->x_layout[i]=calloc(12,sizeof(**p->x_layout));
      for(j=0;j<11;j++)
        p->x_layout[i][j]=gtk_widget_create_pango_layout(ret,labels[i][j]);
    }
  }

  /* phase Y scale */
  {
    char *label1[6] = {"1.0","0.5","0.2","0.1",".01",".001"};
    char *label1a[6] = {"0.5","0.25","0.1","0.05",".005",".0005"};

    char *labeln1[6] = {"-1.0","-0.5","-0.2","-0.1","-.01","-.001"};
    char *labeln1a[6] = {"-0.5","-0.25","-0.1","-0.05","-.005","-.0005"};

    char *label2[6] = {"0dB","-6dB","-14dB","-20dB","-40dB","-60dB"};
    char *label3[5] = {"0","-65dB","-96dB","-120dB","-160dB"};

    int val2[6] = {0,-6,-14,-20,-40,-60};
    int val3[5] = {0,-65,-96,-120,-160};

    p->y_layout=calloc(6,sizeof(*p->y_layout));
    for(i=0;i<6;i++){
      p->y_layout[i]=calloc(5,sizeof(**p->y_layout));
      p->y_layout[i][0]=calloc(6,sizeof(***p->y_layout));

      p->y_layout[i][0][0]=gtk_widget_create_pango_layout(ret,label1[i]);
      p->y_layout[i][0][1]=gtk_widget_create_pango_layout(ret,label1a[i]);
      p->y_layout[i][0][2]=gtk_widget_create_pango_layout(ret,label3[0]);
      p->y_layout[i][0][3]=gtk_widget_create_pango_layout(ret,labeln1a[i]);
      p->y_layout[i][0][4]=gtk_widget_create_pango_layout(ret,labeln1[i]);

      for(j=1;j<5;j++){
        char buf[10];
        p->y_layout[i][j]=calloc(6,sizeof(***p->y_layout));
        p->y_layout[i][j][0]=gtk_widget_create_pango_layout(ret,label2[i]);
        p->y_layout[i][j][2]=gtk_widget_create_pango_layout(ret,label3[j]);
        p->y_layout[i][j][4]=gtk_widget_create_pango_layout(ret,label2[i]);
        sprintf(buf,"%ddB",(val2[i]+val3[j])/2);
        p->y_layout[i][j][1]=gtk_widget_create_pango_layout(ret,buf);
        p->y_layout[i][j][3]=gtk_widget_create_pango_layout(ret,buf);
      }
    }
  }

  p->ch_active=calloc(ch,sizeof(*p->ch_active));
  
  plot_clear(p);
  return ret;
}

void plot_refresh (Plot *p){
  float ymax,pmax,pmin;
  int width=GTK_WIDGET(p)->allocation.width-p->padx;
  int height=GTK_WIDGET(p)->allocation.height-p->pady;
  float **data;
  float *floor;

  if(!p->configured)return;

  data = process_fetch(p->blockslice, p->overslice, p->span);
  p->ydata=data;
}

void plot_clear (Plot *p){
  GtkWidget *widget=GTK_WIDGET(p);
  int width=GTK_WIDGET(p)->allocation.width-p->padx;
  int i,j;

  if(p->ydata)
    for(i=0;i<p->total_ch;i++)
      for(j=0;j<p->size;j++)
	p->ydata[i][j]=NAN;
  draw_and_expose(widget);
}

float **plot_get (Plot *p){
  return(p->ydata);
}

void plot_setting (Plot *p, float range, int scale, int interval, int span, int rangechoice, int scalechoice, int spanchoice,int type,
                   int *blockslice, int *overslice){
  GtkWidget *widget=GTK_WIDGET(p);
  p->range=range;
  p->scale=scale;
  p->span=span;
  p->interval=interval;
  p->rchoice=rangechoice;
  p->schoice=scalechoice;
  p->spanchoice=spanchoice;
  p->type=type;

  if(blockslice){
    if(!p->blockslice)
      p->blockslice=calloc(p->groups,sizeof(*p->blockslice));
    memcpy(p->blockslice, blockslice, p->groups*sizeof(*p->blockslice));
  }
  if(overslice){
    if(!p->overslice)
      p->overslice=calloc(p->groups,sizeof(*p->overslice));
    memcpy(p->overslice, overslice, p->groups*sizeof(*p->blockslice));
  }

  compute_metadata(widget);
  plot_refresh(p);
  draw_and_expose(widget);
}

void plot_draw (Plot *p){
  GtkWidget *widget=GTK_WIDGET(p);
  draw_and_expose(widget);
}

void plot_set_active(Plot *p, int *a){
  GtkWidget *widget=GTK_WIDGET(p);
  memcpy(p->ch_active,a,p->total_ch*sizeof(*a));
  plot_refresh(p);
  draw_and_expose(widget);
}

