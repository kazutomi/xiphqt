/*
 *
 *  gt2 spectrum analyzer
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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "plot.h"

static GtkDrawingAreaClass *parent_class = NULL;

static void compute_imp_scale(GtkWidget *widget){
  Plot *p=PLOT(widget);
  int height=widget->allocation.height-p->pady;
  int i;
  double lfreqs[9]={10000000.,1000000.,100000.,10000.,1000.,100.,10.,1.,.1};
  double tfreqs[64]={9000000.,8000000.,7000000.,6000000.,
		     5000000.,4000000.,3000000.,2000000.,
		     900000.,800000.,700000.,600000.,
		     500000.,400000.,300000.,200000.,
		     90000.,80000.,70000.,60000.,
		     50000.,40000.,30000.,20000.,
		     9000.,8000.,7000.,6000.,
		     5000.,4000.,3000.,2000.,
		     900.,800.,700.,600.,
		     500.,400.,300.,200.,
		     90.,80.,70.,60.,
		     50.,40.,30.,20,
		     9.,8.,7.,6.,
		     5.,4.,3.,2.,
		     .9,.8,.7,.6,
		     .5,.4,.3,.2};

  for(i=0;i<9;i++)
    p->ygrid[i]=rint( (log10(p->ymax)-log10(lfreqs[i]))/(log10(p->ymax)-log10(.1)) * (height-1));
  for(i=0;i<64;i++)
    p->ytic[i]=rint( (log10(p->ymax)-log10(tfreqs[i]))/(log10(p->ymax)-log10(.1)) * (height-1));
  p->ygrids=9;
  p->ytics=64;

}

static void compute_metadata(GtkWidget *widget){
  Plot *p=PLOT(widget);
  int width=widget->allocation.width-p->padx;
  int i;

  /* find the places to plot the x grid lines according to scale */
  switch(p->scale){
  case 0: /* linear; 2kHz spacing */
    {
      float lfreq;
      for(i=0;i<11;i++){
	lfreq=i*2000.;
	p->xgrid[i]=rint(lfreq/20000. * (width-1))+p->padx;
      }
	
      p->xgrids=11;
      p->xtics=0;
      while((lfreq=(p->xtics+1)*500.)<20000.)
	p->xtic[p->xtics++]=rint(lfreq/20000. * (width-1))+p->padx;
    }
    break;
    
  case 1: /* log */
    {
      double lfreqs[6]={1.,10.,100.,1000.,10000.,100000};
      double tfreqs[37]={5.,6.,7.,8.,9.,20.,30.,40.,50.,60.,70.,80.,90.
			 ,200.,300.,400.,500.,600.,700.,800.,900.,
			 2000.,3000.,4000.,5000.,6000.,7000.,8000.,9000.,
			 20000.,30000,40000,50000,60000,70000,80000,90000};
      for(i=0;i<6;i++)
	p->xgrid[i]=rint( (log10(lfreqs[i])-log10(5.))/(log10(100000.)-log10(5.)) * (width-1))+p->padx;
      for(i=0;i<37;i++)
	p->xtic[i]=rint( (log10(tfreqs[i])-log10(5.))/(log10(100000.)-log10(5.)) * (width-1))+p->padx;
      p->xgrids=6;
      p->xtics=37;
    }
    
    break;
  case 2: /* ISO log */
    {
      double lfreqs[10]={31.,63.,125.,250.,500.,1000.,2000.,4000.,8000.,16000.};
      double tfreqs[20]={25.,40.,50.,80.,100.,160.,200.,315.,400.,630.,800.,
			1250.,1600.,2500.,3150.,5000.,6300.,10000.,12500.,20000.};
      for(i=0;i<10;i++)
	p->xgrid[i]=rint( (log2(lfreqs[i])-log2(25.))/(log2(20000.)-log2(25.)) * (width-1))+p->padx;
      for(i=0;i<20;i++)
	p->xtic[i]=rint( (log2(tfreqs[i])-log2(25.))/(log2(20000.)-log2(25.)) * (width-1))+p->padx;
      p->xgrids=10;
      p->xtics=20;
    }

    break;
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
  int impedence = (p->link == LINK_IMPEDENCE_p1 ||
		   p->link == LINK_IMPEDENCE_1 ||
		   p->link == LINK_IMPEDENCE_10);
  int phase = (p->link == LINK_PHASE);
  int padx = p->padx;

  if(phase){
    /* are any of the phase channels actually active? */
    int gi;
    int ch=0;

    phase = 0;
    for(gi=0;gi<p->groups && !phase;gi++){
      if(p->ch_active[ch+1]){
	phase=1;
	break;
      }

      ch+=p->ch[gi];
    }
  }

  if(!p->drawgc){
    p->drawgc=gdk_gc_new(p->backing);
    gdk_gc_copy(p->drawgc,widget->style->black_gc);
  }
  if(!p->dashes){
    p->dashes=gdk_gc_new(p->backing);
    gdk_gc_copy(p->dashes, p->drawgc);
    gdk_gc_set_line_attributes(p->dashes, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
    gdk_gc_set_dashes(p->dashes,0,"\002\002",2);
  }

  /* clear the old rectangle out */
  {
    GdkGC *gc=parent->style->bg_gc[0];
    gdk_draw_rectangle(p->backing,gc,1,0,0,padx,height);
    gdk_draw_rectangle(p->backing,gc,1,0,height-p->pady,width,p->pady);

    gc=parent->style->white_gc;
    gdk_draw_rectangle(p->backing,gc,1,padx,0,width-padx,height-p->pady+1);
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

    /* draw the x labels */
  {
    PangoLayout **proper;
    switch(p->scale){
    case 0:
      proper=p->lin_layout;
      break;
    case 1:
      proper=p->log_layout;
      break;
    case 2:
      proper=p->iso_layout;
      break;
    }
    for(i=0;i<p->xgrids;i++){
      int px,py;
      pango_layout_get_pixel_size(proper[i],&px,&py);
      
      gdk_draw_layout (p->backing,
		       widget->style->black_gc,
		       p->xgrid[i]-(px/2), height-py+2,
		       proper[i]);
    }
  }

  /* draw the light y grid */
  if(impedence){ /* impedence mode */

    GdkColor rgb={0,0,0,0};
    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    compute_imp_scale(widget);

    for(i=0;i<p->ytics;i++)
      gdk_draw_line(p->backing,p->drawgc,padx,p->ytic[i],width,p->ytic[i]);

  }else{
    float del=(height-p->pady-1)/(float)p->depth,off;
    int i,half=0;
    int max,mul;
    GdkColor rgb={0,0,0,0};

    {
      if(del>16){
	half=1;
	max=303;
	mul=1;
	off=(p->ymax-ceil(p->ymax))*2;
	del*=.5;
      }else if(del>8){
	max=151;
	mul=1;
	off=p->ymax-ceil(p->ymax);
      }else if(del*2>8){
	max=76;
	mul=2;
	off=p->ymax-ceil(p->ymax*.5)*2;
      }else{
	max=31;
	mul=5;
	off=p->ymax-ceil(p->ymax*.2)*5;
      }

      rgb.red=0xc000;
      rgb.green=0xff00;
      rgb.blue=0xff00;
      gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);
      gdk_gc_set_rgb_fg_color(p->dashes,&rgb);

      for(i=0;i<max;i+=2){
	int ymid=rint(del * (i * mul + off));
	if(ymid>=0 && ymid<height-p->pady)
	  gdk_draw_line(p->backing,p->drawgc,padx,ymid,width,ymid);
      }

      for(i=1;i<max;i+=2){
	int ymid=rint(del * (i * mul + off));
	if(ymid>=0 && ymid<height-p->pady)
	  gdk_draw_line(p->backing,(half?p->dashes:p->drawgc),padx,ymid,width,ymid);
      }
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

  /* dark y grid */
  if(impedence){
    GdkColor rgb={0,0,0,0};
    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;

    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<p->ygrids;i++){
      int px,py;
      pango_layout_get_pixel_size(p->imp_layout[i],&px,&py);

      gdk_draw_layout (p->backing,
		       widget->style->black_gc,
		       padx-px-2, p->ygrid[i]-py/2,
		       p->imp_layout[i]);

      gdk_draw_line(p->backing,p->drawgc,padx,p->ygrid[i],width,p->ygrid[i]);
    }
    
  }else{
    GdkColor rgb={0,0,0,0};
    int label=ceil(p->ymax/5+28),i;
    float del=(height-p->pady-1)/(float)p->depth,step;
    float off=p->ymax-ceil(p->ymax*.2)*5;
    step=2;
    if(del>8)step=1;

    for(i=0;i<32;i++){
      if(((label-i)&1)==0 || step==1){
	int ymid=rint(del * (i*5+off));
	int px,py;

	if(((label-i)&1)==0){
	  rgb.red=0x0000;
	  rgb.green=0xc000;
	  rgb.blue=0xc000;
	}else{
	  rgb.red=0xa000;
	  rgb.green=0xe000;
	  rgb.blue=0xe000;
	}
	gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

	if(label-i>=0 && label-i<57 && ymid>=0 && ymid<height-p->pady){
	  pango_layout_get_pixel_size(p->db_layout[label-i],&px,&py);
	  
	  gdk_draw_layout (p->backing,
			   widget->style->black_gc,
			   padx-px-2, ymid-py/2,
			   p->db_layout[label-i]);
	  gdk_draw_line(p->backing,p->drawgc,padx,ymid,width,ymid);
	}
      }
    }
  }

  /* phase?  draw in phase and tics on right axis */
  if(phase){
    GdkColor rgb={0,0,0,0};
    float depth = p->pmax-p->pmin;
    int label=ceil(p->pmax/10+18),i;
    float del=(height-p->pady-1)/depth,step;
    float off=p->pmax-ceil(p->pmax*.1)*10;
    step=2;
    if(del>8)step=1;
    
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);
    gdk_gc_set_rgb_fg_color(p->dashes,&rgb);
    for(i=0;i<37;i++){
      if(((label-i)&1)==0 || step==1){
	int ymid=rint(del * (i*10+off));
	int px,py;

	if(label-i>=0 && label-i<37 && ymid>=0 && ymid<height-p->pady){
	  pango_layout_get_pixel_size(p->phase_layout[label-i],&px,&py);
	  
	  gdk_draw_layout (p->backing,
			   widget->style->black_gc,
			   width-p->phax, ymid-py/2,
			   p->phase_layout[label-i]);
	}
      }
    }

    if(del>10){
      for(i=0;;i++){
	int ymid=rint(del * (i+off));
	if(ymid>=height-p->pady)break;
	if(ymid>=0)
	  gdk_draw_line(p->backing,p->drawgc,width-p->phax-(i%5==0?15:10),ymid,width-p->phax-(i%5==0?5:7),ymid);
      }
    }else if(del>5){
      for(i=0;;i++){
	int ymid=rint(del * (i*2+off));
	if(ymid>=height-p->pady)break;
	if(ymid>=0)
	  gdk_draw_line(p->backing,p->drawgc,width-p->phax-12,ymid,width-p->phax-7,ymid);
      }
    } else if(del>2){
      for(i=0;;i++){
	int ymid=rint(del * (i*5+off));
	if(ymid>=height-p->pady)break;
	if(ymid>=0)
	  gdk_draw_line(p->backing,p->drawgc,width-p->phax-15,ymid,width-p->phax-5,ymid);
      }
    }
    
    for(i=0;;i++){
      int ymid=rint(del * (i*10+off));
      if(ymid>=height-p->pady)break;
      if(ymid>=0){
	gdk_draw_line(p->backing,p->drawgc,width-p->phax-5,ymid-1,width-p->phax-2,ymid-1);
	gdk_draw_line(p->backing,p->drawgc,width-p->phax-25,ymid,width-p->phax-2,ymid);
	gdk_draw_line(p->backing,p->drawgc,width-p->phax-5,ymid+1,width-p->phax-2,ymid+1);
      }
    }

  }
  
  /* draw actual data */
  {
    int cho=0;
    int gi;
    for(gi=0;gi<p->groups;gi++){
      int ch;
      GdkColor rgb;
      
      for(ch=cho;ch<cho+p->ch[gi];ch++){
	if(p->ch_active[ch]){
	  int prev;	
	  float yprev=NAN;

	  rgb = chcolor(ch);
	  gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);
	  
	  for(i=0;i<width-padx;i++){
	    float val=p->ydata[ch][i];
	    int y;
	    
	    if(impedence){ /* log scale for impedence */
	      y =rint( (log10(p->ymax)-log10(val))/(log10(p->ymax)-log10(.1)) * 
		       (height-p->pady-1));
	    }else if(phase && ch==cho+1){
	      y= rint((height-p->pady-1)/(p->pmax-p->pmin)*(p->pmax-val));
	    }else{
	      y= rint((height-p->pady-1)/p->depth*(p->ymax-val));
	    }
	    
	    if(isnan(yprev) || isnan(val)){
	      yprev = val;
	    }else{
	      yprev = val;

	      if(y<height-p->pady || prev<height-p->pady){
		int ly = y;
		int lp = prev;
		
		if(ly>=height-p->pady)ly=height-p->pady;
		if(lp>=height-p->pady)lp=height-p->pady;
		
		gdk_draw_line(p->backing,p->drawgc,padx+i-1,lp,padx+i,ly);
		
		ly++;
		lp++;
		
		if(ly>=height-p->pady)ly=height-p->pady;
		if(lp>=height-p->pady)lp=height-p->pady;
		
		gdk_draw_line(p->backing,p->drawgc,padx+i-1,lp,padx+i,ly);
	      }
	    }
	    prev=y;
	   
	  }
	}
      }
      cho+=p->ch[gi];
    }
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
  /* no smaller than 800x600 */
  requisition->width = 800;
  requisition->height = 600;
  int axisy=0,axisx=0,pady=0,padx=0,phax=0,px,py,i;

  /* find max lin layout */
  {
    int max=0;
    for(i=0;p->lin_layout[i];i++){
      pango_layout_get_pixel_size(p->lin_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
    }
    max*=i;
    if(axisx<max)axisx=max;
  }
  /* find max log layout */
  {
    int max=0;
    for(i=0;p->log_layout[i];i++){
      pango_layout_get_pixel_size(p->log_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
    }
    max*=i;
    if(axisx<max)axisx=max;
  }
  /* find max iso layout */
  {
    int max=0;
    for(i=0;p->iso_layout[i];i++){
      pango_layout_get_pixel_size(p->iso_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
    }
    max*=i;
    if(axisx<max)axisx=max;
  }
  /* find max db layout */
  {
    int max=0;
    for(i=0;p->db_layout[i];i++){
      pango_layout_get_pixel_size(p->db_layout[i],&px,&py);
      if(py>max)max=py;
      if(px>padx)padx=px;
    }
    axisy=(max+5)*8;
    if(axisy<max)axisx=max;
  }
  /* find max imped layout */
  {
    int max=0;
    for(i=0;p->imp_layout[i];i++){
      pango_layout_get_pixel_size(p->imp_layout[i],&px,&py);
      if(py>max)max=py;
      if(px>padx)padx=px;
    }
    axisy=(max+5)*8;
    if(axisy<max)axisx=max;
  }
  /* find max phase layout */
  {
    int max=0;
    for(i=0;p->phase_layout[i];i++){
      pango_layout_get_pixel_size(p->phase_layout[i],&px,&py);
      if(py>max)max=py;
      if(px>phax)phax=px;
    }
    axisy=(max+5)*8;
    if(axisy<max)axisx=max;
  }
  
  if(requisition->width<axisx+padx)requisition->width=axisx+padx;
  if(requisition->height<axisy+pady)requisition->height=axisy+pady;
  p->padx=padx;
  p->pady=pady;
  p->phax=phax;
}

static gboolean configure(GtkWidget *widget, GdkEventConfigure *event){
  Plot *p=PLOT(widget);
  int i;

  if (p->backing)
    g_object_unref(p->backing);
  
  p->backing = gdk_pixmap_new(widget->window,
			      widget->allocation.width,
			      widget->allocation.height,
			      -1);

  if(p->ydata){
    for(i=0;i<p->total_ch;i++)free(p->ydata[i]);
    free(p->ydata);
  }

  p->ydata=malloc(p->total_ch*sizeof(*p->ydata));
  for(i=0;i<p->total_ch;i++)
    p->ydata[i]=
      calloc(widget->allocation.width,sizeof(**p->ydata));
  
  compute_metadata(widget);
  plot_refresh(p,NULL);
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
  p->mode=0;
  p->scale=0;
  p->depth=45.;
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
  int g,i;
  int ch=0;

  p->groups = groups;
  for(g=0;g<groups;g++)
    ch+=channels[g];
  p->total_ch = ch;

  p->ch=channels;
  p->rate=rate;

  /* generate all the text layouts we'll need */
  {
    char *labels[11]={"DC","2kHz","4kHz","6kHz","8kHz","10kHz","12kHz",
		      "14kHz","16kHz","18kHz",""};
    p->lin_layout=calloc(12,sizeof(*p->lin_layout));
    for(i=0;i<11;i++)
      p->lin_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[37]={"-180\xC2\xB0","-170\xC2\xB0","-160\xC2\xB0",
		      "-150\xC2\xB0","-140\xC2\xB0","-130\xC2\xB0",
		      "-120\xC2\xB0","-110\xC2\xB0","-100\xC2\xB0",
		      "-90\xC2\xB0","-80\xC2\xB0","-70\xC2\xB0",
		      "-60\xC2\xB0","-50\xC2\xB0","-40\xC2\xB0",
		      "-30\xC2\xB0","-20\xC2\xB0","-10\xC2\xB0",
		      "-0\xC2\xB0","+10\xC2\xB0","+20\xC2\xB0",
		      "+30\xC2\xB0","+40\xC2\xB0","+50\xC2\xB0",
		      "+60\xC2\xB0","+70\xC2\xB0","+80\xC2\xB0",
		      "+90\xC2\xB0","+100\xC2\xB0","+110\xC2\xB0",
		      "+120\xC2\xB0","+130\xC2\xB0","+140\xC2\xB0",
		      "+150\xC2\xB0","+160\xC2\xB0","+170\xC2\xB0",
		      "+180\xC2\xB0"};
    p->phase_layout=calloc(38,sizeof(*p->phase_layout));
    for(i=0;i<37;i++)
      p->phase_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[6]={"1Hz","10Hz","100Hz","1kHz","10kHz",""};
    p->log_layout=calloc(7,sizeof(*p->log_layout));
    for(i=0;i<6;i++)
      p->log_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[9]={"10M\xCE\xA9","1M\xCE\xA9","100k\xCE\xA9","10k\xCE\xA9",
		     "1k\xCE\xA9","100\xCE\xA9","10\xCE\xA9","1\xCE\xA9",".1\xCE\xA9"};
    p->imp_layout=calloc(10,sizeof(*p->imp_layout));
    for(i=0;i<9;i++)
      p->imp_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[10]={"31Hz","63Hz","125Hz","250Hz","500Hz","1kHz","2kHz",
		      "4kHz","8kHz","16kHz"};
    p->iso_layout=calloc(11,sizeof(*p->iso_layout));
    for(i=0;i<10;i++)
      p->iso_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[57]={"-140dB","-135dB","-130dB","-125dB","-120dB","-115dB",
		      "-110dB","-105dB","-100dB","-95dB","-90dB","-85dB",
		      "-80dB","-75dB","-70dB","-65dB","-60dB","-55dB","-50dB",
		      "-45dB","-40dB","-35dB","-30dB","-25dB","-20dB",
		      "-15dB","-10dB","-5dB","0dB","+5dB","+10dB","+15dB",
		      "+20dB","+25dB","+30dB","+35dB","+40dB","+45dB","+50dB",
		      "+55dB","+60dB","+65dB","+70dB","+75dB","+80dB","+85dB",
		      "+90dB","+95dB","+100dB","+105dB","+110dB","+115dB",
		      "+120dB","+125dB","+130dB","+135dB","+140dB"};
    p->db_layout=calloc(58,sizeof(*p->db_layout));
    for(i=0;i<57;i++)
      p->db_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }

  p->ch_active=calloc(ch,sizeof(*p->ch_active));
  p->ch_process=calloc(ch,sizeof(*p->ch_process));

  /*
    p->cal=calloc(p->ch,sizeof(*p->cal));
    for(i=0;i<p->ch;i++)
      p->cal[i]=calloc(p->datasize,sizeof(**p->cal));
  */
  plot_clear(p);
  return ret;
}

void plot_refresh (Plot *p, int *process){
  int i;
  float ymax,pmax,pmin;
  int width=GTK_WIDGET(p)->allocation.width-p->padx;
  int height=GTK_WIDGET(p)->allocation.height-p->pady;
  float **data;
  
  if(process)
    memcpy(p->ch_process,process,p->total_ch*sizeof(*process));
  
  data = process_fetch(p->scale,p->mode,p->link,
		       p->ch_process,width,&ymax,&pmax,&pmin);
  
  if(p->ydata)
    for(i=0;i<p->total_ch;i++)
      memcpy(p->ydata[i],data[i],width*sizeof(**p->ydata));

  /* graph limit updates are conditional depending on mode/link */
  if(pmax<12)pmax=12;
  if(pmin>-12)pmin=-12;

  switch(p->link){
  case LINK_INDEPENDENT:
  case LINK_SUMMED:
  case LINK_PHASE:
  case LINK_THD:
  case LINK_THDN:
    {
      float dBpp = p->depth/height;
      ymax += dBpp*10;
    }
    break;
  case LINK_IMPEDENCE_p1:
  case LINK_IMPEDENCE_1:
  case LINK_IMPEDENCE_10:
    ymax *=1.8;
    break;
  }

  //if(p->ymax>ymax)ymax=p->ymax;
  //if(pmax<p->pmax)pmax=p->pmax;
  //if(pmin>p->pmin)pmin=p->pmin;

  /* time damp decay */



  /* finally, align phase/response zeros on phase graphs */
  if(ymax>-140){
    if(p->link == LINK_PHASE){
      /* align the phase and response zeros by shifting phase */
      float mzero = (height-1)/p->depth*ymax;
      float pzero = (height-1)/(pmax-pmin)*pmax;
      
      if(mzero<pzero){
	pmin = pmax-(height-1)/mzero*pmax;
	pzero = (height-1)/(pmax-pmin)*pmax;
      }else{
	pmax = pmin/(1-(height-1)/mzero);
	pzero = (height-1)/(pmax-pmin)*pmax;
      }
      
      /* If phase shifts beyond +/- 180, shift main scale. */
      
      if(pmin<-180.){
	/* increase ymax, shift main scale zero down */
	pmin = -180.;
	pzero = (height-1)/(pmax-pmin)*pmax;
	ymax = pzero*p->depth/(height-1);
      }else if(pmax>180.){
	/* only way to reconcile this one is to increase the pdepth */
	pmax = 180.;
	pzero = (height-1)/(pmax-pmin)*pmax;
	p->depth = (height-1)/pzero*ymax;
      }
    }
  }

  if(ymax<p->depth-140.)ymax=p->depth-140.;
  if(ymax>140.)ymax=140.;
  if(pmax>180)pmax=180;
  if(pmin<-180)pmin=-180;
  
  p->ymax=ymax;
  p->pmax=pmax;
  p->pmin=pmin;
}

void plot_clear (Plot *p){
  GtkWidget *widget=GTK_WIDGET(p);
  int width=GTK_WIDGET(p)->allocation.width-p->padx;
  int i,j;

  if(p->ydata)
    for(i=0;i<p->total_ch;i++)
      for(j=0;j<width;j++)
	p->ydata[i][j]=NAN;
  p->ymax=-140;
  p->pmax=12.;
  p->pmin=-12.;
  draw_and_expose(widget);
}

float **plot_get (Plot *p){
  return(p->ydata);
}

void plot_setting (Plot *p, int scale, int mode, int link, int depth){
  GtkWidget *widget=GTK_WIDGET(p);
  p->scale=scale;
  p->mode=mode;
  p->depth=depth;
  p->link=link;

  p->ymax=-140;
  p->pmax=12.;
  p->pmin=-12.;

  compute_metadata(widget);
  plot_refresh(p,NULL);
  draw_and_expose(widget);
}

void plot_draw (Plot *p){
  GtkWidget *widget=GTK_WIDGET(p);
  draw_and_expose(widget);
}

void plot_set_active(Plot *p, int *a, int *b){
  GtkWidget *widget=GTK_WIDGET(p);
  memcpy(p->ch_active,a,p->total_ch*sizeof(*a));
  memcpy(p->ch_process,b,p->total_ch*sizeof(*b));
  plot_refresh(p,NULL);
  draw_and_expose(widget);
}

