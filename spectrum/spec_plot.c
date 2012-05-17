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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "spec_plot.h"

static double log_lfreqs[5]={10.,100.,1000.,10000.,100000};
static double log_llfreqs[15]={10.,20.,30.,50.,100.,200.,300.,500.,
                              1000.,2000.,3000.,5000.,10000.,
                              20000.,30000.};
static double log_tfreqs[37]={5.,6.,7.,8.,9.,20.,30.,40.,50.,60.,70.,80.,90.
			 ,200.,300.,400.,500.,600.,700.,800.,900.,
			 2000.,3000.,4000.,5000.,6000.,7000.,8000.,9000.,
			 20000.,30000,40000,50000,60000,70000,80000,90000};

static double iso_lfreqs[12]={31.,63.,125.,250.,500.,1000.,2000.,4000.,8000.,
                              16000.,32000., 64000.};
static double iso_tfreqs[24]={25.,40.,50.,80.,100.,160.,200.,315.,400.,630.,
                              800.,1250.,1600.,2500.,3150.,5000.,6300.,10000.,
                              12500.,20000.,25000.,40000.,50000.,80000.};

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
    p->ygrid[i]=rint( (log10(p->disp_ymax)-log10(lfreqs[i]))/(log10(p->disp_ymax)-log10(.1)) * (height-1));
  for(i=0;i<64;i++)
    p->ytic[i]=rint( (log10(p->disp_ymax)-log10(tfreqs[i]))/(log10(p->disp_ymax)-log10(.1)) * (height-1));
  p->ygrids=9;
  p->ytics=64;

}

int phase_active_p(Plot *p){
  if(p->link == LINK_PHASE){
    int cho=0;
    int gi;
    for(gi=0;gi<p->groups;gi++)
      if(p->ch[gi]>1 && p->ch_active[cho+1])
        return 1;
  }
  return 0;
}

