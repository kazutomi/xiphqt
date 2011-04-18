/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007-2011              *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: graphing code for chirp tests
 last mod: $Id$

 ********************************************************************/

#define _GNU_SOURCE
#include <math.h>
#include "chirp.h"
#include "chirpgraph.h"
#include "scales.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <pthread.h>

/********************** colors for graphs *********************************/

void set_iter_color(cairo_t *cC, int ret, float a){
  if (ret>100){
    cairo_set_source_rgba(cC,1,1,1,a); /* white */
  }else if (ret>50){ /* 51(black)-100(white) */
    cairo_set_source_rgba(cC,(ret-50)*.02,(ret-50)*.02,(ret-50)*.02,a);
  }else if (ret>30){ /* 31(red) - 50(black) */
    cairo_set_source_rgba(cC,1-(ret-30)*.05,0,0,a);
  }else if (ret>10){ /* 11(yellow) - 30(red) */
    cairo_set_source_rgba(cC,1,1-(ret-10)*.05,0,a);
  }else if (ret>4){ /*  5 (green) - 10 (yellow) */
    cairo_set_source_rgba(cC,(ret-5)*.15+.25,1,0,a);
  }else if (ret==4){ /*  4 brighter cyan */
    cairo_set_source_rgba(cC,0,.8,.4,a);
  }else if (ret==3){  /* 3 cyan */
    cairo_set_source_rgba(cC,0,.6,.6,a);
  }else if (ret==2){  /* 2 dark cyan */
    cairo_set_source_rgba(cC,0,.4,.8,a);
  }else if (ret==1){ /* 1 blue */
    cairo_set_source_rgba(cC,.1,.2,1,a);
  }else{ /* dark blue */
    cairo_set_source_rgba(cC,.2,0,1,a);
  }
}

void set_error_color(cairo_t *c, float err,float a){
  if(isnan(err) || fabs(err)>1.){
    cairo_set_source_rgba(c,1,1,1,a); /* white */
  }else{
    err=fabs(err);
    if(err>.1){
      cairo_set_source_rgba(c,(err-.1)/.9,(err-.1)/.9,(err-.1)/.9,a); /* white->black */
    }else if(err>.01){
      cairo_set_source_rgba(c,1.-((err-.01)/.09),0,0,a); /* black->red */
    }else if(err>.001){
      cairo_set_source_rgba(c,1,1.-((err-.001)/.009),0,a); /* red->yellow */
    }else if(err>.0001){
      cairo_set_source_rgba(c,((err-.0001)/.0009),1,0,a); /* yellow->green */
    }else if(err>.00001){
      cairo_set_source_rgba(c,0,1,1.-((err-.00001)/.00009),a); /* green->cyan */
    }else if(err>.000001){
      cairo_set_source_rgba(c,.2-((err-.000001)/.000009)*.2,
                            ((err-.000001)/.000009)*.8+.2,1,a); /* cyan->blue */
    }else{
      cairo_set_source_rgba(c,.1,.1,1,a); /* blue */
    }
  }
}

/********* draw everything in the graph except the graph data itself *******/

static float fontsize=12;
static int x0s,x1s,xmajor;
static int y0s,y1s,ymajor;

/* computed configuration globals */
int leftpad=0;
static int rightpad=0;
int toppad=0;
static int bottompad=0;
static float legendy=0;
static float legendh=0;
static int pic_w=0;
static int pic_h=0;
static float titleheight=0.;
static int x_n=0;
static int y_n=0;

/* determines padding, etc.  Will not expand the frame to prevent
   overruns of user-set titles/legends */
