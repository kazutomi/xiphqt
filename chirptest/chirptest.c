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

typedef struct {
  float fontsize;
  char *subtitle1;
  char *subtitle2;
  char *subtitle3;
  char *xaxis_label;
  char *yaxis_label;

  int blocksize;
  int threads;

  void (*window)(float *,int n);
  float fit_tolerance;

  int fit_gauss_seidel;
  int fit_W;
  int fit_dA;
  int fit_dW;
  int fit_ddA;
  int fit_nonlinear;
  int fit_symm_norm;
  int fit_bound_zero;

  float fit_W_alpha_min;
  float fit_W_alpha_max;
  float fit_dW_alpha_min;
  float fit_dW_alpha_max;

  int x_dim;
  int x_steps;
  float x_major;
  float x_minor;
  int y_dim;
  int y_steps;
  float y_major;
  float y_minor;
  int sweep_steps;

  /* If the randomize flag is unset and min!=max, a param is swept
     from min to max (inclusive) divided into <sweep_steps>
     increments.  If the rand flag is set, <sweep_steps> random values
     in the range min to max instead. */
  int sweep_or_rand_p;

  float min_est_A;
  float max_est_A;
  int rel_est_A;

  float min_est_P;
  float max_est_P;
  int rel_est_P;

  float min_est_W;
  float max_est_W;
  int rel_est_W;

  float min_est_dA;
  float max_est_dA;
  int rel_est_dA;

  float min_est_dW;
  float max_est_dW;
  int rel_est_dW;

  float min_est_ddA;
  float max_est_ddA;
  int rel_est_ddA;

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

  /* generate which graphs? */
  int graph_convergence_max;
  int graph_convergence_delta;

  int graph_Aerror_max;
  int graph_Aerror_delta;

  int graph_Perror_max;
  int graph_Perror_delta;

  int graph_Werror_max;
  int graph_Werror_delta;

  int graph_dAerror_max;
  int graph_dAerror_delta;

  int graph_dWerror_max;
  int graph_dWerror_delta;

  int graph_ddAerror_max;
  int graph_ddAerror_delta;

  int graph_RMSerror_max;
  int graph_RMSerror_delta;

}  graph_run;

float circular_distance(float A,float B){
  float ret = A-B;
  while(ret<-M_PI)ret+=2*M_PI;
  while(ret>=M_PI)ret-=2*M_PI;
  return ret;
}

typedef struct {
  float fit_W_alpha;
  float fit_dW_alpha;
} colvec;

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
  int fit_symm_norm;
  int fit_bound_zero;

  chirp *chirp;
  chirp *estimate;
  colvec *sweep;
  float *rms_error;
  int *iterations;

} colarg;

#define DIM_ESTIMATE_A 1
#define DIM_ESTIMATE_P 2
#define DIM_ESTIMATE_W 3
#define DIM_ESTIMATE_dA 4
#define DIM_ESTIMATE_dW 5
#define DIM_ESTIMATE_ddA 6
#define DIM_ESTIMATE_MASK 0xf

#define DIM_CHIRP_A (1<<4)
#define DIM_CHIRP_P (2<<4)
#define DIM_CHIRP_W (3<<4)
#define DIM_CHIRP_dA (4<<4)
#define DIM_CHIRP_dW  (5<<4)
#define DIM_CHIRP_ddA (6<<4)
#define DIM_CHIRP_MASK 0xf0

#define DIM_ALPHA_W (1<<8)
#define DIM_ALPHA_dW (2<<8)

/* ranges are inclusive */
void set_chirp(chirp *c,
               int xi, int xn, int xdim,
               int yi, int yn, int ydim,
               int stepi, int stepn, int rand_p,
               float A0, float A1,
               float P0, float P1,
               float W0, float W1,
               float dA0, float dA1,
               float dW0, float dW1,
               float ddA0, float ddA1){

  float An,Pn,Wn,dAn,dWn,ddAn;
  float Ai,Pi,Wi,dAi,dWi,ddAi;

  xdim = (xdim | (xdim>>4)) & DIM_ESTIMATE_MASK;
  ydim = (ydim | (ydim>>4)) & DIM_ESTIMATE_MASK;
  if(stepn<2)stepn=2;

  An=Pn=Wn=dAn=dWn=ddAn=stepn-1;
  Ai = (rand_p ? drand48()*An : stepi);
  Pi = (rand_p ? drand48()*Pn : stepi);
  Wi = (rand_p ? drand48()*Wn : stepi);
  dAi = (rand_p ? drand48()*dAn : stepi);
  dWi = (rand_p ? drand48()*dWn : stepi);
  ddAi = (rand_p ? drand48()*ddAn : stepi);

  switch(xdim){
  case DIM_ESTIMATE_A:
    An = xn-1;
    Ai = xi;
    break;
  case DIM_ESTIMATE_P:
    Pn = xn-1;
    Pi = xi;
    break;
  case DIM_ESTIMATE_W:
    Wn = xn-1;
    Wi = xi;
    break;
  case DIM_ESTIMATE_dA:
    dAn = xn-1;
    dAi = xi;
    break;
  case DIM_ESTIMATE_dW:
    dWn = xn-1;
    dWi = xi;
    break;
  case DIM_ESTIMATE_ddA:
    ddAn = xn-1;
    ddAi = xi;
    break;
  }

  switch(ydim){
  case DIM_ESTIMATE_A:
    An = yn-1;
    Ai = yi;
    break;
  case DIM_ESTIMATE_P:
    Pn = yn-1;
    Pi = yi;
    break;
  case DIM_ESTIMATE_W:
    Wn = yn-1;
    Wi = yi;
    break;
  case DIM_ESTIMATE_dA:
    dAn = yn-1;
    dAi = yi;
    break;
  case DIM_ESTIMATE_dW:
    dWn = yn-1;
    dWi = yi;
    break;
  case DIM_ESTIMATE_ddA:
    ddAn = yn-1;
    ddAi = yi;
    break;
  }

  c->A = A0 + (A1-A0) / An * Ai;
  c->P = P0 + (P1-P0) / Pn * Pi;
  c->W = W0 + (W1-W0) / Wn * Wi;
  c->dA = dA0 + (dA1-dA0) / dAn * dAi;
  c->dW = dW0 + (dW1-dW0) / dWn * dWi;
  c->ddA = ddA0 + (ddA1-ddA0) / ddAn * ddAi;

}

