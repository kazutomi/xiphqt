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
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo.h>
#include <pthread.h>
#include <sys/time.h>

float circular_distance(float A,float B){
  float ret = A-B;
  while(ret<-M_PI)ret+=2*M_PI;
  while(ret>=M_PI)ret-=2*M_PI;
  return ret;
}

/* ranges are inclusive */
void set_chirp(chirp *c,int rand_p, int i, int n,
               float A0, float A1,
               float P0, float P1,
               float W0, float W1,
               float dA0, float dA1,
               float dW0, float dW1,
               float ddA0, float ddA1){
  if(n<=1){
    c->A=A0;
    c->P=P0;
    c->W=W0;
    c->dA=dA0;
    c->dW=dW0;
    c->ddA=ddA0;
  }else if(rand_p){
    c->A = A0 + (A1-A0)*drand48();
    c->P = P0 + (P1-P0)*drand48();
    c->W = W0 + (W1-W0)*drand48();
    c->dA = dA0 + (dA1-dA0)*drand48();
    c->dW = dW0 + (dW1-dW0)*drand48();
    c->ddA = ddA0 + (ddA1-ddA0)*drand48();
  }else{
    c->A = A0 + (A1-A0)/(n-1)*i;
    c->P = P0 + (P1-P0)/(n-1)*i;
    c->W = W0 + (W1-W0)/(n-1)*i;
    c->dA = dA0 + (dA1-dA0)/(n-1)*i;
    c->dW = dW0 + (dW1-dW0)/(n-1)*i;
    c->ddA = ddA0 + (ddA1-ddA0)/(n-1)*i;
  }
}

/*********************** Plot w estimate error against w **********************/

typedef struct {
  float *in;
  float *window;
  int blocksize;
  int max_iterations;
  float fit_tolerance;

  int fit_gauss_seidel;
  int fit_W;
  int fit_dA;
  int fit_dW;
  int fit_ddA;
  int fit_nonlinear; /* 0==linear, 1==W-recentered, 2==W,dW-recentered */
  float fit_W_alpha;
  float fit_dW_alpha;
  int fit_symm_norm;
  int fit_bound_zero;

  chirp *chirp;
  chirp *estimate;
  float *rms_error;
  int *iterations;

} colarg;


pthread_mutex_t ymutex = PTHREAD_MUTEX_INITIALIZER;
int next_y=0;
int max_y=0;

#define _GNU_SOURCE
#include <fenv.h>

void *compute_column(void *in){
  colarg *arg = (colarg *)in;
  int blocksize=arg->blocksize;
  float rec[blocksize];
  int y,i,ret;
  int except;
  int localinit = !arg->in;
  float *chirp = localinit ? malloc(sizeof(*chirp)*blocksize) : arg->in;

  while(1){
    float s=0.;

    pthread_mutex_lock(&ymutex);
    y=next_y;
    if(y>=max_y){
      pthread_mutex_unlock(&ymutex);
      return NULL;
    }
    next_y++;
    pthread_mutex_unlock(&ymutex);

    /* if the input is uninitialized, it's because we're sweeping or
       randomizing chirp components across the column; generate the
       input here in the thread */
    if(localinit){
      for(i=0;i<blocksize;i++){
        double jj = i - blocksize/2 + .5;
        double A = arg->chirp[y].A + arg->chirp[y].dA*jj + arg->chirp[y].ddA*jj*jj;
        double P = arg->chirp[y].P + arg->chirp[y].W *jj + arg->chirp[y].dW *jj*jj;
        chirp[i] = A*cos(P);
      }
    }

    except=feenableexcept(FE_ALL_EXCEPT);
    fedisableexcept(FE_INEXACT);
    fedisableexcept(FE_UNDERFLOW);

    ret=estimate_chirps(chirp,rec,arg->window,blocksize,
                        arg->estimate+y,1,arg->fit_tolerance,arg->max_iterations,
                        arg->fit_gauss_seidel,
                        arg->fit_W,
                        arg->fit_dA,
                        arg->fit_dW,
                        arg->fit_ddA,
                        arg->fit_nonlinear,
                        arg->fit_W_alpha,
                        arg->fit_dW_alpha,
                        arg->fit_symm_norm,
                        arg->fit_bound_zero);

    for(i=0;i<blocksize;i++){
      float v = (chirp[i]-rec[i])*arg->window[i];
      s += v*v;
    }
    arg->rms_error[y] = sqrt(s/blocksize);
    arg->iterations[y] = arg->max_iterations-ret;
    feclearexcept(FE_ALL_EXCEPT);
    feenableexcept(except);

  }

  if(localinit)free(chirp);
  return NULL;
}