static void compute_metadata(GtkWidget *widget){
  Plot *p=PLOT(widget);
  int phase = phase_active_p(p);
  int width = widget->allocation.width-p->padx-(phase?p->phax:0);
  int rate=p->maxrate;
  int nyq=p->maxrate/2.;
  int i;

  p->xgrids=0;
  p->xtics=0;

  /* find the places to plot the x grid lines according to scale */
  switch(p->scale){
  case 0: /* log */
    {
      for(i=0;i<5;i++){
        if(log_lfreqs[i]<(nyq-.1))
          p->xgrids=i+1;
      }
      for(i=0;i<15;i++){
        if(log_llfreqs[i]<(nyq-.1))
          p->xlgrids=i+1;
      }
      for(i=0;i<37;i++){
        if(log_tfreqs[i]<(nyq-.1))
          p->xtics=i+1;
      }

      for(i=0;i<p->xgrids;i++)
	p->xgrid[i]=rint( (log10(log_lfreqs[i])-log10(5.))/(log10(nyq)-log10(5.)) * (width-1))+p->padx;
      for(i=0;i<p->xlgrids;i++)
	p->xlgrid[i]=rint( (log10(log_llfreqs[i])-log10(5.))/(log10(nyq)-log10(5.)) * (width-1))+p->padx;
      for(i=0;i<p->xtics;i++)
	p->xtic[i]=rint( (log10(log_tfreqs[i])-log10(5.))/(log10(nyq)-log10(5.)) * (width-1))+p->padx;
    }

    break;
  case 1: /* ISO log */
    {
      for(i=0;i<12;i++){
        if(iso_lfreqs[i]<(nyq-.1)){
          p->xgrids=i+1;
          p->xlgrids=i+1;
        }
      }
      for(i=0;i<24;i++){
        if(iso_tfreqs[i]<(nyq-.1))
          p->xtics=i+1;
      }

      for(i=0;i<p->xgrids;i++)
	p->xgrid[i]=p->xlgrid[i]=rint( (log2(iso_lfreqs[i])-log2(25.))/(log2(nyq)-log2(25.)) * (width-1))+p->padx;
      for(i=0;i<p->xtics;i++)
	p->xtic[i]=rint( (log2(iso_tfreqs[i])-log2(25.))/(log2(nyq)-log2(25.)) * (width-1))+p->padx;
    }

    break;
  case 2: /* linear spacing */
    {
      int j;
      for(i=0;;i++){
        if(i*p->lin_major >= nyq-.1 || i*p->lin_major>=100000-.1)
          break;
        p->xgrids=i+1;
        p->xlgrids=i+1;
      }
      for(i=0;;i++){
        if(i*p->lin_minor >= nyq-.1 || i*p->lin_minor>=100000-.1)
          break;
        if(i%p->lin_mult!=0)
          p->xtics++;
      }

      for(i=0;i<p->xgrids;i++){
        double lfreq=i*p->lin_major;
        p->xgrid[i]=p->xlgrid[i]=rint(lfreq/nyq * (width-1))+p->padx;
      }
      j=0;
      for(i=0;i<p->xtics;i++,j++){
        double lfreq;
        if(j%p->lin_mult==0)j++;
        lfreq=j*p->lin_minor;
        p->xtic[i]=rint(lfreq/nyq * (width-1))+p->padx;
      }
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
#if 0
  int impedence = (p->link == LINK_IMPEDENCE_p1 ||
		   p->link == LINK_IMPEDENCE_1 ||
		   p->link == LINK_IMPEDENCE_10);
#endif
  int phase = phase_active_p(p);
  int padx = p->padx;
  int phax = phase ? p->phax : 0;
  int pwidth = width - padx - phax;

  if(!p->drawgc){
    p->drawgc=gdk_gc_new(p->backing);
    gdk_gc_copy(p->drawgc,widget->style->black_gc);
  }

  if(!p->dashes){
    p->dashes=gdk_gc_new(p->backing);
    gdk_gc_copy(p->dashes, p->drawgc);
    gdk_gc_set_line_attributes(p->dashes, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_BUTT, GDK_JOIN_MITER);
    gdk_gc_set_dashes(p->dashes,0,(signed char *)"\002\002",2);
  }
  if(!p->graygc){
    GdkColor rgb_bg;
    GdkColor rgb_fg;
    GdkGCValues v;
    p->graygc=gdk_gc_new(p->backing);
    gdk_gc_copy(p->graygc, p->drawgc);
    gdk_gc_get_values(p->graygc,&v);
    gdk_colormap_query_color(gdk_gc_get_colormap(p->graygc),v.foreground.pixel,&rgb_fg);
    gdk_colormap_query_color(gdk_gc_get_colormap(p->graygc),v.background.pixel,&rgb_bg);
    rgb_fg.red = (rgb_fg.red*3 + rgb_bg.red*2)/5;
    rgb_fg.green = (rgb_fg.green*3 + rgb_bg.green*2)/5;
    rgb_fg.blue = (rgb_fg.blue*3 + rgb_bg.blue*2)/5;
    gdk_gc_set_rgb_fg_color(p->graygc,&rgb_fg);
  }
  if(!p->phasegc){
    GdkColor rgb={0,0x8000,0x0000,0x0000};
    p->phasegc=gdk_gc_new(p->backing);
    gdk_gc_set_rgb_fg_color(p->phasegc,&rgb);
    gdk_gc_set_line_attributes(p->phasegc,1,GDK_LINE_SOLID,GDK_CAP_PROJECTING,GDK_JOIN_MITER);
  }

  {
    const GdkRectangle clip = {p->padx,0,pwidth,height-p->pady};
    GdkGCValues values;
    //gdk_gc_get_values(p->drawgc,&values);
    values.line_width=1;
    gdk_gc_set_values(p->drawgc,&values,GDK_GC_LINE_WIDTH);
    gdk_gc_set_clip_rectangle (p->drawgc, &clip);
  }

  /* clear the old rectangle out */
  {
    GdkGC *gc=parent->style->bg_gc[0];
    gdk_draw_rectangle(p->backing,gc,1,0,0,padx,height);
    gdk_draw_rectangle(p->backing,gc,1,0,height-p->pady,width,p->pady);
    if(phase)
      gdk_draw_rectangle(p->backing,gc,1,width-phax,0,phax,height);

    gc=parent->style->white_gc;
    gdk_draw_rectangle(p->backing,gc,1,padx,0,pwidth,height-p->pady);
  }

  /* draw the noise floor if active */
  #if 0
  if(p->floor){
    GdkColor rgb = {0,0xd000,0xd000,0xd000};
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<pwidth;i++){
      float val=p->floor[i];
      int y;

      /* No noise floor is passed back for display in the modes where it's irrelevant */
      y= rint((height-p->pady-1)/p->disp_depth*(p->disp_ymax-val));
      if(y<height-p->pady)
	gdk_draw_line(p->backing,p->drawgc,padx+i,y,padx+i,height-p->pady-1);
    }
  }
  #endif

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
      proper=p->log_layout;
      break;
    case 1:
      proper=p->iso_layout;
      break;
    case 2:
      proper=p->lin_layout;
      break;
    }
    for(i=0;i<p->xlgrids;i++){
      int px,py;
      pango_layout_get_pixel_size(proper[i],&px,&py);

      if(p->xlgrid[i]+(px/2)<width)
        gdk_draw_layout (p->backing,
                         widget->style->black_gc,
                         p->xlgrid[i]-(px/2), height-py+2,
                         proper[i]);
    }
  }

  /* draw the y grid */
  //if(impedence){ /* impedence mode */
  if(0){

    /* light grid */

    GdkColor rgb={0,0,0,0};
    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    compute_imp_scale(widget);

    for(i=0;i<p->ytics;i++)
      gdk_draw_line(p->backing,p->drawgc,padx,p->ytic[i],pwidth,p->ytic[i]);

    /* dark grid */
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

      gdk_draw_line(p->backing,p->drawgc,padx,p->ygrid[i],pwidth,p->ygrid[i]);
    }

  }else{
    GdkColor rgb={0,0,0,0};
    float emheight = (height-p->pady)/p->pady;
    float emperdB = emheight/p->disp_depth;
    float pxperdB = (height-p->pady)/p->disp_depth;

    /* we want no more than <n> major lines per graph */
    int maxmajorper = 15;
    /* we want major grid lines and labels no less than <n>em apart */
    float majorsep = 2.5;
    /* we want no more than <n> minor/subminor lines per major */
    int maxminorper = 10;
    /* don't put minor/subminor lines closer together than <n>px */
    float minorsep = 8.;

    int majordel=50000;
    int minordel=50000;
    int subminordel=50000;

    int majordellist[]=
      { 100,  1000, 5000, 10000, 20000, -1};
    float majordeltest[]=
      {  10,     1,   .2,    .1,   .05, -1};

    for(i=0;majordellist[i]>0;i++){
      /* minimum em seperation? */
      if(emperdB>majorsep*majordeltest[i] &&
         /* Not over the number of lines limit? */
         p->disp_depth*majordeltest[i]<maxmajorper){
        majordel=majordellist[i];
        break;
      }
    }

    /* choose appropriate minor and subminor spacing */
    int minordellist[]=
      { 10,  25,  50,  100, 250, 500, 1000, 2500, 5000, 10000, 25000, -1, -1};
    float minordeltest[]=
      {100,  40,  20,   10,   4,   2,    1,   .4,   .2,   .1,    .04, -1, -1};

    for(i=0;minordellist[i]>0;i++){
      /* minimum px seperation? */
      if(pxperdB>minorsep*minordeltest[i] &&
         /* is the major an integer multiple? */
         majordel % minordellist[i] == 0 &&
         /* not too many minor lines per major? */
         majordel / minordellist[i] <= maxminorper){
        subminordel=minordellist[i];
        break;
      }
    }
    minordel = subminordel;

    for(i++;minordellist[i]>0;i++){
      /* minor must not equal major */
      if(minordellist[i]==majordel)break;
      if(/* is it an integer multiple of the subminor */
         minordellist[i] % subminordel == 0 &&
         /* is the major an integer multiple? */
         majordel % minordellist[i] == 0){
        minordel=minordellist[i];
        break;
      }
    }

    /* Light Y grid */
    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);
    gdk_gc_set_rgb_fg_color(p->dashes,&rgb);

    float ymin = (p->disp_ymax - p->disp_depth)*1000;
    int yval = rint((p->disp_ymax*1000/subminordel)+1)*subminordel;

    while(1){
      float ydel = (yval - ymin)/(p->disp_depth*1000);
      int ymid = rint(height-p->pady-1 - (height-p->pady) * ydel);

      if(ymid>=height-p->pady)break;

      if(ymid>=0){
        if(yval % majordel == 0){
        }else if(yval % minordel == 0){
          gdk_draw_line(p->backing,p->drawgc,padx,ymid,width-phax,ymid);
        }else{
          gdk_draw_line(p->backing,p->dashes,padx,ymid,width-phax,ymid);
        }
      }
      yval-=subminordel;
    }

    /* Dark Y grid */
    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    ymin = (p->disp_ymax - p->disp_depth)*1000;
    yval = rint((p->disp_ymax*1000/majordel)+1)*majordel;

    {
      int px,py,pxdB,pxN,pxMAX;
      pango_layout_get_pixel_size(p->db_layoutN,&pxN,&py);
      pango_layout_get_pixel_size(p->db_layoutdB,&pxdB,&py);
      pango_layout_get_pixel_size(p->db_layout[0],&pxMAX,&py);
      pxMAX+=pxdB;

      while(1){
        float ydel = (yval - ymin)/(p->disp_depth*1000);
        int ymid = rint(height-p->pady-1 - (height-p->pady) * ydel);

        if(ymid>=height-p->pady)break;

        if(ymid>=0){
          int do_dB=0;
          int label = yval/100+2000;

          if(label>=0 && label<=4000 /* in range check */
             && (p->scale<2 || ymid+py/2 < height-p->pady) /* don't collide with DC label */
             ){

            if(label%10){ /* fractional (decimal) dB */
              int sofar=0;
              /*  -.9dB
                 -9.9dB
                  -99.9
                 -999.9 */

              if(fabsf(yval*.001)<9.98){
                gdk_draw_layout (p->backing,
                                 p->graygc,
                                 padx-pxdB-2, ymid-py/2,
                                 p->db_layoutdB);
                sofar+=pxdB;
              }

              pango_layout_get_pixel_size(p->db_layout1[abs(yval/100)%10],&px,&py);
              sofar+=px;
              gdk_draw_layout (p->backing,
                               p->graygc,
                               padx-sofar-2, ymid-py/2,
                               p->db_layout1[abs(yval/100)%10]);

              if(yval/1000!=0){ /* no leading zero please */
                pango_layout_get_pixel_size(p->db_layout[yval/1000+200],&px,&py);
                sofar+=px;
                gdk_draw_layout (p->backing,
                                 p->graygc,
                                 padx-sofar-2, ymid-py/2,
                                 p->db_layout[yval/1000+200]);
              }else{
                if(yval<0){
                  /* need to explicitly place negative */
                  sofar+=pxN;
                  gdk_draw_layout (p->backing,
                                   p->graygc,
                                   padx-sofar-2, ymid-py/2,
                                   p->db_layoutN);
                }
              }
            }else{
              gdk_draw_layout (p->backing,
                               widget->style->black_gc,
                               padx-pxdB-2, ymid-py/2,
                               p->db_layoutdB);

              pango_layout_get_pixel_size(p->db_layout[yval/1000+200],&px,&py);

              gdk_draw_layout (p->backing,
                               widget->style->black_gc,
                               padx-px-pxdB-2, ymid-py/2,
                               p->db_layout[yval/1000+200]);
            }
          }
          gdk_draw_line(p->backing,p->drawgc,padx,ymid,width-phax,ymid);
        }
        yval-=majordel;
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


  gdk_gc_set_line_attributes(p->drawgc,p->bold+1,GDK_LINE_SOLID,GDK_CAP_PROJECTING,
                             GDK_JOIN_MITER);

  /* draw actual data */
  if(p->ydata){
    int cho=0;
    int gi;
    for(gi=0;gi<p->groups;gi++){
      int ch;
      GdkColor rgb;

      for(ch=cho;ch<cho+p->ch[gi];ch++){
	if(p->ch_active[ch]){

	  rgb = chcolor(ch);
	  gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

	  for(i=0;i<pwidth;i++){
	    float valmin=p->ydata[ch][i*2];
	    float valmax=p->ydata[ch][i*2+1];
            float ymin, ymax;

            if(!isnan(valmin) && !isnan(valmax)){

              //if(impedence){ /* log scale for impedence */
              if(0){

                ymin = rint( (log10(p->disp_ymax)-log10(valmin))/
                             (log10(p->disp_ymax)-log10(.1)) *
                             (height-p->pady-1));
                ymax = rint( (log10(p->disp_ymax)-log10(valmax))/
                             (log10(p->disp_ymax)-log10(.1)) *
                             (height-p->pady-1));

              }else if(phase && ch==cho+1){

                ymin = rint((height-p->pady-1)/
                            (p->disp_pmax-p->disp_pmin)*
                            (p->disp_pmax-valmin));
                ymax = rint((height-p->pady-1)/
                            (p->disp_pmax-p->disp_pmin)*
                            (p->disp_pmax-valmax));

              }else{

                ymin = rint((height-p->pady-1)/p->disp_depth*(p->disp_ymax-valmin));
                ymax = rint((height-p->pady-1)/p->disp_depth*(p->disp_ymax-valmax));

              }

              gdk_draw_line(p->backing,p->drawgc,padx+i,ymin,padx+i,ymax);
            }
	  }
	}
      }
      cho+=p->ch[gi];
    }
  }

  /* phase?  draw in phase and tics on right axis */
  if(phase){
    float depth = p->disp_pmax-p->disp_pmin;
    int label=ceil(p->disp_pmax/10+18),i;
    float del=(height-p->pady-1)/depth,step;
    float off=p->disp_pmax-ceil(p->disp_pmax*.1)*10;
    step=2;
    if(del>8)step=1;

    for(i=0;i<38;i++){
      if(((label-i)&1)==0 || step==1){
	int ymid=rint(del * (i*10+off));
	int px,py;

	if(label-i>=0 && label-i<37 && ymid>=p->pady/2 && ymid<height-p->pady/2){
          pango_layout_get_pixel_size(p->phase_layout[label-i],&px,&py);
	  gdk_draw_layout (p->backing,p->phasegc,
			   width-p->phax+2, ymid-py/2,
			   p->phase_layout[label-i]);
	}
      }
    }

    if(del>10){
      for(i=0;;i++){
	int ymid=rint(del * (i+off));
        int pv = rint(p->pmax - ymid/(float)(height-p->pady) * (p->pmax - p->pmin));
	if(ymid>=height-p->pady)break;
	if(ymid>=0 && pv>=-180 && pv<=180)
	  gdk_draw_line(p->backing,p->phasegc,width-p->phax-(i%5==0?15:10),ymid,width-p->phax-(i%5==0?5:7),ymid);
      }
    }else if(del>5){
      for(i=0;;i++){
	int ymid=rint(del * (i*2+off));
        int pv = rint(p->pmax - ymid/(float)(height-p->pady) * (p->pmax - p->pmin));
	if(ymid>=height-p->pady)break;
	if(ymid>=0 && pv>=-180 && pv<=180)
	  gdk_draw_line(p->backing,p->phasegc,width-p->phax-12,ymid,width-p->phax-7,ymid);
      }
    } else if(del>2){
      for(i=0;;i++){
	int ymid=rint(del * (i*5+off));
        int pv = rint(p->pmax - ymid/(float)(height-p->pady) * (p->pmax - p->pmin));
	if(ymid>=height-p->pady)break;
	if(ymid>=0 && pv>=-180 && pv<=180)
	  gdk_draw_line(p->backing,p->phasegc,width-p->phax-15,ymid,width-p->phax-5,ymid);
      }
    }

    if(del>=2){
      for(i=0;;i++){
        int ymid=rint(del * (i*10+off));
        int pv = rint(p->pmax - ymid/(float)(height-p->pady) * (p->pmax - p->pmin));
        if(ymid>=height-p->pady)break;
	if(ymid>=0 && pv>=-180 && pv<=180){
          gdk_draw_line(p->backing,p->phasegc,width-p->phax-5,ymid-1,width-p->phax-1,ymid-1);
          gdk_draw_line(p->backing,p->phasegc,width-p->phax-25,ymid,width-p->phax-1,ymid);
          gdk_draw_line(p->backing,p->phasegc,width-p->phax-5,ymid+1,width-p->phax-1,ymid+1);
        }
      }
    }else{

      for(i=0;;i++){
	int ymid=rint(del * (i*10+off));
        int pv = rint(p->pmax - ymid/(float)(height-p->pady) * (p->pmax - p->pmin));
	if(ymid>=height-p->pady)break;
	if(ymid>=0 && pv>=-180 && pv<=180)
	  gdk_draw_line(p->backing,p->phasegc,width-p->phax-15,ymid,width-p->phax-5,ymid);
      }

      for(i=0;;i++){
        int ymid=rint(del * (i*10+off));
        int pv = rint((p->pmax - ymid/(float)(height-p->pady) * (p->pmax - p->pmin))/10);
        if(ymid>=height-p->pady)break;
	if(ymid>=0 && pv>=-18 && pv<=18 && (pv&1)==0){
          gdk_draw_line(p->backing,p->phasegc,width-p->phax-5,ymid-1,width-p->phax-1,ymid-1);
          gdk_draw_line(p->backing,p->phasegc,width-p->phax-25,ymid,width-p->phax-1,ymid);
          gdk_draw_line(p->backing,p->phasegc,width-p->phax-5,ymid+1,width-p->phax-1,ymid+1);
        }
      }
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
  requisition->width = 400;
  requisition->height = 200;
  int axisy=0,axisx=0,pady=0,padx=0,phax=0,px,py,i;

  /* find max lin layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->lin_layout[i];i++){
      if(p->lin_major*i >= p->maxrate/2-.1)break;
      pango_layout_get_pixel_size(p->lin_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max+=maxy*1.5;
    max*=i+1;
    if(axisx<max)axisx=max;
  }
  /* find max log layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->log_layout[i];i++){
      if(log_lfreqs[i] >= p->maxrate/2-.1)break;
      pango_layout_get_pixel_size(p->log_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max+=maxy*1.5;
    max*=(i+1)*3;
    if(axisx<max)axisx=max;
  }
  /* find max iso layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->iso_layout[i];i++){
      if(iso_lfreqs[i] >= p->maxrate/2-.1)break;
      pango_layout_get_pixel_size(p->iso_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max+=maxy*1.5;
    max*=i+1;
    if(axisx<max)axisx=max;
  }


  /* find max db layout */
  {
    int max=0;
    int px2;
    pango_layout_get_pixel_size(p->db_layoutdB,&px2,&py);
    pango_layout_get_pixel_size(p->db_layout[0],&px,&py);
    if(px+px2>padx)padx=px+px2;
    axisy=(max)*8;
    if(axisy<max)axisy=max;
  }
  /* find max imped layout */
#if 0
  {
    int max=0;
    for(i=0;p->imp_layout[i];i++){
      pango_layout_get_pixel_size(p->imp_layout[i],&px,&py);
      //if(py>max)max=py;
      if(px>padx)padx=px;
    }
    axisy=(max)*8;
    if(axisy<max)axisy=max;
  }
#endif

  /* find max phase layout */
  {
    int max=0;
    for(i=0;p->phase_layout[i];i++){
      pango_layout_get_pixel_size(p->phase_layout[i],&px,&py);
      //if(py>max)max=py;
      if(px>phax)phax=px;
    }
    axisy=(max)*8;
    if(axisy<max)axisy=max;
  }

  if(requisition->width<axisx+padx)requisition->width=axisx+padx;
  if(requisition->height<axisy+pady)requisition->height=axisy+pady;
  p->padx=padx;
  p->pady=pady;
  p->phax=phax+2;
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
  plot_refresh(p,NULL);
  draw_and_expose(widget);

  return TRUE;
}

static void plot_class_init (PlotClass *class){
  int i,w,h;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);
  GdkWindow *root=gdk_get_default_root_window();
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
  int maxrate=-1;
  p->groups = groups;
  for(g=0;g<groups;g++){
    ch+=channels[g];
    if(rate[g]>maxrate)maxrate=rate[g];
  }

  p->total_ch = ch;

  p->ch=channels;
  p->rate=rate;
  p->maxrate=maxrate;

  if(maxrate > 100000){
    p->lin_major = 10000.;
    p->lin_minor = 2000.;
    p->lin_mult = 5;
  }else if(maxrate > 50000){
    p->lin_major = 5000.;
    p->lin_minor = 1000.;
    p->lin_mult = 5;
  }else{
    p->lin_major=2000.;
    p->lin_minor=500.;
    p->lin_mult=4;
  }

  /* generate all the text layouts we'll need */
  /* linear X scale */
  {
    if(maxrate>100000){
      char *labels[11]={"DC","10kHz","20kHz","30kHz","40kHz","50kHz","60kHz",
                        "70kHz","80kHz","90kHz",""};
      p->lin_layout=calloc(12,sizeof(*p->lin_layout));
      for(i=0;i<11;i++)
        p->lin_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
    }else if(maxrate > 50000){
      char *labels[11]={"DC","5kHz","10kHz","15kHz","20kHz","25kHz","30kHz",
                        "35kHz","40kHz","45kHz",""};
      p->lin_layout=calloc(12,sizeof(*p->lin_layout));
      for(i=0;i<11;i++)
        p->lin_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
    }else{
      char *labels[14]={"DC","2kHz","4kHz","6kHz","8kHz","10kHz","12kHz",
                        "14kHz","16kHz","18kHz","20kHz","22kHz","24kHz",""};
      p->lin_layout=calloc(15,sizeof(*p->lin_layout));
      for(i=0;i<14;i++)
        p->lin_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
    }
  }
  /* phase Y scale */
  {
    char *labels[37]={"-180\xC2\xB0","-170\xC2\xB0","-160\xC2\xB0",
		      "-150\xC2\xB0","-140\xC2\xB0","-130\xC2\xB0",
		      "-120\xC2\xB0","-110\xC2\xB0","-100\xC2\xB0",
		      "-90\xC2\xB0","-80\xC2\xB0","-70\xC2\xB0",
		      "-60\xC2\xB0","-50\xC2\xB0","-40\xC2\xB0",
		      "-30\xC2\xB0","-20\xC2\xB0","-10\xC2\xB0",
		      "0\xC2\xB0","10\xC2\xB0","20\xC2\xB0",
		      "30\xC2\xB0","40\xC2\xB0","50\xC2\xB0",
		      "60\xC2\xB0","70\xC2\xB0","80\xC2\xB0",
		      "90\xC2\xB0","100\xC2\xB0","110\xC2\xB0",
		      "120\xC2\xB0","130\xC2\xB0","140\xC2\xB0",
		      "150\xC2\xB0","160\xC2\xB0","170\xC2\xB0",
		      "180\xC2\xB0"};
    p->phase_layout=calloc(38,sizeof(*p->phase_layout));
    for(i=0;i<37;i++)
      p->phase_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  /* log X scale */
  {
    char *labels[15]={"10Hz","20","30","50","100Hz",
                     "200","300","500","1kHz",
                     "2k","3k","5k","10kHz",
                     "20k","30k"};
    p->log_layout=calloc(16,sizeof(*p->log_layout));
    for(i=0;i<15;i++)
      p->log_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  /* Impedence Y scale */
  {
    char *labels[9]={"10M\xCE\xA9","1M\xCE\xA9","100k\xCE\xA9","10k\xCE\xA9",
		     "1k\xCE\xA9","100\xCE\xA9","10\xCE\xA9","1\xCE\xA9",".1\xCE\xA9"};
    p->imp_layout=calloc(10,sizeof(*p->imp_layout));
    for(i=0;i<9;i++)
      p->imp_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  /* ISO log X scale */
  {
    char *labels[12]={"31Hz","63Hz","125Hz","250Hz","500Hz","1kHz","2kHz",
		      "4kHz","8kHz","16kHz","32kHz",""};
    p->iso_layout=calloc(13,sizeof(*p->iso_layout));
    for(i=0;i<12;i++)
      p->iso_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  /* dB Y scale (integer) */
  {
    char buf[10];
    p->db_layout=calloc(402,sizeof(*p->db_layout));
    for(i=-200;i<=200;i++){
      snprintf(buf,10," %d",i);
      p->db_layout[i+200]=gtk_widget_create_pango_layout(ret,buf);
    }
  }
  /* dB Y scale (decimal) */
  {
    char buf[10];
    p->db_layout1=calloc(10,sizeof(*p->db_layout1));
    for(i=0;i<10;i++){
      snprintf(buf,10,".%d",i);
      p->db_layout1[i]=gtk_widget_create_pango_layout(ret,buf);
    }
  }
  /* dB Y scale (dB) */
  p->db_layoutdB=gtk_widget_create_pango_layout(ret,"dB");
  /* dB Y scale (-) */
  p->db_layoutN=gtk_widget_create_pango_layout(ret,"-");

  p->ch_active=calloc(ch,sizeof(*p->ch_active));
  p->ch_process=calloc(ch,sizeof(*p->ch_process));

  p->autoscale=1;

  plot_clear(p);
  return ret;
}

void plot_refresh (Plot *p, int *process){
  float ymax,pmax,pmin;
  int phase = phase_active_p(p);
  int width=GTK_WIDGET(p)->allocation.width-p->padx-(phase ? p->phax : 0);
  int height=GTK_WIDGET(p)->allocation.height-p->pady;
  float **data;

#define THRESH .25
#define PXDEL 10.
#define TIMERFRAMES 20

  if(!p->configured)return;

  if(process)
    memcpy(p->ch_process,process,p->total_ch*sizeof(*process));

  data = process_fetch(p->scale, p->mode, p->link,
		       p->ch_process,width,&ymax,&pmax,&pmin);

  p->ydata=data;

  /* graph limit updates are conditional depending on mode/link */
  pmax+=5;
  pmin-=5;
  if(pmax<5)pmax=5;
  if(pmin>-30)pmin=-30;

  switch(p->link){
  case LINK_INDEPENDENT:
  case LINK_SUMMED:
  case LINK_PHASE:
    {
      float dBpp = p->depth/height;
      ymax += dBpp*10;
    }
    break;
#if 0
  case LINK_IMPEDENCE_p1:
  case LINK_IMPEDENCE_1:
  case LINK_IMPEDENCE_10:
    if(ymax<12)
      ymax=12;
    else
      ymax*=1.8;
    break;
#endif
  }

  if(p->mode == 0){
    /* "Instantaneous' mode scale regression is conditional and
       damped. Start the timer/run the timer while any one scale
       measure should be dropping by more than 25% of the current
       depth. If any peaks occur above, reset timer.  Once timer runs
       out, drop PXEDL px per frame */
    /* todo:  might be nice to use a bessel function to track the scale
       shifts */
    if(p->ymax>ymax){
      float oldzero = height/p->depth*p->ymax;
      float newzero = height/p->depth*ymax;

      if(oldzero-newzero > height*THRESH){
        if(p->ymaxtimer){
          p->ymaxtimer--;
        }else{
            ymax = (oldzero-PXDEL)*p->depth/(height-1);
        }
      }else{
        p->ymaxtimer = TIMERFRAMES;
      }
    }else
      p->ymaxtimer = TIMERFRAMES;

    if(p->pmax>pmax || p->pmin<pmin){
      float newmax = height/(p->pmax-p->pmin)*(p->pmax-pmax);
      float newmin = height/(p->pmax-p->pmin)*(pmin-p->pmin);

      if(newmax>height*THRESH || newmin>height*THRESH){
	if(p->phtimer){
	  p->phtimer--;
	}else{
	  if(newmax>height*THRESH)
	    p->pmax -= PXDEL/(height-1)*(p->pmax-p->pmin);
	  if(newmin>height*THRESH)
	    p->pmin += PXDEL/(height-1)*(p->pmax-p->pmin);
	}
      }else{
	p->phtimer = TIMERFRAMES;
      }
    }else
      p->phtimer = TIMERFRAMES;
  }

  if(ymax<p->depth-p->ymax_limit)ymax=p->depth-p->ymax_limit;
  if(ymax>p->ymax_limit)ymax=p->ymax_limit;
  if(pmax>180)pmax=180;
  if(pmin<-180)pmin=-180;
  pmax+=10;
  pmin-=10;

  p->disp_depth = p->depth;

  if(p->autoscale){
    if(p->mode == 0){
      if(ymax>p->ymax)p->ymax=ymax;
      if(pmax>p->pmax)p->pmax=pmax;
      if(pmin<p->pmin)p->pmin=pmin;
    }else{
      p->ymax=ymax;
      p->pmax=pmax;
      p->pmin=pmin;
    }

    p->disp_ymax = p->ymax;
    p->disp_pmax = p->pmax;
    p->disp_pmin = p->pmin;
  }

  /* finally, align phase/response zeros on phase graphs */
  if(p->disp_ymax>-p->ymax_limit){
    if(phase){
      /* In a phase/response graph, 0dB/0degrees are bound and always on-screen. */
      float pzero,mzero = (height-1)/p->disp_depth*p->disp_ymax;

      /* move mag zero back onscreen if it's off */
      if(mzero < height*THRESH){
        p->disp_ymax = (p->disp_depth*height*THRESH)/(height-1);
      }
      if(mzero > height*(1-THRESH)){
        p->disp_ymax = (p->disp_depth*height*(1-THRESH))/(height-1);
      }

      mzero = (height-1)/p->disp_depth*p->disp_ymax;
      pzero = (height-1)/(p->disp_pmax-p->disp_pmin)*p->disp_pmax;

      if(mzero<pzero){
	/* straightforward; move the dB range down */
	p->disp_ymax = pzero*p->disp_depth/(height-1);
      }else{
	/* a little harder as phase has a min and a max.
	   First increase the pmax to match the dB zero. */

	p->disp_pmax = p->disp_pmin/(1-(height-1)/mzero);
	pzero = (height-1)/(p->disp_pmax-p->disp_pmin)*p->disp_pmax;

	/* That worked, but might have run p->max overrange */
	if(p->disp_pmax>190.){
	  /* reconcile by allowing mag to overrange */
	  p->disp_pmax = 190.;
	  pzero = (height-1)/(p->disp_pmax-p->disp_pmin)*p->disp_pmax;
          p->disp_ymax = p->disp_depth*pzero/(height-1);
	}
      }
    }
  }
}

void plot_clear (Plot *p){
  GtkWidget *widget=GTK_WIDGET(p);
  int phase = phase_active_p(p);
  int width=GTK_WIDGET(p)->allocation.width-p->padx-(phase ? p->phax : 0);
  int i,j;

  if(p->ydata)
    for(i=0;i<p->total_ch;i++)
      for(j=0;j<width*2;j++)
	p->ydata[i][j]=-300;
  p->ymax=p->depth-p->ymax_limit;
  p->pmax=0;
  p->pmin=0;
  draw_and_expose(widget);
}

float **plot_get (Plot *p){
  return(p->ydata);
}

void plot_setting (Plot *p, int scale, int mode, int link, int depth, int noise){
  GtkWidget *widget=GTK_WIDGET(p);
  p->scale=scale;
  p->mode=mode;
  p->depth=depth;
  p->link=link;
  p->noise=noise;

  if(depth>140)
    p->ymax_limit=depth;
  else
    p->ymax_limit=140;

  p->ymax=-p->ymax_limit;
  p->pmax=0;
  p->pmin=0;

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

void plot_set_autoscale(Plot *p, int a){
  GtkWidget *widget=GTK_WIDGET(p);
  p->autoscale=a;
  plot_refresh(p,NULL);
  draw_and_expose(widget);
}

void plot_set_bold(Plot *p, int b){
  GtkWidget *widget=GTK_WIDGET(p);
  p->bold=b;
  draw_and_expose(widget);
}

int plot_get_left_pad (Plot *m){
  return m->padx;
}

int plot_get_right_pad (Plot *m){
  return (phase_active_p(m) ? m->phax : 0);
}