float W_alpha(graph_run *arg,
              int xi, int xn, int xdim,
              int yi, int yn, int ydim,
              int stepi, int stepn, int rand_p){

  float Ai,An;

  if(stepn<2)stepn=2;
  An=stepn-1;
  Ai = (rand_p ? drand48()*An : stepi);

  if(xdim==DIM_ALPHA_W){
    An = xn-1;
    Ai = xi;
  }

  if(ydim==DIM_ALPHA_W){
    An = yn-1;
    Ai = yi;
  }

  return arg->fit_W_alpha_min +
    (arg->fit_W_alpha_max-arg->fit_W_alpha_min) / An * Ai;
}

float dW_alpha(graph_run *arg,
              int xi, int xn, int xdim,
              int yi, int yn, int ydim,
              int stepi, int stepn, int rand_p){

  float Ai,An;

  if(stepn<2)stepn=2;
  An=stepn-1;
  Ai = (rand_p ? drand48()*An : stepi);

  if(xdim==DIM_ALPHA_dW){
    An = xn-1;
    Ai = xi;
  }

  if(ydim==DIM_ALPHA_dW){
    An = yn-1;
    Ai = yi;
  }

  return arg->fit_dW_alpha_min +
    (arg->fit_dW_alpha_max-arg->fit_dW_alpha_min) / An * Ai;
}

/*********************** Plot single estimate vs. chirp **********************/

pthread_mutex_t ymutex = PTHREAD_MUTEX_INITIALIZER;
int next_y=0;
int max_y=0;

#define _GNU_SOURCE
#include <fenv.h>

void *compute_column(void *in){
  colarg *arg = (colarg *)in;
  int blocksize=arg->blocksize;
  int y,i,ret;
  int except;
  int localinit = !arg->in;
  float *chirp = localinit ? malloc(sizeof(*chirp)*blocksize) : arg->in;

  while(1){
    float rms_acc=0.;
    float e_acc=0.;

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
        double A = arg->chirp[y].A + (arg->chirp[y].dA + arg->chirp[y].ddA*jj)*jj;
        double P = arg->chirp[y].P + (arg->chirp[y].W  + arg->chirp[y].dW *jj)*jj;
        chirp[i] = A*cos(P);
      }
    }

    except=feenableexcept(FE_ALL_EXCEPT);
    fedisableexcept(FE_INEXACT);
    fedisableexcept(FE_UNDERFLOW);

    ret=estimate_chirps(chirp,arg->window,blocksize,
                        arg->estimate+y,1,arg->fit_tolerance,arg->max_iterations,
                        arg->fit_gauss_seidel,
                        arg->fit_W,
                        arg->fit_dA,
                        arg->fit_dW,
                        arg->fit_ddA,
                        arg->fit_nonlinear,
                        arg->sweep[y].fit_W_alpha,
                        arg->sweep[y].fit_dW_alpha,
                        arg->fit_symm_norm,
                        arg->fit_bound_zero);

    for(i=0;i<blocksize;i++){
      double jj = i - blocksize/2 + .5;
      double A = arg->estimate[y].A + (arg->estimate[y].dA + arg->estimate[y].ddA*jj)*jj;
      double P = arg->estimate[y].P + (arg->estimate[y].W  + arg->estimate[y].dW *jj)*jj;
      float  r = A*cos(P);
      float ce = chirp[i]*arg->window[i];
      float ee = (chirp[i]-r)*arg->window[i];

      e_acc += ce*ce;
      rms_acc += ee*ee;
    }
    arg->rms_error[y] = sqrt(rms_acc)/sqrt(e_acc);
    arg->iterations[y] = arg->max_iterations-ret;
    feclearexcept(FE_ALL_EXCEPT);
    feenableexcept(except);

  }

  if(localinit)free(chirp);
  return NULL;
}

/* performs a W initial estimate error vs chirp W plot.  Ignores the
   est and chirp arguments for W; these are pulled from the x and y setup */