typedef struct {
  float fontsize;
  char *subtitle1;
  char *subtitle2;
  char *subtitle3;

  int blocksize;
  int threads;

  int x0;
  int x1;
  int xmajor;
  int xminor;
  int y0;
  int y1;
  int ymajor;
  int yminor;

  void (*window)(float *,int n);
  float fit_tolerance;

  int fit_gauss_seidel;
  int fit_W;
  int fit_dA;
  int fit_dW;
  int fit_ddA;
  int fit_nonlinear;
  float fit_W_alpha;
  float fit_dW_alpha;
  int fit_symm_norm;
  int fit_bound_zero;

  /* If the randomize flag is unset and min!=max, a param is swept
     from min to max (inclusive) divided into <sweep_steps>
     increments.  If the rand flag is set, <sweep_steps> random values
     in the range min to max instead. */
  int sweep_steps;
  int sweep_or_rand_p;

  float min_est_A;
  float max_est_A;

  float min_est_P;
  float max_est_P;

  float min_est_W;
  float max_est_W;

  float min_est_dA;
  float max_est_dA;

  float min_est_dW;
  float max_est_dW;

  float min_est_ddA;
  float max_est_ddA;

  float min_chirp_A;
  float max_chirp_A;

  float min_chirp_P;
  float max_chirp_P;

  float min_chirp_W;
  float max_chirp_W;

  float min_chirp_dA;
  float max_chirp_dA;

  float min_chirp_dW;
  float max_chirp_dW;

  float min_chirp_ddA;
  float max_chirp_ddA;

}  graph_run;

/* performs a W initial estimate error vs chirp W plot.  Ignores the
   est and chirp arguments for W; these are pulled from the x and y setup */

