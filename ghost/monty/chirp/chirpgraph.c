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
#include "scales.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <pthread.h>

/* configuration globals. */
int BLOCKSIZE=2048;
int cores=8;
int xstepd=100;
int ystepd=100;
int xstepg=25;
int ystepg=25;
int x_n=801;
int y_n=300;
int fontsize=12;

int randomize_est_A=0;
int randomize_est_P=0;
int randomize_est_dA=0;
int randomize_est_dW=0;
int randomize_est_ddA=0;

int randomize_chirp_P=0;
int randomize_chirp_dA=0;
int randomize_chirp_dW=0;
int randomize_chirp_ddA=0;

float initial_est_A = 0.;
float initial_est_P = 0.;
float initial_est_dA = 0.;
float initial_est_dW = 0.;
float initial_est_ddA = 0.;

float initial_chirp_A = 1.;
float initial_chirp_P = M_PI/4;
float initial_chirp_dA = 0.;
float initial_chirp_dW = 0.;
float initial_chirp_ddA = 0.;

int fit_linear=0;
int fit_W=1;
int fit_dA=1;
int fit_dW=1;
int fit_ddA=1;
int fit_symm_norm=1;
int fit_gauss_seidel=1;
int fit_bound_zero=0;
float fit_tolerance=.000001;
int fit_hanning=1;
int iterations=-1;