void w_e(char *filebase,graph_run *arg){
  int threads=arg->threads;
  int blocksize = arg->blocksize;
  float window[blocksize];
  float in[blocksize];
  int i,xi,yi;

  int x_n = arg->x_steps;
  int y_n = arg->y_steps;

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

  int swept=0;
  int chirp_swept=0;
  int est_swept=0;
  int fit_swept=0;

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

  float minX=0,maxX=0;
  float minY=0,maxY=0;
  int x0s,x1s;
  int y0s,y1s;
  int xmajori,ymajori;
  int xminori,yminori;

  struct timeval last;
  gettimeofday(&last,NULL);

  switch(arg->x_dim){
  case DIM_ESTIMATE_A:
    minX = arg->min_est_A;
    maxX = arg->max_est_A;
    break;
  case DIM_ESTIMATE_P:
    minX = arg->min_est_P;
    maxX = arg->max_est_P;
    break;
  case DIM_ESTIMATE_W:
    minX = arg->min_est_W;
    maxX = arg->max_est_W;
    break;
  case DIM_ESTIMATE_dA:
    minX = arg->min_est_dA;
    maxX = arg->max_est_dA;
    break;
  case DIM_ESTIMATE_dW:
    minX = arg->min_est_dW;
    maxX = arg->max_est_dW;
    break;
  case DIM_ESTIMATE_ddA:
    minX = arg->min_est_ddA;
    maxX = arg->max_est_ddA;
    break;
  case DIM_CHIRP_A:
    minX = arg->min_chirp_A;
    maxX = arg->max_chirp_A;
    break;
  case DIM_CHIRP_P:
    minX = arg->min_chirp_P;
    maxX = arg->max_chirp_P;
    break;
  case DIM_CHIRP_W:
    minX = arg->min_chirp_W;
    maxX = arg->max_chirp_W;
    break;
  case DIM_CHIRP_dA:
    minX = arg->min_chirp_dA;
    maxX = arg->max_chirp_dA;
    break;
  case DIM_CHIRP_dW:
    minX = arg->min_chirp_dW;
    maxX = arg->max_chirp_dW;
    break;
  case DIM_CHIRP_ddA:
    minX = arg->min_chirp_ddA;
    maxX = arg->max_chirp_ddA;
    break;
  case DIM_ALPHA_W:
    minX = arg->fit_W_alpha_min;
    maxX = arg->fit_W_alpha_max;
    break;
  case DIM_ALPHA_dW:
    minX = arg->fit_dW_alpha_min;
    maxX = arg->fit_dW_alpha_max;
    break;
  }

  switch(arg->y_dim){
  case DIM_ESTIMATE_A:
    minY = arg->min_est_A;
    maxY = arg->max_est_A;
    break;
  case DIM_ESTIMATE_P:
    minY = arg->min_est_P;
    maxY = arg->max_est_P;
    break;
  case DIM_ESTIMATE_W:
    minY = arg->min_est_W;
    maxY = arg->max_est_W;
    break;
  case DIM_ESTIMATE_dA:
    minY = arg->min_est_dA;
    maxY = arg->max_est_dA;
    break;
  case DIM_ESTIMATE_dW:
    minY = arg->min_est_dW;
    maxY = arg->max_est_dW;
    break;
  case DIM_ESTIMATE_ddA:
    minY = arg->min_est_ddA;
    maxY = arg->max_est_ddA;
    break;
  case DIM_CHIRP_A:
    minY = arg->min_chirp_A;
    maxY = arg->max_chirp_A;
    break;
  case DIM_CHIRP_P:
    minY = arg->min_chirp_P;
    maxY = arg->max_chirp_P;
    break;
  case DIM_CHIRP_W:
    minY = arg->min_chirp_W;
    maxY = arg->max_chirp_W;
    break;
  case DIM_CHIRP_dA:
    minY = arg->min_chirp_dA;
    maxY = arg->max_chirp_dA;
    break;
  case DIM_CHIRP_dW:
    minY = arg->min_chirp_dW;
    maxY = arg->max_chirp_dW;
    break;
  case DIM_CHIRP_ddA:
    minY = arg->min_chirp_ddA;
    maxY = arg->max_chirp_ddA;
    break;
  case DIM_ALPHA_W:
    minY = arg->fit_W_alpha_min;
    maxY = arg->fit_W_alpha_max;
    break;
  case DIM_ALPHA_dW:
    minY = arg->fit_dW_alpha_min;
    maxY = arg->fit_dW_alpha_max;
    break;
  }

  x0s = rint((x_n-1)/(maxX-minX)*minX);
  y0s = rint((y_n-1)/(maxY-minY)*minY);
  x1s = x0s+x_n-1;
  y1s = y0s+y_n-1;

  xminori = rint((x_n-1)/(maxX-minX)*arg->x_minor);
  yminori = rint((y_n-1)/(maxY-minY)*arg->y_minor);
  xmajori = rint(xminori/arg->x_minor*arg->x_major);
  ymajori = rint(yminori/arg->y_minor*arg->y_major);

  if(xminori<1 || yminori<1 || xmajori<1 || ymajori<1){
    fprintf(stderr,"Bad xmajor/xminor/ymajor/yminor value.\n");
    exit(1);
  }

  if( rint(xmajori*(maxX-minX)/arg->x_major) != x_n-1){
    fprintf(stderr,"Xmajor or Xminor results in non-integer pixel increment.\n");
    exit(1);
  }

  if( rint(ymajori*(maxY-minY)/arg->y_major) != y_n-1){
    fprintf(stderr,"Ymajor or Yminor results in non-integer pixel increment.\n");
    exit(1);
  }

  /* determine ~ padding needed */
  setup_graphs(x0s,x1s,xmajori,arg->x_major,
               y0s,y1s,ymajori,arg->y_major,
               (arg->subtitle1!=0)+(arg->subtitle2!=0)+(arg->subtitle3!=0),
               arg->fontsize);

  if(arg->sweep_steps>1){
    if(!(arg->x_dim==DIM_ESTIMATE_A || arg->y_dim==DIM_ESTIMATE_A) &&
       arg->min_est_A != arg->max_est_A) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_P || arg->y_dim==DIM_ESTIMATE_P) &&
       arg->min_est_P != arg->max_est_P) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_W || arg->y_dim==DIM_ESTIMATE_W) &&
       arg->min_est_W != arg->max_est_W) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_dA || arg->y_dim==DIM_ESTIMATE_dA) &&
       arg->min_est_dA != arg->max_est_dA) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_dW || arg->y_dim==DIM_ESTIMATE_dW) &&
       arg->min_est_dW != arg->max_est_dW) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_ddA || arg->y_dim==DIM_ESTIMATE_ddA) &&
       arg->min_est_ddA != arg->max_est_ddA) est_swept=1;

    if(!(arg->x_dim==DIM_CHIRP_A || arg->y_dim==DIM_CHIRP_A) &&
       arg->min_chirp_A != arg->max_chirp_A) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_P || arg->y_dim==DIM_CHIRP_P) &&
       arg->min_chirp_P != arg->max_chirp_P) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_W || arg->y_dim==DIM_CHIRP_W) &&
       arg->min_chirp_W != arg->max_chirp_W) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_dA || arg->y_dim==DIM_CHIRP_dA) &&
       arg->min_chirp_dA != arg->max_chirp_dA) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_dW || arg->y_dim==DIM_CHIRP_dW) &&
       arg->min_chirp_dW != arg->max_chirp_dW) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_ddA || arg->y_dim==DIM_CHIRP_ddA) &&
       arg->min_chirp_ddA != arg->max_chirp_ddA) chirp_swept=1;

    if(!(arg->x_dim==DIM_ALPHA_W || arg->y_dim==DIM_ALPHA_W) &&
       arg->fit_W_alpha_min != arg->fit_W_alpha_max) fit_swept=1;
    if(!(arg->x_dim==DIM_ALPHA_dW || arg->y_dim==DIM_ALPHA_dW) &&
       arg->fit_dW_alpha_min != arg->fit_dW_alpha_max) fit_swept=1;
  }

  swept = est_swept | chirp_swept | fit_swept;

  if(arg->y_dim==DIM_CHIRP_A &&
     arg->min_chirp_A != arg->max_chirp_A) chirp_swept=1;
  if(arg->y_dim==DIM_CHIRP_P &&
     arg->min_chirp_P != arg->max_chirp_P) chirp_swept=1;
  if(arg->y_dim==DIM_CHIRP_W &&
     arg->min_chirp_W != arg->max_chirp_W) chirp_swept=1;
  if(arg->y_dim==DIM_CHIRP_dA &&
     arg->min_chirp_dA != arg->max_chirp_dA) chirp_swept=1;
  if(arg->y_dim==DIM_CHIRP_dW &&
     arg->min_chirp_dW != arg->max_chirp_dW) chirp_swept=1;
  if(arg->y_dim==DIM_CHIRP_ddA &&
     arg->min_chirp_ddA != arg->max_chirp_ddA) chirp_swept=1;

  if(arg->graph_convergence_max)
    cC = draw_page(!swept?"Convergence":"Worst Case Convergence",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   arg->xaxis_label,
                   arg->yaxis_label,
                   "Iterations:",
                   DT_iterations,
                   arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  if(swept && arg->graph_convergence_delta)
    cC_d = draw_page("Convergence Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Iteration span:",
                     DT_iterations,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(arg->graph_Aerror_max)
    cA = draw_page(!swept?"A (Amplitude) Error":"Maximum A (Amplitude) Error",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   arg->xaxis_label,
                   arg->yaxis_label,
                   "Percentage Error:",
                   DT_percent,
                   arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  if(swept && arg->graph_Aerror_delta)
    cA_d = draw_page("A (Amplitude) Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Delta:",
                     DT_abserror,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(arg->graph_Perror_max)
    cP = draw_page(!swept?"P (Phase) Error":"Maximum P (Phase) Error",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   arg->xaxis_label,
                   arg->yaxis_label,
                   "Error (radians):",
                   DT_abserror,
                   arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(swept && arg->graph_Perror_delta)
    cP_d = draw_page("Phase Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Delta (radians):",
                     DT_abserror,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(arg->fit_W){
    if(arg->graph_Werror_max)
      cW = draw_page(!swept?"W (Frequency) Error":"Maximum W (Frequency) Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Error (cycles/block):",
                     DT_abserror,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

    if(swept && arg->graph_Werror_delta)
      cW_d = draw_page("Frequency Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Delta (cycles/block):",
                       DT_abserror,
                       arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  }

  if(arg->fit_dA){
    if(arg->graph_dAerror_max)
      cdA = draw_page(!swept?"dA (Amplitude Modulation) Error":"Maximum dA (Amplitude Modulation) Error",
                      arg->subtitle1,
                      arg->subtitle2,
                      arg->subtitle3,
                      arg->xaxis_label,
                      arg->yaxis_label,
                      "Error:",
                      DT_abserror,
                      arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

    if(swept && arg->graph_dAerror_delta)
      cdA_d = draw_page("Amplitude Modulation Delta",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        arg->xaxis_label,
                        arg->yaxis_label,
                        "Delta:",
                        DT_abserror,
                        arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  }

  if(arg->fit_dW){
    if(arg->graph_dWerror_max)
      cdW = draw_page(!swept?"dW (Chirp Rate) Error":"Maximum dW (Chirp Rate) Error",
                      arg->subtitle1,
                      arg->subtitle2,
                      arg->subtitle3,
                      arg->xaxis_label,
                      arg->yaxis_label,
                      "Error (cycles/block):",
                      DT_abserror,
                      arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
    if(swept && arg->graph_dWerror_delta)
      cdW_d = draw_page("Chirp Rate Delta",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        arg->xaxis_label,
                        arg->yaxis_label,
                        "Delta (cycles/block):",
                        DT_abserror,
                        arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  }

  if(arg->fit_ddA){
    if(arg->graph_ddAerror_max)
      cddA = draw_page(!swept?"ddA (Amplitude Modulation Squared) Error":
                       "Maximum ddA (Amplitude Modulation Squared) Error",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Error:",
                       DT_abserror,
                       arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
    if(swept && arg->graph_ddAerror_delta)
      cddA_d = draw_page("Amplitude Modulation Squared Delta",
                         arg->subtitle1,
                         arg->subtitle2,
                         arg->subtitle3,
                         arg->xaxis_label,
                         arg->yaxis_label,
                         "Delta:",
                         DT_abserror,
                         arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  }

  if(arg->graph_RMSerror_max)
    cRMS = draw_page(swept?"Maximum RMS Fit Error":
                     "RMS Fit Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percentage Error:",
                     DT_percent,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(swept && arg->graph_RMSerror_delta)
    cRMS_d = draw_page("RMS Error Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Percentage Delta:",
                       DT_percent,
                       arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(arg->window)
    arg->window(window,blocksize);
  else
    for(i=0;i<blocksize;i++)
      window[i]=1.;

  /* graph computation */
  for(xi=0;xi<x_n;xi++){
    chirp chirps[y_n];
    chirp estimates[y_n];
    colvec fitvec[y_n];
    int iter[y_n];
    float rms[y_n];
    int si,sn=(swept && arg->sweep_steps>1 ? arg->sweep_steps : 1);

    fprintf(stderr,"\r%s: column %d/%d...",filebase,xi,x_n-1);

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
      targ[i].fit_symm_norm=arg->fit_symm_norm;
      targ[i].fit_bound_zero=arg->fit_bound_zero;
      targ[i].chirp=chirps;
      //targ[i].in=NULL;
      targ[i].estimate=estimates;
      targ[i].sweep=fitvec;
      targ[i].rms_error=rms;
      targ[i].iterations=iter;
    }

    /* if we're sweeping a parameter, we're going to iterate here for a bit. */
    for(si=0;si<sn;si++){
      max_y=y_n;
      next_y=0;

      /* compute/set chirp and est parameters, potentially compute the
         chirp waveform */
      for(yi=0;yi<y_n;yi++){

        fitvec[yi].fit_W_alpha = W_alpha(arg,
                                         xi,x_n,arg->x_dim,
                                         y_n-yi-1,y_n,arg->y_dim,
                                         si,sn,
                                         arg->sweep_or_rand_p);
        fitvec[yi].fit_dW_alpha = dW_alpha(arg,
                                           xi,x_n,arg->x_dim,
                                           y_n-yi-1,y_n,arg->y_dim,
                                           si,sn,
                                           arg->sweep_or_rand_p);

        set_chirp(chirps+yi,
                  xi,x_n,
                  arg->x_dim&DIM_CHIRP_MASK,
                  y_n-yi-1,y_n,
                  arg->y_dim&DIM_CHIRP_MASK,
                  si,sn,
                  arg->sweep_or_rand_p,
                  arg->min_chirp_A,
                  arg->max_chirp_A,
                  arg->min_chirp_P*2.*M_PI,
                  arg->max_chirp_P*2.*M_PI,
                  arg->min_chirp_W*2.*M_PI/blocksize,
                  arg->max_chirp_W*2.*M_PI/blocksize,
                  arg->min_chirp_dA/blocksize,
                  arg->max_chirp_dA/blocksize,
                  arg->min_chirp_dW*2.*M_PI/blocksize/blocksize,
                  arg->max_chirp_dW*2.*M_PI/blocksize/blocksize,
                  arg->min_chirp_ddA/blocksize/blocksize,
                  arg->max_chirp_ddA/blocksize/blocksize);

        set_chirp(estimates+yi,
                  xi,x_n,
                  arg->x_dim&DIM_ESTIMATE_MASK,
                  y_n-yi-1,y_n,
                  arg->y_dim&DIM_ESTIMATE_MASK,
                  si,sn,
                  arg->sweep_or_rand_p,
                  arg->min_est_A,
                  arg->max_est_A,
                  arg->min_est_P*2.*M_PI,
                  arg->max_est_P*2.*M_PI,
                  arg->min_est_W*2.*M_PI/blocksize,
                  arg->max_est_W*2.*M_PI/blocksize,
                  arg->min_est_dA/blocksize,
                  arg->max_est_dA/blocksize,
                  arg->min_est_dW*2.*M_PI/blocksize/blocksize,
                  arg->max_est_dW*2.*M_PI/blocksize/blocksize,
                  arg->min_est_ddA/blocksize/blocksize,
                  arg->max_est_ddA/blocksize/blocksize);

        if(arg->rel_est_A) estimates[yi].A += chirps[yi].A;
        if(arg->rel_est_P) estimates[yi].P += chirps[yi].P;
        if(arg->rel_est_W) estimates[yi].W += chirps[yi].W;
        if(arg->rel_est_dA) estimates[yi].dA += chirps[yi].dA;
        if(arg->rel_est_dW) estimates[yi].dW += chirps[yi].dW;
        if(arg->rel_est_ddA) estimates[yi].ddA += chirps[yi].ddA;

      }

      if(!chirp_swept){
        for(i=0;i<threads;i++)
          targ[i].in=in;

        for(i=0;i<blocksize;i++){
          double jj = i-blocksize/2+.5;
          double A = chirps[0].A + (chirps[0].dA + chirps[0].ddA*jj)*jj;
          double P = chirps[0].P + (chirps[0].W  + chirps[0].dW *jj)*jj;
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
      int x=x0s+xi;
      int y=y1s-yi;
      float a = 1.;
      if(x%xminori==0 || y%yminori==0) a = .8;
      if(x%xmajori==0 || y%ymajori==0) a = .3;

      /* Convergence graph */
      if(cC){
        set_iter_color(cC,ret_maxiter[yi],a);
        cairo_rectangle(cC,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cC);
      }

      /* Convergence delta graph */
      if(cC_d){
        set_iter_color(cC_d,ret_maxiter[yi]-ret_miniter[yi],a);
        cairo_rectangle(cC_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cC_d);
      }

      /* A error graph */
      if(cA){
        set_error_color(cA,MAX(fabs(ret_maxA[yi]),fabs(ret_minA[yi])),a);
        cairo_rectangle(cA,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cA);
      }

      /* A delta graph */
      if(cA_d){
        set_error_color(cA_d,ret_maxA[yi]-ret_minA[yi],a);
        cairo_rectangle(cA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cA_d);
      }

      /* P error graph */
      if(cP){
        set_error_color(cP,MAX(fabs(ret_maxP[yi]),fabs(ret_minP[yi])),a);
        cairo_rectangle(cP,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cP);
      }

      /* P delta graph */
      if(cP_d){
        set_error_color(cP_d,circular_distance(ret_maxP[yi],ret_minP[yi]),a);
        cairo_rectangle(cP_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cP_d);
      }

      if(cW){
        /* W error graph */
        set_error_color(cW,MAX(fabs(ret_maxW[yi]),fabs(ret_minW[yi]))/2./M_PI*blocksize,a);
        cairo_rectangle(cW,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cW);
      }

        /* W delta graph */
      if(cW_d){
        set_error_color(cW_d,(ret_maxW[yi]-ret_minW[yi])/2./M_PI*blocksize,a);
        cairo_rectangle(cW_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cW_d);
      }

      if(cdA){
        /* dA error graph */
        set_error_color(cdA,MAX(fabs(ret_maxdA[yi]),fabs(ret_mindA[yi]))*blocksize,a);
        cairo_rectangle(cdA,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdA);
      }
        /* dA delta graph */
      if(cdA_d){
        set_error_color(cdA_d,(ret_maxdA[yi]-ret_mindA[yi])*blocksize,a);
        cairo_rectangle(cdA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdA_d);
      }

      if(cdW){
        /* dW error graph */
        set_error_color(cdW,MAX(fabs(ret_maxdW[yi]),fabs(ret_mindW[yi]))/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdW);
      }

      /* dW delta graph */
      if(cdW_d){
        set_error_color(cdW_d,(ret_maxdW[yi]-ret_mindW[yi])/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdW_d);
      }

      if(cddA){
        /* ddA error graph */
        set_error_color(cddA,MAX(fabs(ret_maxddA[yi]),fabs(ret_minddA[yi]))*blocksize*blocksize,a);
        cairo_rectangle(cddA,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cddA);
      }

      /* dA delta graph */
      if(cddA_d){
        set_error_color(cddA_d,(ret_maxddA[yi]-ret_minddA[yi])*blocksize*blocksize,a);
        cairo_rectangle(cddA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cddA_d);
      }

      /* RMS error graph */
      if(cRMS){
        set_error_color(cRMS,ret_maxRMS[yi],a);
        cairo_rectangle(cRMS,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cRMS);
      }

      /* RMS delta graph */
      if(cRMS_d){
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

  destroy_page(cC);
  destroy_page(cA);
  destroy_page(cP);
  destroy_page(cW);
  destroy_page(cdA);
  destroy_page(cdW);
  destroy_page(cddA);
  destroy_page(cRMS);
  destroy_page(cC_d);
  destroy_page(cA_d);
  destroy_page(cP_d);
  destroy_page(cW_d);
  destroy_page(cdA_d);
  destroy_page(cdW_d);
  destroy_page(cddA_d);
  destroy_page(cRMS_d);

  fprintf(stderr," done\n");
}

int main(){
  graph_run arg={
    /* fontsize */      18,
    /* subtitle1 */     "Linear estimation, no ddA fit",
    /* subtitle2 */     "chirp: A=1.0, dA=0., swept phase | estimate A=P=dA=dW=0, estimate W=chirp W",
    /* subtitle3 */     "sine window",
    /* xaxis label */   "W (cycles/block)",
    /* yaxis label */   "dW (cycles/block)",

    /* blocksize */     128,
    /* threads */       32,

    /* window */        window_functions.sine,
    /* fit_tol */       .000001,
    /* gauss_seidel */  1,
    /* fit_W */         1,
    /* fit_dA */        1,
    /* fit_dW */        1,
    /* fit_ddA */       0,
    /* nonlinear */     0,
    /* symm_norm */     0,
    /* bound_zero */    0,
    /* W_alpha_min */   1.,
    /* W_alpha_max */   1.,
    /* dW_alpha_min */  1.,
    /* dW_alpha_max */  1.,

    /* x dimension */   DIM_CHIRP_W,
    /* x steps */       1001,
    /* x major */       1.,
    /* x minor */       .25,
    /* y dimension */   DIM_CHIRP_dW,
    /* y steps */       601,
    /* y major */       1.,
    /* y minor */       .25,
    /* sweep_steps */   32,
    /* randomize_p */   0,

    /* est A range */     0.,  0.,  0, /* relative flag */
    /* est P range */     0.,  0.,  0, /* relative flag */
    /* est W range */     0.,  0.,  1, /* relative flag */
    /* est dA range */    0.,  0.,  0, /* relative flag */
    /* est dW range */    0.,  0.,  0, /* relative flag */
    /* est ddA range */   0.,  0.,  0, /* relative flag */

    /* ch A range */    1.,1.,
    /* ch P range */    0.,1.-1./32.,
    /* ch W range */    0.,10.,
    /* ch dA range */   0.,0.,
    /* ch dW range */   -2.5,2.5,
    /* ch ddA range */  0.,0.,

    /* converge max */    1,
    /* converge del */    0,
    /* max A error */     1,
    /* A error delta */   0,
    /* max P error */     1,
    /* P error delta */   0,
    /* max W error */     1,
    /* W error delta */   0,
    /* max dA error */    1,
    /* dA error delta */  0,
    /* max dW error */    1,
    /* dW error delta */  0,
    /* max ddA error */   1,
    /* ddA error delta */ 0,
    /* max RMS error */   1,
    /* RMS error delta */ 0,

  };

  /* Graphs for dW vs W ****************************************/

  //w_e("linear-dW-vs-W",&arg);
  arg.fit_nonlinear=1;
  arg.subtitle1="Partial nonlinear estimation, no ddA fit";
  //w_e("partial-nonlinear-dW-vs-W",&arg);
  arg.subtitle1="Full nonlinear estimation, no ddA fit";
  arg.fit_nonlinear=2;
  //w_e("full-nonlinear-dW-vs-W",&arg);

  /* Graphs for W estimate distance vs W ************************/

  arg.subtitle1="Linear estimation, no ddA fit";
  arg.fit_nonlinear=0;
  arg.yaxis_label="initial distance from W (cycles/block)";
  arg.y_dim = DIM_ESTIMATE_W;
  arg.min_est_W = -2.5;
  arg.max_est_W =  2.5;
  arg.min_chirp_dW=0.;
  arg.max_chirp_dW=0.;

  //w_e("linear-estW-vs-W",&arg);
  arg.subtitle1="Partial nonlinear estimation, no ddA fit";
  arg.subtitle2="chirp: A=1.0, dA=dW=0., swept phase | estimate A=P=dA=dW=0";
  arg.fit_nonlinear=1;
  //w_e("partial-nonlinear-estW-vs-W",&arg);
  arg.subtitle1="Full nonlinear estimation, no ddA fit";
  arg.fit_nonlinear=2;
  //w_e("full-nonlinear-estW-vs-W",&arg);
  arg.fit_nonlinear=0;

  /* graphs for different windows *******************************/

  arg.min_est_W = -2.5;
  arg.max_est_W =  2.5;
  arg.max_chirp_W =  10;
  arg.fit_nonlinear = 0;
  arg.subtitle1="Linear estimation, no ddA fit";
  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  //w_e("linear-estW-vs-W-rectangular",&arg);

  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  //w_e("linear-estW-vs-W-sine",&arg);

  arg.window = window_functions.hanning;
  arg.subtitle3 = "hanning window";
  //w_e("linear-estW-vs-W-hanning",&arg);

  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  //w_e("linear-estW-vs-W-unimodal",&arg);

  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  //w_e("linear-estW-vs-W-maxwell",&arg);

  arg.min_est_W = -15;
  arg.max_est_W =  15;
  arg.max_chirp_W =  25;
  arg.fit_nonlinear = 2;
  arg.subtitle1="Fully nonlinear estimation, no ddA fit";
  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  //w_e("full-nonlinear-estW-vs-W-rectangular",&arg);

  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  //w_e("full-nonlinear-estW-vs-W-sine",&arg);

  arg.window = window_functions.hanning;
  arg.subtitle3 = "hanning window";
  //w_e("full-nonlinear-estW-vs-W-hanning",&arg);

  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  //w_e("full-nonlinear-estW-vs-W-unimodal",&arg);

  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  //w_e("full-nonlinear-estW-vs-W-maxwell",&arg);

  /* 1, 1.5, 2nd order **********************************************/
  arg.min_est_W = -3;
  arg.max_est_W =  3;
  arg.max_chirp_W =  10;
  arg.window = window_functions.hanning;
  arg.subtitle3 = "hanning window";
  arg.fit_dW = 0;
  arg.fit_dA = 0;

  arg.fit_nonlinear = 2;
  arg.subtitle1="Fully nonlinear estimation, half-order fit (W only)";
  //w_e("nonlinear-estW-vs-W-.5order",&arg);

  arg.fit_dA=1;

  arg.fit_nonlinear = 0;
  arg.subtitle1="Linear estimation, first-order fit";
  //w_e("linear-estW-vs-W-1order",&arg);

  arg.fit_nonlinear = 2;
  arg.subtitle1="Fully nonlinear estimation, first-order fit";
  //w_e("nonlinear-estW-vs-W-1order",&arg);

  arg.fit_dW=1;

  arg.fit_nonlinear = 2;
  arg.subtitle1="Fully nonlinear estimation, 1.5th-order fit (dW only)";
  //w_e("nonlinear-estW-vs-W-1.5order",&arg);

  arg.fit_ddA=1;

  arg.fit_nonlinear = 0;
  arg.subtitle1="Linear estimation, second-order fit";
  //w_e("linear-estW-vs-W-2order",&arg);

  arg.fit_nonlinear = 2;
  arg.subtitle1="Fully nonlinear estimation, second-order fit";
  //w_e("nonlinear-estW-vs-W-2order",&arg);

  /**************** symmetric norm tests ********************/

  arg.fit_nonlinear = 0;
  arg.fit_symm_norm = 1;
  arg.fit_ddA=0;
  arg.min_chirp_dW = -2.5;
  arg.max_chirp_dW =  2.5;
  arg.min_est_W = 0;
  arg.max_est_W = 0;
  arg.y_dim = DIM_CHIRP_dW;
  arg.subtitle2="chirp: A=1.0, dA=0., swept phase | estimate A=P=dA=dW=0, estimate W=chirp W",
  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  arg.yaxis_label = "dW (cycles/block)",

  arg.subtitle1="linear estimation, symmetric normalization, no ddA fit";
  //w_e("linear-dW-vs-W-symmetric",&arg);
  arg.fit_nonlinear=1;
  arg.subtitle1="Partial nonlinear estimation, symmetric normalization, no ddA fit";
  //w_e("partial-nonlinear-dW-vs-W-symmetric",&arg);
  arg.subtitle1="Full nonlinear estimation, symmetric normalization, no ddA fit";
  arg.fit_nonlinear=2;
  //w_e("full-nonlinear-dW-vs-W-symmetric",&arg);

  /* Graphs for W estimate distance vs W ************************/

  arg.subtitle1="Linear estimation, symmetric normalization, no ddA fit";
  arg.fit_nonlinear=0;
  arg.yaxis_label="initial distance from W (cycles/block)";
  arg.y_dim = DIM_ESTIMATE_W;
  arg.min_est_W = -2.5;
  arg.max_est_W =  2.5;
  arg.min_chirp_dW=0.;
  arg.max_chirp_dW=0.;

  //w_e("linear-estW-vs-W-symmetric",&arg);
  arg.subtitle1="Partial nonlinear estimation, symmetric normalization, no ddA fit";
  arg.subtitle2="chirp: A=1.0, dA=dW=0., swept phase | estimate A=P=dA=dW=0";
  arg.fit_nonlinear=1;
  //w_e("partial-nonlinear-estW-vs-W-symmetric",&arg);
  arg.subtitle1="Full nonlinear estimation, symmetric normalization, no ddA fit";
  arg.fit_nonlinear=2;
  //w_e("full-nonlinear-estW-vs-W-symmetric",&arg);
  arg.fit_nonlinear=0;

  /* W alpha *****************************************************/
  /* Y axis = estW */
  arg.x_minor=.0625;
  arg.subtitle1="full nonlinear estimation, no ddA fit, W centered";
  arg.min_chirp_W = arg.max_chirp_W = rint(arg.blocksize/4);

  arg.fit_W_alpha_min = 0;
  arg.fit_W_alpha_max = 2.;
  arg.x_dim = DIM_ALPHA_W;
  arg.xaxis_label = "alphaW",

  arg.fit_nonlinear = 2;
  arg.fit_symm_norm = 0;

  arg.yaxis_label="initial distance from W (cycles/block)";
  arg.y_dim = DIM_ESTIMATE_W;
  arg.min_est_W = -3;
  arg.max_est_W =  3;

  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  //w_e("nonlinear-estW-vs-alphaW-rectangle",&arg);

  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  //w_e("nonlinear-estW-vs-alphaW-sine",&arg);

  arg.window = window_functions.hanning;
  arg.subtitle3 = "rectangular hanning";
  //w_e("nonlinear-estW-vs-alphaW-hanning",&arg);

  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  //w_e("nonlinear-estW-vs-alphaW-unimodal",&arg);

  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  //w_e("nonlinear-estW-vs-alphaW-maxwell",&arg);

  /* dW alpha *****************************************************/

  arg.fit_W_alpha_min = 1.;
  arg.fit_W_alpha_max = 1.;
  arg.fit_dW_alpha_min = 0;
  arg.fit_dW_alpha_max = 3.125;
  arg.x_dim = DIM_ALPHA_dW;
  arg.xaxis_label = "alphadW",

  arg.fit_nonlinear = 2;
  arg.fit_symm_norm = 0;

  arg.yaxis_label="initial distance from W (cycles/block)";
  arg.y_dim = DIM_ESTIMATE_W;
  arg.min_est_W = -3;
  arg.max_est_W =  3;

  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  //w_e("nonlinear-estW-vs-alphadW-rectangle",&arg);

  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  //w_e("nonlinear-estW-vs-alphadW-sine",&arg);

  arg.window = window_functions.hanning;
  arg.subtitle3 = "rectangular hanning";
  //w_e("nonlinear-estW-vs-alphadW-hanning",&arg);

  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  //w_e("nonlinear-estW-vs-alphadW-unimodal",&arg);

  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  //w_e("nonlinear-estW-vs-alphadW-maxwell",&arg);

  arg.yaxis_label="dW (cycles/block)";
  arg.y_dim = DIM_CHIRP_dW;
  arg.min_est_W =  0;
  arg.max_est_W =  0;
  arg.min_chirp_dW = -3;
  arg.max_chirp_dW =  3;

  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  //w_e("nonlinear-dW-vs-alphadW-rectangle",&arg);

  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  //w_e("nonlinear-dW-vs-alphadW-sine",&arg);

  arg.window = window_functions.hanning;
  arg.subtitle3 = "hanning window";
  //w_e("nonlinear-dW-vs-alphadW-hanning",&arg);

  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  //w_e("nonlinear-dW-vs-alphadW-unimodal",&arg);

  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  //w_e("nonlinear-dW-vs-alphadW-maxwell",&arg);

  /* replot earlier fits with opt dW alpha *************************/
  arg.min_chirp_dW = 0;
  arg.max_chirp_dW = 0;
  arg.min_est_W = -9.375;
  arg.max_est_W =  9.375;
  arg.min_chirp_W =  0;
  arg.max_chirp_W =  25;
  arg.fit_nonlinear = 2;
  arg.x_minor = .25;
  arg.x_dim = DIM_CHIRP_W;
  arg.y_dim = DIM_ESTIMATE_W;

  arg.subtitle1="Fully nonlinear estimation, no ddA fit";
  arg.xaxis_label="W (cycles/block)";
  arg.yaxis_label="initial distance from W (cycles/block)";


  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  //w_e("nodWa-estW-vs-W-rectangular",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 2.2;
  arg.subtitle3 = "rectangular window, dW alpha = 2.2";
  //w_e("optdWa-estW-vs-W-rectangular",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  //w_e("nodWa-estW-vs-W-sine",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.711;
  arg.subtitle3 = "sine window, dW alpha = 1.711";
  //w_e("optdWa-estW-vs-W-sine",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.hanning;
  arg.subtitle3 = "hanning window";
  //w_e("nodWa-estW-vs-W-hanning",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.618;
  arg.subtitle3 = "hanning window, dW alpha = 1.618";
  //w_e("optdWa-estW-vs-W-hanning",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  //w_e("nodWa-estW-vs-W-unimodal",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.526;
  arg.subtitle3 = "unimodal triangular/gaussian window, dW alpha = 1.526";
  //w_e("optdWa-estW-vs-W-unimodal",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  //w_e("nodWa-estW-vs-W-maxwell",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.554;
  arg.subtitle3 = "maxwell (optimized) window, dW alpha = 1.554";
  //w_e("optdWa-estW-vs-W-maxwell",&arg);


  arg.min_chirp_dW = -9.375;
  arg.max_chirp_dW = 9.375;
  arg.min_est_W = 0;
  arg.max_est_W = 0;
  arg.min_chirp_W =  0;
  arg.max_chirp_W =  25;
  arg.fit_nonlinear = 2;
  arg.x_minor = .25;
  arg.x_dim = DIM_CHIRP_W;
  arg.y_dim = DIM_CHIRP_dW;

  arg.subtitle1="Fully nonlinear estimation, no ddA fit";
  arg.xaxis_label="W (cycles/block)";
  arg.yaxis_label="dW (cycles/block)";


  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.rectangle;
  arg.subtitle3 = "rectangular window";
  w_e("nodWa-dW-vs-W-rectangular",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 2.125;
  arg.subtitle3 = "rectangular window, dW alpha = 2.125";
  w_e("optdWa-dW-vs-W-rectangular",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.sine;
  arg.subtitle3 = "sine window";
  w_e("nodWa-dW-vs-W-sine",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.711;
  arg.subtitle3 = "sine window, dW alpha = 1.711";
  w_e("optdWa-dW-vs-W-sine",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.hanning;
  arg.subtitle3 = "hanning window";
  w_e("nodWa-dW-vs-W-hanning",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.618;
  arg.subtitle3 = "hanning window, dW alpha = 1.618";
  w_e("optdWa-dW-vs-W-hanning",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.tgauss_deep;
  arg.subtitle3 = "unimodal triangular/gaussian window";
  w_e("nodWa-dW-vs-W-unimodal",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.526;
  arg.subtitle3 = "unimodal triangular/gaussian window, dW alpha = 1.526";
  w_e("optdWa-dW-vs-W-unimodal",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.maxwell1;
  arg.subtitle3 = "maxwell (optimized) window";
  w_e("nodWa-dW-vs-W-maxwell",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.554;
  arg.subtitle3 = "maxwell (optimized) window, dW alpha = 1.554";
  w_e("optdWa-dW-vs-W-maxwell",&arg);


  return 0;
}