void w_e(char *filebase,graph_run *arg){
  int threads=arg->threads;
  int blocksize = arg->blocksize;
  float window[blocksize];
  float in[blocksize];
  int i,xi,yi;

  int x_n = arg->x1-arg->x0+1;
  int y_n = arg->y1-arg->y0+1;

  /* graphs:

     convergence
     Aerror
     Perror
     Werror
     dAerror
     dWerror
     ddAerror
     rms fit error

     generate for:
     worst case across sweep (0-7)
     delta across sweep (8-15)
  */

  cairo_t *cC=NULL;
  cairo_t *cA=NULL;
  cairo_t *cP=NULL;
  cairo_t *cW=NULL;
  cairo_t *cdA=NULL;
  cairo_t *cdW=NULL;
  cairo_t *cddA=NULL;
  cairo_t *cRMS=NULL;

  cairo_t *cC_d=NULL;
  cairo_t *cA_d=NULL;
  cairo_t *cP_d=NULL;
  cairo_t *cW_d=NULL;
  cairo_t *cdA_d=NULL;
  cairo_t *cdW_d=NULL;
  cairo_t *cddA_d=NULL;
  cairo_t *cRMS_d=NULL;

  pthread_t threadlist[threads];
  colarg targ[threads];

  char *yaxis_label = "initial distance from W (cycles/block)";
  char *xaxis_label = "W (cycles/block)";
  int swept=0;
  int chirp_swept=0;
  int est_swept=0;

  float ret_minA[y_n];
  float ret_minP[y_n];
  float ret_minW[y_n];
  float ret_mindA[y_n];
  float ret_mindW[y_n];
  float ret_minddA[y_n];
  float ret_minRMS[y_n];
  float ret_miniter[y_n];

  float ret_maxA[y_n];
  float ret_maxP[y_n];
  float ret_maxW[y_n];
  float ret_maxdA[y_n];
  float ret_maxdW[y_n];
  float ret_maxddA[y_n];
  float ret_maxRMS[y_n];
  float ret_maxiter[y_n];

  struct timeval last;
  gettimeofday(&last,NULL);

  /* determine ~ padding needed */
  setup_graphs(arg->x0,arg->x1,arg->xmajor,
               arg->y0,arg->y1,arg->ymajor,
               (arg->subtitle1!=0)+(arg->subtitle2!=0)+(arg->subtitle3!=0),
               arg->fontsize);

  /* don't check W; it's always swept in this graph */
  if(arg->min_est_A != arg->max_est_A ||
     arg->min_est_P != arg->max_est_P ||
     arg->min_est_dA != arg->max_est_dA ||
     arg->min_est_dW != arg->max_est_dW ||
     arg->min_est_ddA != arg->max_est_ddA) est_swept=1;

  if(arg->min_chirp_A != arg->max_chirp_A ||
     arg->min_chirp_P != arg->max_chirp_P ||
     arg->min_chirp_dA != arg->max_chirp_dA ||
     arg->min_chirp_dW != arg->max_chirp_dW ||
     arg->min_chirp_ddA != arg->max_chirp_ddA) chirp_swept=1;
  swept = est_swept | chirp_swept;

  cC = draw_page(!swept?"Convergence":"Worst Case Convergence",
                 arg->subtitle1,
                 arg->subtitle2,
                 arg->subtitle3,
                 xaxis_label,
                 yaxis_label,
                 "Iterations:",
                 DT_iterations);
  if(swept)
    cC_d = draw_page("Convergence Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     xaxis_label,
                     yaxis_label,
                     "Iteration span:",
                     DT_iterations);


  cA = draw_page(!swept?"A (Amplitude) Error":"Maximum A (Amplitude) Error",
                 arg->subtitle1,
                 arg->subtitle2,
                 arg->subtitle3,
                 xaxis_label,
                 yaxis_label,
                 "Percentage Error:",
                 DT_percent);
  if(swept)
    cA_d = draw_page("A (Amplitude) Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     xaxis_label,
                     yaxis_label,
                     "Delta:",
                     DT_abserror);


  cP = draw_page(!swept?"P (Phase) Error":"Maximum P (Phase) Error",
                 arg->subtitle1,
                 arg->subtitle2,
                 arg->subtitle3,
                 xaxis_label,
                 yaxis_label,
                 "Error (rads/block):",
                 DT_percent);
  if(swept)
    cP_d = draw_page("Phase Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     xaxis_label,
                     yaxis_label,
                     "Delta (rads/block):",
                     DT_abserror);

  if(arg->fit_W){
    cW = draw_page(!swept?"W (Frequency) Error":"Maximum W (Frequency) Error",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   xaxis_label,
                   yaxis_label,
                   "Error (rads/block):",
                   DT_abserror);
    if(swept)
      cW_d = draw_page("Frequency Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       xaxis_label,
                       yaxis_label,
                       "Delta (rads/block):",
                       DT_abserror);
  }

  if(arg->fit_dA){
    cdA = draw_page(!swept?"dA (Amplitude Modulation) Error":"Maximum dA (Amplitude Modulation) Error",
                    arg->subtitle1,
                    arg->subtitle2,
                    arg->subtitle3,
                    xaxis_label,
                    yaxis_label,
                    "Error:",
                    DT_abserror);
    if(swept)
      cdA_d = draw_page("Amplitude Modulation Delta",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        xaxis_label,
                        yaxis_label,
                        "Delta:",
                        DT_abserror);
  }

  if(arg->fit_dW){
    cdW = draw_page(!swept?"dW (Chirp Rate) Error":"Maximum dW (Chirp Rate) Error",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   xaxis_label,
                   yaxis_label,
                   "Error (rads/block):",
                   DT_abserror);
    if(swept)
      cdW_d = draw_page("Chirp Rate Delta",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        xaxis_label,
                        yaxis_label,
                        "Delta (rads/block):",
                        DT_abserror);
  }

  if(arg->fit_ddA){
    cddA = draw_page(!swept?"ddA (Amplitude Modulation Squared) Error":
                     "Maximum ddA (Amplitude Modulation Squared) Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     xaxis_label,
                     yaxis_label,
                     "Error:",
                     DT_abserror);
    if(swept)
      cddA_d = draw_page("Amplitude Modulation Squared Delta",
                         arg->subtitle1,
                         arg->subtitle2,
                         arg->subtitle3,
                         xaxis_label,
                         yaxis_label,
                         "Delta:",
                         DT_abserror);
  }

  cRMS = draw_page(swept?"Maximum RMS Fit Error":
                   "RMS Fit Error",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   xaxis_label,
                   yaxis_label,
                   "Percentage Error:",
                   DT_percent);
  if(swept)
    cRMS_d = draw_page("RMS Error Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       xaxis_label,
                       yaxis_label,
                       "Percentage Delta:",
                       DT_percent);

  if(arg->window)
    arg->window(window,blocksize);
  else
    for(i=0;i<blocksize;i++)
      window[i]=1.;

  /* graph computation */
  for(xi=0;xi<x_n;xi++){
    int x = xi+arg->x0;
    float w = ((float)x/arg->xmajor)/blocksize*2.*M_PI;
    chirp chirps[y_n];
    chirp estimates[y_n];
    int iter[y_n];
    float rms[y_n];
    int si,sn=(swept && arg->sweep_steps>1 ? arg->sweep_steps : 1);

    fprintf(stderr,"\rW estimate distance vs. W graphs: column %d/%d...",x-arg->x0,x_n-1);

    memset(targ,0,sizeof(targ));

    for(i=0;i<threads;i++){
      if(!chirp_swept)targ[i].in=in;
      targ[i].window=window;
      targ[i].blocksize=blocksize;
      targ[i].max_iterations=100;
      targ[i].fit_tolerance=arg->fit_tolerance;
      targ[i].fit_gauss_seidel=arg->fit_gauss_seidel;
      targ[i].fit_W=arg->fit_W;
      targ[i].fit_dA=arg->fit_dA;
      targ[i].fit_dW=arg->fit_dW;
      targ[i].fit_ddA=arg->fit_ddA;
      targ[i].fit_nonlinear=arg->fit_nonlinear;
      targ[i].fit_W_alpha=arg->fit_W_alpha;
      targ[i].fit_dW_alpha=arg->fit_dW_alpha;
      targ[i].fit_symm_norm=arg->fit_symm_norm;
      targ[i].fit_bound_zero=arg->fit_bound_zero;
      targ[i].chirp=chirps;
      //targ[i].in=NULL;
      targ[i].estimate=estimates;
      targ[i].rms_error=rms;
      targ[i].iterations=iter;
    }

    /* if we're sweeping a parameter, we're going to iterate here for a bit. */
    for(si=0;si<sn;si++){
      max_y=y_n;
      next_y=0;

      /* compute/set chirp and est parameters, potentially compute the
         chirp waveform */
      for(i=0;i<y_n;i++){
        int y = arg->y1-i;
        float we=((float)y/arg->ymajor)/blocksize*2.*M_PI+w;

        set_chirp(chirps+i,arg->sweep_or_rand_p,si,sn,
                  arg->min_chirp_A,arg->max_chirp_A,
                  arg->min_chirp_P,arg->max_chirp_P,
                  w,w,
                  arg->min_chirp_dA,arg->min_chirp_dA,
                  arg->min_chirp_dW,arg->min_chirp_dW,
                  arg->min_chirp_ddA,arg->max_chirp_ddA);
        set_chirp(estimates+i,arg->sweep_or_rand_p,si,sn,
                  arg->min_est_A,arg->max_est_A,
                  arg->min_est_P,arg->max_est_P,
                  we,we,
                  arg->min_est_dA,arg->min_est_dA,
                  arg->min_est_dW,arg->min_est_dW,
                  arg->min_est_ddA,arg->max_est_ddA);
      }

      if(!chirp_swept){
        for(i=0;i<threads;i++)
          targ[i].in=in;

        for(i=0;i<blocksize;i++){
          double jj = i-blocksize/2+.5;
          double A = chirps[0].A + chirps[0].dA*jj + chirps[0].ddA*jj*jj;
          double P = chirps[0].P + chirps[0].W*jj  + chirps[0].dW *jj*jj;
          in[i] = A*cos(P);
        }
      }

      /* compute column */
      for(i=0;i<threads;i++)
        pthread_create(threadlist+i,NULL,compute_column,targ+i);
      for(i=0;i<threads;i++)
        pthread_join(threadlist[i],NULL);

      /* accumulate results for this pass */
      if(si==0){
        for(i=0;i<y_n;i++){
          ret_minA[i]=ret_maxA[i]=chirps[i].A - estimates[i].A;
          ret_minP[i]=ret_maxP[i]=circular_distance(chirps[i].P,estimates[i].P);
          ret_minW[i]=ret_maxW[i]=chirps[i].W - estimates[i].W;
          ret_mindA[i]=ret_maxdA[i]=chirps[i].dA - estimates[i].dA;
          ret_mindW[i]=ret_maxdW[i]=chirps[i].dW - estimates[i].dW;
          ret_minddA[i]=ret_maxddA[i]=chirps[i].ddA - estimates[i].ddA;
          ret_minRMS[i]=ret_maxRMS[i]=rms[i];
          ret_miniter[i]=ret_maxiter[i]=iter[i];
        }
      }else{
        for(i=0;i<y_n;i++){
          float v = chirps[i].A - estimates[i].A;
          if(ret_minA[i]>v)ret_minA[i]=v;
          if(ret_maxA[i]<v)ret_maxA[i]=v;

          v = circular_distance(chirps[i].P, estimates[i].P);
          if(ret_minP[i]>v)ret_minP[i]=v;
          if(ret_maxP[i]<v)ret_maxP[i]=v;

          v = chirps[i].W - estimates[i].W;
          if(ret_minW[i]>v)ret_minW[i]=v;
          if(ret_maxW[i]<v)ret_maxW[i]=v;

          v = chirps[i].dA - estimates[i].dA;
          if(ret_mindA[i]>v)ret_mindA[i]=v;
          if(ret_maxdA[i]<v)ret_maxdA[i]=v;

          v = chirps[i].dW - estimates[i].dW;
          if(ret_mindW[i]>v)ret_mindW[i]=v;
          if(ret_maxdW[i]<v)ret_maxdW[i]=v;

          v = chirps[i].ddA - estimates[i].ddA;
          if(ret_minddA[i]>v)ret_minddA[i]=v;
          if(ret_maxddA[i]<v)ret_maxddA[i]=v;

          if(ret_minRMS[i]>rms[i])ret_minRMS[i]=rms[i];
          if(ret_maxRMS[i]<rms[i])ret_maxRMS[i]=rms[i];

          if(ret_miniter[i]>iter[i])ret_miniter[i]=iter[i];
          if(ret_maxiter[i]<iter[i])ret_maxiter[i]=iter[i];
        }
      }
    }

    /* Column sweep complete; plot */
    for(yi=0;yi<y_n;yi++){
      int y=arg->y1-yi;
      float a = 1.;
      if(x%arg->xminor==0 || y%arg->yminor==0) a = .8;
      if(x%arg->xmajor==0 || y%arg->ymajor==0) a = .3;

      /* Convergence graph */
      set_iter_color(cC,ret_maxiter[yi],a);
      cairo_rectangle(cC,xi+leftpad,yi+toppad,1,1);
      cairo_fill(cC);

      /* Convergence delta graph */
      if(swept){
        set_iter_color(cC_d,ret_maxiter[yi]-ret_miniter[yi],a);
        cairo_rectangle(cC_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cC_d);
      }

      /* A error graph */
      set_error_color(cA,MAX(fabs(ret_maxA[yi]),fabs(ret_minA[yi])),a);
      cairo_rectangle(cA,xi+leftpad,yi+toppad,1,1);
      cairo_fill(cA);

      /* A delta graph */
      if(swept){
        set_error_color(cA_d,ret_maxA[yi]-ret_minA[yi],a);
        cairo_rectangle(cA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cA_d);
      }

      /* P error graph */
      set_error_color(cP,MAX(fabs(ret_maxP[yi]),fabs(ret_minP[yi])),a);
      cairo_rectangle(cP,xi+leftpad,yi+toppad,1,1);
      cairo_fill(cP);

      /* P delta graph */
      if(swept){
        set_error_color(cP_d,circular_distance(ret_maxP[yi],ret_minP[yi]),a);
        cairo_rectangle(cP_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cP_d);
      }

      if(cW){
        /* W error graph */
        set_error_color(cW,MAX(fabs(ret_maxW[yi]),fabs(ret_minW[yi]))/2./M_PI*blocksize,a);
        cairo_rectangle(cW,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cW);

        /* W delta graph */
        if(swept){
          set_error_color(cW_d,(ret_maxW[yi]-ret_minW[yi])/2./M_PI*blocksize,a);
          cairo_rectangle(cW_d,xi+leftpad,yi+toppad,1,1);
          cairo_fill(cW_d);
        }
      }

      if(cdA){
        /* dA error graph */
        set_error_color(cdA,MAX(fabs(ret_maxdA[yi]),fabs(ret_mindA[yi]))*blocksize,a);
        cairo_rectangle(cdA,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdA);

        /* dA delta graph */
        if(swept){
          set_error_color(cdA_d,(ret_maxdA[yi]-ret_mindA[yi])*blocksize,a);
          cairo_rectangle(cdA_d,xi+leftpad,yi+toppad,1,1);
          cairo_fill(cdA_d);
        }
      }

      if(cdW){
        /* dW error graph */
        set_error_color(cdW,MAX(fabs(ret_maxdW[yi]),fabs(ret_mindW[yi]))/M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdW);

        /* dW delta graph */
        if(swept){
          set_error_color(cdW_d,(ret_maxdW[yi]-ret_mindW[yi])/M_PI*blocksize*blocksize,a);
          cairo_rectangle(cdW_d,xi+leftpad,yi+toppad,1,1);
          cairo_fill(cdW_d);
        }
      }

      if(cddA){
        /* ddA error graph */
        set_error_color(cddA,MAX(fabs(ret_maxddA[yi]),fabs(ret_minddA[yi]))*blocksize*blocksize,a);
        cairo_rectangle(cddA,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cddA);

        /* dA delta graph */
        if(swept){
          set_error_color(cddA_d,(ret_maxddA[yi]-ret_minddA[yi])*blocksize*blocksize,a);
          cairo_rectangle(cddA_d,xi+leftpad,yi+toppad,1,1);
          cairo_fill(cddA_d);
        }
      }

      /* RMS error graph */
      set_error_color(cRMS,ret_maxRMS[yi],a);
      cairo_rectangle(cRMS,xi+leftpad,yi+toppad,1,1);
      cairo_fill(cRMS);

      /* RMS delta graph */
      if(swept){
        set_error_color(cRMS_d,ret_maxRMS[yi]-ret_minRMS[yi],a);
        cairo_rectangle(cRMS_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cRMS_d);
      }
    }

    /* dump a graph update every 10 seconds */
    {
      struct timeval now;
      gettimeofday(&now,NULL);
      if(now.tv_sec-last.tv_sec + (now.tv_usec-last.tv_usec)/1000000. > 10.){
        last=now;

        to_png(cC,filebase,"converge");
        to_png(cA,filebase,"Aerror");
        to_png(cP,filebase,"Perror");
        to_png(cW,filebase,"Werror");
        to_png(cdA,filebase,"dAerror");
        to_png(cdW,filebase,"dWerror");
        to_png(cddA,filebase,"ddAerror");
        to_png(cRMS,filebase,"RMSerror");

        to_png(cC_d,filebase,"convdelta");
        to_png(cA_d,filebase,"Adelta");
        to_png(cP_d,filebase,"Pdelta");
        to_png(cW_d,filebase,"Wdelta");
        to_png(cdA_d,filebase,"dAdelta");
        to_png(cdW_d,filebase,"dWdelta");
        to_png(cddA_d,filebase,"ddAdelta");
        to_png(cRMS_d,filebase,"RMSdelta");
      }
    }
  }

  to_png(cC,filebase,"converge");
  to_png(cA,filebase,"Aerror");
  to_png(cP,filebase,"Perror");
  to_png(cW,filebase,"Werror");
  to_png(cdA,filebase,"dAerror");
  to_png(cdW,filebase,"dWerror");
  to_png(cddA,filebase,"ddAerror");
  to_png(cRMS,filebase,"RMSerror");

  to_png(cC_d,filebase,"convdelta");
  to_png(cA_d,filebase,"Adelta");
  to_png(cP_d,filebase,"Pdelta");
  to_png(cW_d,filebase,"Wdelta");
  to_png(cdA_d,filebase,"dAdelta");
  to_png(cdW_d,filebase,"dWdelta");
  to_png(cddA_d,filebase,"ddAdelta");
  to_png(cRMS_d,filebase,"RMSdelta");

  fprintf(stderr," done\n");
}