void hanning(float *x, int n){
  float scale = 2*M_PI/n;
  int i;
  for(i=0;i<n;i++){
    float i5 = i+.5;
    x[i] = .5-.5*cos(scale*i5);
  }
}

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
  }else if (ret>4){ /*  5 (green) - 10(yellow) */
    cairo_set_source_rgba(cC,(ret-5)*.15+.25,1,0,a);
  }else if (ret==4){ /*  4 brighter cyan */
    cairo_set_source_rgba(cC,0,.8,.4,a);
  }else if (ret==3){  /* 3 cyan */
    cairo_set_source_rgba(cC,0,.6,.6,a);
  }else if (ret==2){  /* 2 dark cyan */
    cairo_set_source_rgba(cC,0,.4,.8,a);
  }else{ /* 1 blue */
    cairo_set_source_rgba(cC,.1,.1,1,a);
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

#define DT_iterations 0
#define DT_abserror 1
#define DT_percent 2

/* computed configuration globals */
int leftpad=0;
int rightpad=0;
int toppad=0;
int bottompad=0;
float legendy=0;
float legendh=0;
int pic_w=0;
int pic_h=0;
float titleheight=0.;

/* determines padding, etc.  Call several times to determine maximums. */
void setup_graph(char *title1,
                 char *title2,
                 char *title3,
                 char *xaxis_label,
                 char *yaxis_label,
                 char *legend_label,
                 int datatype){

  /* determine ~ padding needed */
  cairo_surface_t *cs=cairo_image_surface_create(CAIRO_FORMAT_RGB24,100,100);
  cairo_t *ct=cairo_create(cs);
  int minx=-leftpad,maxx=x_n+rightpad;
  int miny=-toppad,maxy=2*y_n+1+bottompad;
  int textpad=fontsize;
  cairo_text_extents_t extents;
  int x,y;

  /* y labels */
  cairo_set_font_size(ct, fontsize);
  for(y=0;y<=y_n;y+=ystepd){
    char buf[80];
    snprintf(buf,80,"%.0f",(float)y/ystepd);
    cairo_text_extents(ct, buf, &extents);
    if(-textpad-extents.height*.5<miny)miny=-textpad-extents.height*.5;
    if(-textpad-extents.width-extents.height*1.5<minx)minx=-textpad-extents.width-extents.height*1.5;

    if(y>0 && y<y_n){
      snprintf(buf,80,"%.0f",(float)-y/ystepd);
      cairo_text_extents(ct, buf, &extents);
      if(-textpad-extents.height*.5<miny)miny=-textpad-extents.height*.5;
      if(-textpad-extents.width<minx)minx=-textpad-extents.width;
    }
  }

  /* x labels */
  for(x=0;x<=x_n;x+=xstepd){
    char buf[80];
    snprintf(buf,80,"%.0f",(float)x/xstepd);
    cairo_text_extents(ct, buf, &extents);

    if(y_n*2+1+extents.height*3>maxy)maxy=y_n*2+1+extents.height*3;
    if(x-(extents.width/2)-textpad<minx)minx=x-(extents.width/2)-textpad;
    if(x+(extents.width/2)+textpad>maxx)maxx=x+(extents.width/2)+textpad;
  }

  /* center horizontally */
  if(maxx-x_n < -minx)maxx = x_n-minx;
  if(maxx-x_n > -minx)minx = x_n-maxx;

  /* top legend */
  {
    float sofar=0;
    if(title1){
      cairo_save(ct);
      cairo_select_font_face (ct, "",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(ct,fontsize*1.25);
      cairo_text_extents(ct, title1, &extents);

      if(extents.width+textpad > maxx-minx){
        int moar = extents.width+textpad-maxx+minx;
        minx-=moar/2;
        maxx+=moar/2;
      }
      sofar+=extents.height;
      cairo_restore(ct);
    }
    if(title2){
      cairo_text_extents(ct, title2, &extents);

      if(extents.width+textpad > maxx-minx){
        int moar = extents.width+textpad-maxx+minx;
        minx-=moar/2;
        maxx+=moar/2;
      }
      sofar+=extents.height;
    }
    if(title3){
      cairo_text_extents(ct, title3, &extents);

      if(extents.width+textpad > maxx-minx){
        int moar = extents.width+textpad-maxx+minx;
        minx-=moar/2;
        maxx+=moar/2;
      }
      sofar+=extents.height;
    }
    if(sofar>titleheight)titleheight=sofar;
    if(-miny<titleheight+textpad*2)miny=-titleheight-textpad*2;

  }

  /* color legend */
  {
    float width=0.;
    float w1,w2,w3;
    cairo_text_extents(ct, legend_label, &extents);
    width=extents.width;

    cairo_text_extents(ct, "100", &extents);
    w1=extents.width*2*11;
    cairo_text_extents(ct, ".000001", &extents);
    w2=extents.width*1.5*7;
    cairo_text_extents(ct, ".0001%", &extents);
    w3=extents.width*1.5*7;

    width+=MAX(w1,MAX(w2,w3));

    legendy = y_n*2+1+extents.height*4;
    legendh = extents.height*1.5;

    if(width + textpad > x_n-minx){
      int moar = width+textpad-x_n;
      minx-=moar;
      maxx+=moar;
    }
    if(legendy+legendh*4>maxy)
      maxy=legendy+legendh*2.5;
  }

  toppad = -miny;
  leftpad = -minx;
  rightpad = maxx-x_n;
  bottompad = maxy-(y_n*2+1);
  if(toppad<leftpad*.75)toppad = leftpad*.75;
  if(leftpad<toppad)leftpad=toppad;
  if(rightpad<toppad)rightpad=toppad;

  pic_w = maxx-minx;
  pic_h = maxy-miny;
}

/* draws the page surrounding the graph data itself */
void draw_page(cairo_t *c,
               char *title1,
               char *title2,
               char *title3,
               char *xaxis_label,
               char *yaxis_label,
               char *legend_label,
               int datatype){

  int i;
  cairo_text_extents_t extents;
  cairo_save(c);

  /* clear page to white */
  cairo_set_source_rgb(c,1,1,1);
  cairo_rectangle(c,0,0,pic_w,pic_h);
  cairo_fill(c);

  /* set graph area to transparent */
  cairo_set_source_rgba(c,0,0,0,0);
  cairo_set_operator(c,CAIRO_OPERATOR_SOURCE);
  cairo_rectangle(c,leftpad,toppad,x_n,y_n*2+1);
  cairo_fill(c);
  cairo_restore(c);

  /* Y axis numeric labels */
  cairo_set_source_rgb(c,0,0,0);
  for(i=0;i<=y_n;i+=ystepd){
    int y = toppad+y_n;
    char buf[80];

    snprintf(buf,80,"%.0f",(float)i/ystepd);
    cairo_text_extents(c, buf, &extents);
    cairo_move_to(c,leftpad - fontsize*.5 - extents.width,y-i+extents.height*.5);
    cairo_show_text(c,buf);

    if(i>0 && i<y_n){

      snprintf(buf,80,"%.0f",(float)-i/ystepd);
      cairo_text_extents(c, buf, &extents);
      cairo_move_to(c,leftpad - fontsize*.5 - extents.width,y+i+extents.height*.5);
      cairo_show_text(c,buf);

    }
  }

  /* X axis labels */
  for(i=0;i<=x_n;i+=xstepd){
    char buf[80];

    if(i==0){
      snprintf(buf,80,"DC");
    }else{
      snprintf(buf,80,"%.0f",(float)i/xstepd);
    }
    cairo_text_extents(c, buf, &extents);
    cairo_move_to(c,leftpad + i - extents.width/2,y_n*2+1+toppad+extents.height+fontsize*.5);
    cairo_show_text(c,buf);
  }

  /* Y axis label */
  {
    cairo_matrix_t a;
    cairo_matrix_t b = {0.,-1., 1.,0., 0.,0.}; // account for border!
    cairo_matrix_t d;
    cairo_text_extents(c, yaxis_label, &extents);
    cairo_move_to(c,leftpad-extents.height*2,y_n+toppad+extents.width*.5);

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
    cairo_move_to(c,pic_w/2-extents.width/2,y_n*2+1+toppad+extents.height*2+fontsize*.5);
    cairo_show_text(c,xaxis_label);
  }

  /* top title(s) */
  {
    float y = (toppad-titleheight);
    if(title1){
      cairo_save(c);
      cairo_select_font_face (c, "",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(c,fontsize*1.25);
      cairo_text_extents(c, title1, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,title1);
      y+=extents.height;
      cairo_restore(c);
    }
    if(title2){
      cairo_text_extents(c, title2, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,title2);
      y+=extents.height;
    }
    if(title3){
      cairo_text_extents(c, title3, &extents);
      cairo_move_to(c,pic_w/2-extents.width/2,y);
      cairo_show_text(c,title3);
      y+=extents.height;
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

      for(i=1;i<=100;i++){
        switch(i){
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
}

void to_png(cairo_surface_t *c,char *base, char *name){
  if(c){
    char buf[320];
    snprintf(buf,320,"%s-%s.png",name,base);
    cairo_surface_write_to_png(c,buf);
  }
}

/*********************** Plot w estimate error against w **********************/

typedef struct {
  float *in;
  float *window;

  chirp *c;
  int *ret;
  int max_y;
} vearg;


pthread_mutex_t ymutex = PTHREAD_MUTEX_INITIALIZER;
int next_y=0;

#define _GNU_SOURCE
#include <fenv.h>

void *w_e_column(void *in){
  float rec[BLOCKSIZE];
  vearg *arg = (vearg *)in;
  int y;
  chirp save;

  while(1){
    pthread_mutex_lock(&ymutex);
    y=next_y;
    if(y>=arg->max_y){
      pthread_mutex_unlock(&ymutex);
      return NULL;
    }
    next_y++;
    pthread_mutex_unlock(&ymutex);

    int except=feenableexcept(FE_ALL_EXCEPT);
    fedisableexcept(FE_INEXACT);
    fedisableexcept(FE_UNDERFLOW);
    save=arg->c[y];

    /* iterate only until convergence */
    arg->ret[y]=
      estimate_chirps(arg->in,rec,arg->window,BLOCKSIZE,
                      arg->c+y,1,iterations,fit_tolerance,
                      fit_linear,
                      fit_W,
                      fit_dA,
                      fit_dW,
                      fit_ddA,
                      fit_symm_norm,
                      fit_gauss_seidel,
                      fit_bound_zero);

    /* continue iterating to get error numbers for a fixed large
       number of iterations.  The linear estimator must be restarted
       from the beginning, else 'continuing' causes it to recenter the
       basis-- which renders it nonlinear 
    if(fit_linear) arg->c[y]=save;
    int ret=estimate_chirps(arg->in,rec,arg->window,BLOCKSIZE,
                    arg->c+y,1,fit_linear?iterations:arg->ret[y],-1.,
                    fit_linear,
                    fit_W,
                    fit_dA,
                    fit_dW,
                    fit_ddA,
                    fit_symm_norm,
                    fit_gauss_seidel,
                    fit_bound_zero);*/
    arg->ret[y] = iterations-arg->ret[y];
    feclearexcept(FE_ALL_EXCEPT);
    feenableexcept(except);

  }
  return NULL;
}

void w_e(){
  float window[BLOCKSIZE];
  float in[BLOCKSIZE];
  int i,x,y;

  cairo_surface_t *converge=NULL;
  cairo_surface_t *Aerror=NULL;
  cairo_surface_t *Perror=NULL;
  cairo_surface_t *Werror=NULL;
  cairo_surface_t *dAerror=NULL;
  cairo_surface_t *dWerror=NULL;
  cairo_surface_t *ddAerror=NULL;

  cairo_t *cC=NULL;
  cairo_t *cA=NULL;
  cairo_t *cP=NULL;
  cairo_t *cW=NULL;
  cairo_t *cdA=NULL;
  cairo_t *cdW=NULL;
  cairo_t *cddA=NULL;

  pthread_t threads[cores];
  vearg arg[cores];

  char *filebase="W-vs-Westimate";
  char *yaxis_label = "initial distance from W (cycles/block)";
  char *xaxis_label = "W (cycles/block)";
  char *title2=NULL;
  char *title3=NULL;

  pic_w = x_n;
  pic_h = y_n*2+1;
  if(iterations<0)iterations=100;

  /* determine ~ padding needed */

  setup_graph("Convergence",title2,title3,xaxis_label,yaxis_label,
              "Iterations:",DT_iterations);
  setup_graph("A (Amplitude) Error",title2,title3,xaxis_label,yaxis_label,
              "Percentage Error:",DT_percent);
  setup_graph("P (Phase) Error",title2,title3,xaxis_label,yaxis_label,
              "Error (rads/block)",DT_abserror);
  if(fit_W)
    setup_graph("W (Frequency) Error",title2,title3,xaxis_label,yaxis_label,
                "Error (cycles/block)",DT_abserror);
  if(fit_dA)
    setup_graph("dA (Amplitude Modulation) Error",title2,title3,xaxis_label,yaxis_label,
                "Percentage Error:",DT_percent);
  if(fit_dW)
    setup_graph("dW (Chirp Rate) Error",title2,title3,xaxis_label,yaxis_label,
                "Error (cycles/block)",DT_abserror);
  if(fit_ddA)
    setup_graph("ddA (Amplitude Modulation Squared) Error",title2,title3,xaxis_label,yaxis_label,
                "Percentage Error:",DT_percent);

  /* Make cairo drawables */
  converge=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
  if(!converge || cairo_surface_status(converge)!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not set up Cairo surface.\n\n");
    exit(1);
  }
  cC=cairo_create(converge);
  draw_page(cC,"Convergence",title2,title3,xaxis_label,yaxis_label,
            "Iterations:",DT_iterations);

  Aerror=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
  if(!Aerror || cairo_surface_status(Aerror)!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not set up Cairo surface.\n\n");
    exit(1);
  }
  cA=cairo_create(Aerror);
  draw_page(cA,"A (Amplitude) Error",title2,title3,xaxis_label,yaxis_label,
            "Percentage Error:",DT_percent);

  Perror=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
  if(!Perror || cairo_surface_status(Perror)!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"Could not set up Cairo surface.\n\n");
    exit(1);
  }
  cP=cairo_create(Perror);
  draw_page(cP,"P (Phase) Error",title2,title3,xaxis_label,yaxis_label,
            "Error (rads/block)",DT_abserror);

  if(fit_W){
    Werror=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
    if(!Werror || cairo_surface_status(Werror)!=CAIRO_STATUS_SUCCESS){
      fprintf(stderr,"Could not set up Cairo surface.\n\n");
      exit(1);
    }
    cW=cairo_create(Werror);
    draw_page(cW,"W (Frequency) Error",title2,title3,xaxis_label,yaxis_label,
              "Error (cycles/block)",DT_abserror);
  }

  if(fit_dA){
    dAerror=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
    if(!dAerror || cairo_surface_status(dAerror)!=CAIRO_STATUS_SUCCESS){
      fprintf(stderr,"Could not set up Cairo surface.\n\n");
      exit(1);
    }
    cdA=cairo_create(dAerror);
    draw_page(cdA,"dA (Amplitude Modulation) Error",
              title2,title3,xaxis_label,yaxis_label,
              "Percentage Error:",DT_percent);
  }

  if(fit_dW){
    dWerror=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
    if(!dWerror || cairo_surface_status(dWerror)!=CAIRO_STATUS_SUCCESS){
      fprintf(stderr,"Could not set up Cairo surface.\n\n");
      exit(1);
    }
    cdW=cairo_create(dWerror);
    draw_page(cdW,"dW (Chirp Rate) Error",title2,title3,xaxis_label,yaxis_label,
              "Error (cycles/block)",DT_abserror);
  }

  if(fit_ddA){
    ddAerror=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,pic_w, pic_h);
    if(!ddAerror || cairo_surface_status(ddAerror)!=CAIRO_STATUS_SUCCESS){
      fprintf(stderr,"Could not set up Cairo surface.\n\n");
      exit(1);
    }
    cddA=cairo_create(ddAerror);
    draw_page(cddA,"ddA (Amplitude Modulation Squared) Error",
              title2,title3,xaxis_label,yaxis_label,
              "Percentage Error:",DT_percent);
  }

  for(i=0;i<BLOCKSIZE;i++)
    window[i]=1.;
  if(fit_hanning)
    hanning(window,BLOCKSIZE);

  /* graph computation */
  for(x=0;x<x_n;x++){
    float w = (float)x/xstepd;
    chirp ch[y_n*2+1];
    int ret[y_n*2+1];

    float rand_P_ch = 2*M_PI*(float)drand48()-M_PI;

    for(i=0;i<BLOCKSIZE;i++){
      float jj = i-BLOCKSIZE*.5+.5;
      in[i]=initial_chirp_A * cos(w*jj*2.*M_PI/BLOCKSIZE+initial_chirp_P);
    }

    fprintf(stderr,"\rW estimate distance vs. W graphs: %d%%...",100*x/x_n);

    for(y=y_n;y>=-y_n;y--){
      float rand_A_est = (float)drand48();
      float rand_P_est = 2*M_PI*(float)drand48()-M_PI;
      int yi=y_n-y;
      float we=(float)y/ystepd+w;
      ch[yi]=(chirp){0,
                     (we)*2.*M_PI/BLOCKSIZE,
                     0,
                     initial_est_dA,
                     initial_est_dW,
                     initial_est_ddA,
                     yi};
    }
    next_y=0;
    for(y=0;y<cores;y++){
      arg[y].in = in;
      arg[y].window=window;
      arg[y].c=ch;
      arg[y].ret=ret;
      arg[y].max_y=y_n*2+1;
      pthread_create(threads+y,NULL,w_e_column,arg+y);
    }
    for(y=0;y<cores;y++)
      pthread_join(threads[y],NULL);

    for(y=-y_n;y<=y_n;y++){
      int yi=y+y_n;
      float a = (x%xstepg==0 || y%ystepg==0 ?
                 (x%xstepd==0 || y%ystepd==0 ? .3 : .8) : 1.);

      set_iter_color(cC,ret[yi],a);
      cairo_rectangle(cC,x+leftpad,yi+toppad,1,1);
      cairo_fill(cC);

      set_error_color(cA,fabs(initial_chirp_A-ch[yi].A),a);
      cairo_rectangle(cA,x+leftpad,yi+toppad,1,1);
      cairo_fill(cA);

      set_error_color(cP,fabs(initial_chirp_P-ch[yi].P),a);
      cairo_rectangle(cP,x+leftpad,yi+toppad,1,1);
      cairo_fill(cP);

      if(cW){
        set_error_color(cW,fabs(w-(ch[yi].W/2./M_PI*BLOCKSIZE)),a);
        cairo_rectangle(cW,x+leftpad,yi+toppad,1,1);
        cairo_fill(cW);
      }

      if(cdA){
        set_error_color(cdA,ch[yi].dA*BLOCKSIZE,a);
        cairo_rectangle(cdA,x+leftpad,yi+toppad,1,1);
        cairo_fill(cdA);
      }

      if(cdW){
        set_error_color(cdW,ch[yi].dW/M_PI*BLOCKSIZE*BLOCKSIZE,a);
        cairo_rectangle(cdW,x+leftpad,yi+toppad,1,1);
        cairo_fill(cdW);
      }

      if(cddA){
        set_error_color(cddA,ch[yi].ddA/BLOCKSIZE*BLOCKSIZE,a);
        cairo_rectangle(cddA,x+leftpad,yi+toppad,1,1);
        cairo_fill(cddA);
      }
    }

    if((x&15)==0){
      to_png(converge,filebase,"converge");
      to_png(Aerror,filebase,"Aerror");
      to_png(Perror,filebase,"Perror");
      to_png(Werror,filebase,"Werror");
      to_png(dAerror,filebase,"dAerror");
      to_png(dWerror,filebase,"dWerror");
      to_png(ddAerror,filebase,"ddAerror");
    }

  }

  to_png(converge,filebase,"converge");
  to_png(Aerror,filebase,"Aerror");
  to_png(Perror,filebase,"Perror");
  to_png(Werror,filebase,"Werror");
  to_png(dAerror,filebase,"dAerror");
  to_png(dWerror,filebase,"dWerror");
  to_png(ddAerror,filebase,"ddAerror");

}

int main(){
  w_e();
  return 0;
}