void setup_graphs(int start_x_step,
                  int end_x_step, /* inclusive; not one past */
                  int x_major_d,

                  int start_y_step,
                  int end_y_step, /* inclusive; not one past */
                  int y_major_d,

                  int subtitles,
                  float fs){

  /* determine ~ padding needed */
  cairo_surface_t *cs=cairo_image_surface_create(CAIRO_FORMAT_RGB24,100,100);
  cairo_t *ct=cairo_create(cs);
  cairo_text_extents_t extents;
  cairo_font_extents_t fextents;
  int y;

  x_n = end_x_step-start_x_step+1;
  y_n = end_y_step-start_y_step+1;
  fontsize=fs;
  x0s=start_x_step;
  x1s=end_x_step;
  xmajor=x_major_d;
  y0s=start_y_step;
  y1s=end_y_step;
  ymajor=y_major_d;

  /* y labels */
  cairo_set_font_size(ct, fontsize);
  for(y=y0s+fontsize;y<=y1s;y++){
    if(y%ymajor==0){
      char buf[80];
      snprintf(buf,80,"%.0f",(float)y/ymajor);
      cairo_text_extents(ct, buf, &extents);
      if(extents.width + extents.height*3.5>leftpad)leftpad=extents.width + extents.height*3.5;
    }
  }

  /* x labels */
  cairo_font_extents(ct, &fextents);
  if(fextents.height*3>bottompad)bottompad=fextents.height*3;

  /* center horizontally */
  if(leftpad<rightpad)leftpad=rightpad;
  if(rightpad<leftpad)rightpad=leftpad;

  /* top legend */
  {
    float sofar=0;
    cairo_save(ct);
    cairo_select_font_face (ct, "",
                            CAIRO_FONT_SLANT_NORMAL,
                            CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(ct,fontsize*1.25);
    cairo_font_extents(ct, &fextents);
    sofar+=fextents.height;
    cairo_restore(ct);

    cairo_font_extents(ct, &fextents);
    while(subtitles--)
      sofar+=fextents.height;
    if(sofar>titleheight)titleheight=sofar;
    if(toppad<titleheight+fontsize*2)toppad=titleheight+fontsize*2;
  }

  /* color legend */
  {
    float width=0.;
    float w1,w2,w3;

    cairo_text_extents(ct, "100", &extents);
    w1=extents.width*2*12;
    cairo_text_extents(ct, ".000001", &extents);
    w2=extents.width*1.5*7;
    cairo_text_extents(ct, ".0001%", &extents);
    w3=extents.width*1.5*7;

    width+=MAX(w1,MAX(w2,w3));

    legendy = y_n+extents.height*4;
    legendh = extents.height*1.5;

    if(legendy+legendh*4>bottompad)
      bottompad=(legendy-y_n)+legendh*2.5;
  }

  pic_w = x_n + leftpad + rightpad;
  pic_h = y_n + toppad + bottompad;
}

/* draws the page surrounding the graph data itself */
cairo_t *draw_page(char *title,
                   char *subtitle1,
                   char *subtitle2,
                   char *subtitle3,
                   char *xaxis_label,
                   char *yaxis_label,
                   char *legend_label,
                   int datatype){

  int i;
  cairo_text_extents_t extents;
  cairo_font_extents_t fextents;
  cairo_surface_t *cs=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
  if(!cs || cairo_surface_status(cs)!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not set up Cairo surface.\n\n");
    exit(1);
  }
  cairo_t *c = cairo_create(cs);
  cairo_save(c);

  /* clear page to white */
  cairo_set_source_rgb(c,1,1,1);
  cairo_rectangle(c,0,0,pic_w,pic_h);
  cairo_fill(c);
  cairo_set_font_size(c, fontsize);

  /* set graph area to transparent */
  cairo_set_source_rgba(c,0,0,0,0);
  cairo_set_operator(c,CAIRO_OPERATOR_SOURCE);
  cairo_rectangle(c,leftpad,toppad,x_n,y_n);
  cairo_fill(c);
  cairo_restore(c);

  /* Y axis numeric labels */
  cairo_set_source_rgb(c,0,0,0);
  for(i=y0s+fontsize;i<=y1s;i++){
    if(i%ymajor==0){
      int y = toppad+y_n-i+y0s;
      char buf[80];

      snprintf(buf,80,"%.0f",(float)i/ymajor);
      cairo_text_extents(c, buf, &extents);
      cairo_move_to(c,leftpad - fontsize*.5 - extents.width,y+extents.height*.5);
      cairo_show_text(c,buf);
    }
  }

  /* X axis labels */
  {
    int xadv=0;
    for(i=x0s;i<=x1s;i++){
      if(i%xmajor==0){
        char buf[80];
        int x = leftpad + i - x0s;
        if(i==0){
          snprintf(buf,80,"DC");
        }else{
          snprintf(buf,80,"%.0f",(float)i/xmajor);
        }
        cairo_text_extents(c, buf, &extents);
        if(x-extents.width/2 - fontsize > xadv){
          cairo_move_to(c,x - extents.width/2,y_n+toppad+extents.height+fontsize*.5);
          cairo_show_text(c,buf);
          xadv = x+extents.width/2;
        }
      }
    }
  }

  /* Y axis label */
  {
    cairo_matrix_t a;
    cairo_matrix_t b = {0.,-1., 1.,0., 0.,0.}; // account for border!
    cairo_matrix_t d;
    cairo_text_extents(c, yaxis_label, &extents);
    cairo_move_to(c,extents.height+fontsize,y_n/2+toppad+extents.width*.5);

    cairo_save(c);
    cairo_get_matrix(c,&a);
    cairo_matrix_multiply(&d,&a,&b);
    cairo_set_matrix(c,&d);
    cairo_show_text(c,yaxis_label);
    cairo_restore(c);
  }

  /* X axis caption */
  {
    cairo_text_extents(c, xaxis_label, &extents);
    cairo_move_to(c,pic_w/2-extents.width/2,y_n+toppad+extents.height*2+fontsize*.5);
    cairo_show_text(c,xaxis_label);
  }

  /* top title(s) */
  {
    float y = (toppad-titleheight);
    if(title){
      cairo_save(c);
      cairo_select_font_face (c, "",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(c,fontsize*1.25);
      cairo_font_extents(c, &fextents);
      cairo_text_extents(c, title, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,title);
      y+=fextents.height;
      cairo_restore(c);
    }
    cairo_font_extents(c, &fextents);
    if(subtitle1){
      cairo_text_extents(c, subtitle1, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,subtitle1);
      y+=fextents.height;
    }
    if(subtitle2){
      cairo_text_extents(c, subtitle2, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,subtitle2);
      y+=fextents.height;
    }
    if(subtitle3){
      cairo_text_extents(c, subtitle3, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,subtitle3);
      y+=fextents.height;
    }
  }

  /* color legend */
  {
    float cw;
    cairo_text_extents(c, "100", &extents);
    cw=extents.width*2*11;
    cairo_text_extents(c, ".000001", &extents);
    cw=MAX(cw,extents.width*1.5*7);
    cairo_text_extents(c, ".0001%", &extents);
    cw=MAX(cw,extents.width*1.5*7);

    if(datatype==DT_iterations){
      char buf[80];
      int i;
      float w=cw/11,px=leftpad+x_n;

      for(i=0;i<=100;i++){
        switch(i){
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 10:
        case 20:
        case 30:
        case 40:
        case 50:
        case 100:
          px-=w;
          cairo_rectangle(c,px,legendy+toppad,w,legendh*1.25);
          cairo_set_source_rgb(c,0,0,0);
          cairo_stroke_preserve(c);
          set_iter_color(c, i, 1.);
          cairo_fill(c);

          snprintf(buf,80,"%d",i);
          cairo_text_extents(c, buf, &extents);
          cairo_move_to(c,px+w/2-extents.width*.5,legendy+toppad+legendh*.625+extents.height/2);
          cairo_set_source_rgba(c,1,1,1,.7);
          cairo_text_path (c, buf);
          cairo_stroke_preserve(c);
          cairo_set_source_rgb(c,0,0,0);
          cairo_fill(c);

          break;
        }
      }
      cairo_text_extents(c, legend_label, &extents);
      cairo_move_to(c,px-extents.width-legendh*.75,toppad+legendy+legendh*.625+extents.height*.5);
      cairo_show_text(c,legend_label);
    }else{
      int per_p = (datatype==DT_percent);
      int i;
      float w=cw/7,px=leftpad+x_n;

      for(i=0;i<7;i++){
        char *buf;

        px-=w;
        cairo_rectangle(c,px,legendy+toppad,w,legendh*1.25);
        cairo_set_source_rgb(c,0,0,0);
        cairo_stroke_preserve(c);

        switch(i){
        case 0:
          buf=(per_p?".0001%":".000001");
          set_error_color(c, 0., 1.);
          break;
        case 1:
          buf=(per_p?".001%":".00001");
          set_error_color(c, .00001, 1.);
          break;
        case 2:
          buf=(per_p?".01%":".0001");
          set_error_color(c, .0001, 1.);
          break;
        case 3:
          buf=(per_p?".1%":".001");
          set_error_color(c, .001, 1.);
          break;
        case 4:
          buf=(per_p?"1%":".01");
          set_error_color(c, .01, 1.);
          break;
        case 5:
          buf=(per_p?"10%":".1");
          set_error_color(c, .1, 1.);
          break;
        case 6:
          buf=(per_p?"100%":"1");
          set_error_color(c, 1., 1.);
          break;
        }
        cairo_fill(c);

        cairo_text_extents(c, buf, &extents);
        cairo_move_to(c,px+w/2-extents.width*.5,legendy+toppad+legendh*.625+extents.height/2);
        cairo_set_source_rgba(c,1,1,1,.7);
        cairo_text_path (c, buf);
        cairo_stroke_preserve(c);
        cairo_set_source_rgb(c,0,0,0);
        cairo_fill(c);
      }
      cairo_text_extents(c, legend_label, &extents);
      cairo_move_to(c,px-extents.width-legendh*.75,toppad+legendy+legendh*.625+extents.height*.5);
      cairo_show_text(c,legend_label);
    }
  }

  return c;
}

void to_png(cairo_t *c,char *base, char *name){
  if(c){
    cairo_surface_t *cs=cairo_get_target(c);
    char buf[320];
    snprintf(buf,320,"%s-%s.png",base,name);
    cairo_surface_write_to_png(cs,buf);
  }
}

void destroy_page(cairo_t *c){
  if(c){
    cairo_surface_t *cs=cairo_get_target(c);
    cairo_destroy(c);
    cairo_surface_destroy(cs);
  }
}