int main(){
  graph_run arg={
    /* fontsize */      12,
    /* subtitle1 */     "graphtest1",
    /* subtitle2 */     "graphtest2",
    /* subtitle3 */     "graphtest3",
    /* blocksize */     256,
    /* threads */       8,

    /* x0 */            0,
    /* x1 */            1280,
    /* xmajor */        10,
    /* xminor */        5,

    /* y0 */            -300,
    /* y1 */            300,
    /* ymajor */        30,
    /* yminor */        15,

    /* window */        window_functions.maxwell1,
    /* fit_tol */       .000001,
    /* gauss_seidel */  1,
    /* fit_W */         1,
    /* fit_dA */        1,
    /* fit_dW */        1,
    /* fit_ddA */       0,
    /* nonlinear */     2,
    /* W_alpha */       1.,
    /* dW_alpha */      1.75,
    /* symm_norm */     1,
    /* bound_zero */    0,

    /* sweep_steps */   8,
    /* randomize_p */   0,

    /* est A range */   0.,0.,
    /* est P range */   0.,0.,
    /* est W range */   0.,0.,
    /* est dA range */  0.,0.,
    /* est dW range */  0.,0.,
    /* est ddA range */ 0.,0.,

    /* ch A range */    1.,1.,
    /* ch P range */    0,2*M_PI,
    /* ch W range */    0.,0.,
    /* ch dA range */   0.,0.,
    /* ch dW range */   0.,0.,
    /* ch ddA range */  0.,0.,
  };

  w_e("graphs-A",&arg);
  return 0;
}

