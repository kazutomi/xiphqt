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
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <gdk/gdk.h>
#include "spec_plot.h"

static double log_lfreqs[5]={10.,100.,1000.,10000.,100000};
static double log_llfreqs[16]={10.,20.,30.,50.,100.,200.,300.,500.,
                              1000.,2000.,3000.,5000.,10000.,
                               20000.,30000.,50000.};
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

#define PHAX_MIN 8

static void compute_xgrid(Plot *p, fetchdata *f){
  if(p->maxrate!=f->maxrate ||
     p->scale!=f->scale ||
     p->width != f->width){
    int width = f->width;
    int nyq=f->maxrate/2.;
    int i,j;

    p->xgrids=0;
    p->xtics=0;

    /* find the places to plot the x grid lines according to scale */
    switch(f->scale){
    case 0: /* log */

      for(i=0;i<5;i++){
        if(log_lfreqs[i]<(nyq-.1))
          p->xgrids=i+1;
      }
      for(i=0;i<16;i++){
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

      break;
    case 1: /* ISO log */

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

      break;
    case 2: /* linear spacing */

      if(f->maxrate > 100000){
        p->lin_major = 10000.;
        p->lin_minor = 2000.;
        p->lin_mult = 5;
        p->lin_layout = p->lin_layout_200;
      }else if(f->maxrate > 50000){
        p->lin_major = 5000.;
        p->lin_minor = 1000.;
        p->lin_mult = 5;
        p->lin_layout = p->lin_layout_100;
      }else{
        p->lin_major=2000.;
        p->lin_minor=500.;
        p->lin_mult=4;
        p->lin_layout = p->lin_layout_50;
      }

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
      break;
    }
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

  if(!GDK_IS_DRAWABLE(p->backing))return;
  if(!pp)return;
  if(!f)return;

  int phase = f->phase_active;
  int padx = p->padx;
  int phax = phase ? p->phax : PHAX_MIN;
  int pwidth = width - padx - phax;

  /* lazy GC init */
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

  /* set clip rectangle */
  {
    const GdkRectangle clip = {p->padx,0,pwidth,height-p->pady};
    GdkGCValues values;
    values.line_width=1;
    gdk_gc_set_values(p->drawgc,&values,GDK_GC_LINE_WIDTH);
    gdk_gc_set_clip_rectangle (p->drawgc, &clip);
  }

  /* clear the old rectangle out */
  {
    GdkGC *gc=parent->style->bg_gc[0];
    gdk_draw_rectangle(p->backing,gc,1,0,0,padx,height);
    gdk_draw_rectangle(p->backing,gc,1,0,height-p->pady,width,p->pady);
    gdk_draw_rectangle(p->backing,gc,1,width-phax,0,phax,height);

    gc=parent->style->white_gc;
    gdk_draw_rectangle(p->backing,gc,1,padx,0,pwidth,height-p->pady);
  }

  compute_xgrid(p,f);
  p->maxrate=f->maxrate;
  p->scale=f->scale;
  p->width=f->width;
  p->phase_active=f->phase_active;

  /* draw the light x grid */
  {
    int i;
    GdkColor rgb={0,0,0,0};

    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<p->xtics;i++)
      gdk_draw_line(p->backing,p->drawgc,p->xtic[i],0,
                    p->xtic[i],height-p->pady);
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
  {
    GdkColor rgb={0,0,0,0};
    float emheight = (height-p->pady)/p->pady;
    float emperdB = emheight/pp->depth;
    float pxperdB = (height-p->pady)/pp->depth;

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
         pp->depth*majordeltest[i]<maxmajorper){
        majordel=majordellist[i];
        break;
      }
    }

    /* choose appropriate minor and subminor spacing */
    int minordellist[]=
      { 10,  25,  50,  100, 250, 500, 1000, 2500, 5000,
        10000, 25000, -1, -1};
    float minordeltest[]=
      {100,  40,  20,   10,   4,   2,    1,   .4,   .2,
       .1,    .04, -1, -1};

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

    /* draw the light Y grid */
    rgb.red=0xc000;
    rgb.green=0xff00;
    rgb.blue=0xff00;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);
    gdk_gc_set_rgb_fg_color(p->dashes,&rgb);

    float ymin = (pp->ymax - pp->depth)*1000;
    int yval = rint((pp->ymax*1000/subminordel)+1)*subminordel;

    while(1){
      float ydel = (yval - ymin)/(pp->depth*1000);
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

    /* draw the dark Y grid */
    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    ymin = (pp->ymax - pp->depth)*1000;
    yval = rint((pp->ymax*1000/majordel)+1)*majordel;

    {
      int px,py,pxdB,pxN,pxMAX;
      pango_layout_get_pixel_size(p->db_layoutN,&pxN,&py);
      pango_layout_get_pixel_size(p->db_layoutdB,&pxdB,&py);
      pango_layout_get_pixel_size(p->db_layout[0],&pxMAX,&py);
      pxMAX+=pxdB;

      while(1){
        float ydel = (yval - ymin)/(pp->depth*1000);
        int ymid = rint(height-p->pady-1 - (height-p->pady) * ydel);

        if(ymid>=height-p->pady)break;

        if(ymid>=0){
          int label = yval/100+2000;

          if(label>=0 && label<=4000 /* in range check */
             && (p->scale<2 || ymid+py/2 < height-p->pady)
             /* don't collide with DC label */
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

  /* draw the dark x grid */
  {
    int i;
    GdkColor rgb={0,0,0,0};

    rgb.red=0x0000;
    rgb.green=0xc000;
    rgb.blue=0xc000;
    gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

    for(i=0;i<p->xgrids;i++)
      gdk_draw_line(p->backing,p->drawgc,p->xgrid[i],0,
                    p->xgrid[i],height-p->pady);
  }


  gdk_gc_set_line_attributes(p->drawgc,pp->bold+1,GDK_LINE_SOLID,
                             GDK_CAP_PROJECTING, GDK_JOIN_MITER);

  /* draw actual data */
  if(f->data){
    int cho=0;
    int gi;
    for(gi=0;gi<f->groups;gi++){
      int ch;
      GdkColor rgb;

      for(ch=cho;ch<cho+f->channels[gi];ch++){
        if(f->active[ch]){

          rgb = chcolor(ch);
          gdk_gc_set_rgb_fg_color(p->drawgc,&rgb);

          for(i=0;i<pwidth;i++){
            float valmin=f->data[ch][i*2];
            float valmax=f->data[ch][i*2+1];
            float ymin, ymax;

            if(!isnan(valmin) && !isnan(valmax)){

              if(phase && ch==cho+1){

                ymin = rint((height-p->pady-1)/
                            (pp->pmax-pp->pmin)*
                            (pp->pmax-valmin));
                ymax = rint((height-p->pady-1)/
                            (pp->pmax-pp->pmin)*
                            (pp->pmax-valmax));

              }else{

                ymin = rint((height-p->pady-1)/pp->depth*(pp->ymax-valmin));
                ymax = rint((height-p->pady-1)/pp->depth*(pp->ymax-valmax));

              }

              if(ymin>32767)ymin=32767;
              if(ymax>32767)ymax=32767;
              if(ymin<-32768)ymin=-32768;
              if(ymax<-32768)ymax=-32768;

              gdk_draw_line(p->backing,p->drawgc,padx+i,ymin,padx+i,ymax);
            }
          }
        }
      }
      cho+=f->channels[gi];
    }
  }

  /* draw in phase labels and tics on right axis */
  /* currently messy as hell cut&paste spaghetti, but working */
  if(phase){
    float depth = pp->pmax-pp->pmin;
    float del=(height-p->pady-1)/depth;
    float off=pp->pmax-ceil(pp->pmax*.1)*10;

    for(i=0;;i++){
      int ymid=rint(del * (i*10+off));
      int pv = rint((pp->pmax - ymid/(float)(height-p->pady) *
                     (pp->pmax - pp->pmin))/10);
      if(ymid>=height-p->pady)break;
      if(ymid>=0 && pv>=-18 && pv<=18 && ((pv&1)==0 || del>2)){
        int px,py;
        pango_layout_get_pixel_size(p->phase_layout[pv+18],&px,&py);
        gdk_draw_layout (p->backing,p->phasegc,
                         width-phax+2, ymid-py/2,
                         p->phase_layout[pv+18]);
      }
    }

    if(del>10){
      for(i=0;;i++){
        int ymid=rint(del * (i+off));
        int pv = rint(pp->pmax - ymid/(float)(height-p->pady) *
                      (pp->pmax - pp->pmin));
        if(ymid>=height-p->pady)break;
        if(ymid>=0 && pv>=-180 && pv<=180)
          gdk_draw_line(p->backing,p->phasegc,width-phax-(i%5==0?15:10),
                        ymid,width-phax-(i%5==0?5:7),ymid);
      }
    }else if(del>5){
      for(i=0;;i++){
        int ymid=rint(del * (i*2+off));
        int pv = rint(pp->pmax - ymid/(float)(height-p->pady) *
                      (pp->pmax - pp->pmin));
        if(ymid>=height-p->pady)break;
        if(ymid>=0 && pv>=-180 && pv<=180)
          gdk_draw_line(p->backing,p->phasegc,width-phax-12,ymid,
                        width-phax-7,ymid);
      }
    } else if(del>2){
      for(i=0;;i++){
        int ymid=rint(del * (i*5+off));
        int pv = rint(pp->pmax - ymid/(float)(height-p->pady) *
                      (pp->pmax - pp->pmin));
        if(ymid>=height-p->pady)break;
        if(ymid>=0 && pv>=-180 && pv<=180)
          gdk_draw_line(p->backing,p->phasegc,width-phax-15,ymid,
                        width-phax-5,ymid);
      }
    }

    if(del>=2){
      for(i=0;;i++){
        int ymid=rint(del * (i*10+off));
        int pv = rint(pp->pmax - ymid/(float)(height-p->pady) *
                      (pp->pmax - pp->pmin));
        if(ymid>=height-p->pady)break;
        if(ymid>=0 && pv>=-180 && pv<=180){
          gdk_draw_line(p->backing,p->phasegc,width-phax-5,ymid-1,
                        width-phax-1,ymid-1);
          gdk_draw_line(p->backing,p->phasegc,width-phax-25,ymid,
                        width-phax-1,ymid);
          gdk_draw_line(p->backing,p->phasegc,width-phax-5,ymid+1,
                        width-phax-1,ymid+1);
        }
      }
    }else{

      for(i=0;;i++){
        int ymid=rint(del * (i*10+off));
        int pv = rint(pp->pmax - ymid/(float)(height-p->pady) *
                      (pp->pmax - pp->pmin));
        if(ymid>=height-p->pady)break;
        if(ymid>=0 && pv>=-180 && pv<=180)
          gdk_draw_line(p->backing,p->phasegc,width-phax-15,ymid,
                        width-phax-5,ymid);
      }

      for(i=0;;i++){
        int ymid=rint(del * (i*10+off));
        int pv = rint((pp->pmax - ymid/(float)(height-p->pady) *
                       (pp->pmax - pp->pmin))/10);
        if(ymid>=height-p->pady)break;
        if(ymid>=0 && pv>=-18 && pv<=18 && (pv&1)==0){
          gdk_draw_line(p->backing,p->phasegc,width-phax-5,ymid-1,
                        width-phax-1,ymid-1);
          gdk_draw_line(p->backing,p->phasegc,width-phax-25,ymid,
                        width-phax-1,ymid);
          gdk_draw_line(p->backing,p->phasegc,width-phax-5,ymid+1,
                        width-phax-1,ymid+1);
        }
      }
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
  int axisy=0,axisx=0,pady=0,padx=0,phax=0,px,py,i;

  /* find max lin layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->lin_layout_50[i];i++){
      pango_layout_get_pixel_size(p->lin_layout_50[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max*=i+1;
    max+=maxy*1.5;
    if(axisx<max)axisx=max;
  }
  /* find max log layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->log_layout[i];i++){
      pango_layout_get_pixel_size(p->log_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max*=(i+1);
    max+=maxy*1.5;
    if(axisx<max)axisx=max;
  }
  /* find max iso layout */
  {
    int max=0;
    int maxy=0;
    for(i=0;p->iso_layout[i];i++){
      pango_layout_get_pixel_size(p->iso_layout[i],&px,&py);
      if(px>max)max=px;
      if(py>pady)pady=py;
      if(py>maxy)maxy=py;
    }
    max*=i+1;
    max+=maxy*1.5;
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

  if(!p->configured){
    p->configured=1;
    replot(1,0,0);
  }else{
    replot(0,0,0);
  }
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
  int i;

  /* generate all the text layouts we'll need */
  /* linear X scale */
  {
    char *labels[10]={"DC","10kHz","20kHz","30kHz","40kHz","50kHz","60kHz",
                        "70kHz","80kHz","90kHz"};
    p->lin_layout_200=calloc(11,sizeof(*p->lin_layout_200));
    for(i=0;i<10;i++)
      p->lin_layout_200[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[10]={"DC","5kHz","10kHz","15kHz","20kHz","25kHz","30kHz",
                      "35kHz","40kHz","45kHz"};
    p->lin_layout_100=calloc(11,sizeof(*p->lin_layout_100));
    for(i=0;i<10;i++)
      p->lin_layout_100[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }
  {
    char *labels[13]={"DC","2kHz","4kHz","6kHz","8kHz","10kHz","12kHz",
                      "14kHz","16kHz","18kHz","20kHz","22kHz","24kHz"};
    p->lin_layout_50=calloc(14,sizeof(*p->lin_layout_50));
    for(i=0;i<13;i++)
      p->lin_layout_50[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }

  /* log X scale */
  {
    char *labels[16]={"10","20","30","50","100",
                     "200","300","500","1kHz",
                     "2kHz","3kHz","5k","10kHz",
                      "20k","30k","50k"};
    p->log_layout=calloc(17,sizeof(*p->log_layout));
    for(i=0;i<16;i++)
      p->log_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
  }

  /* ISO log X scale */
  {
    char *labels[12]={"31","63","125","250","500","1kHz","2kHz",
		      "4kHz","8kHz","16kHz","32kHz","64kHz"};
    p->iso_layout=calloc(13,sizeof(*p->iso_layout));
    for(i=0;i<12;i++)
      p->iso_layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);
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

  return ret;
}

int plot_get_left_pad (Plot *m){
  return m->padx;
}

int plot_get_right_pad (Plot *m){
  return (m->phase_active ? m->phax : PHAX_MIN);
}

int plot_width (Plot *m,int ph){
  return GTK_WIDGET(m)->allocation.width-m->padx-(ph?m->phax:PHAX_MIN);
}

int plot_height (Plot *m){
  return GTK_WIDGET(m)->allocation.height-m->pady;
}
