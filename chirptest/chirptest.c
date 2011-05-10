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
  char *filebase;
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

  float white_noise;

  /* generate which graphs? */
  int graph_convergence_av;
  int graph_convergence_max;
  int graph_convergence_delta;

  int graph_Aerror_av;
  int graph_Aerror_max;
  int graph_Aerror_delta;

  int graph_Perror_av;
  int graph_Perror_max;
  int graph_Perror_delta;

  int graph_Werror_av;
  int graph_Werror_max;
  int graph_Werror_delta;

  int graph_dAerror_av;
  int graph_dAerror_max;
  int graph_dAerror_delta;

  int graph_dWerror_av;
  int graph_dWerror_max;
  int graph_dWerror_delta;

  int graph_ddAerror_av;
  int graph_ddAerror_max;
  int graph_ddAerror_delta;

  int graph_RMSerror_av;
  int graph_RMSerror_max;
  int graph_RMSerror_delta;

}  graph_1chirp_arg;

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

  float white_noise;
  chirp *chirp;
  chirp *estimate;
  colvec *sweep;
  float *ssq_error;
  float *ssq_energy;
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

float W_alpha(graph_1chirp_arg *arg,
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

float dW_alpha(graph_1chirp_arg *arg,
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
    float energy_acc=0.;
    float error_acc=0.;

    pthread_mutex_lock(&ymutex);
    y=next_y;
    if(y>=max_y){
      pthread_mutex_unlock(&ymutex);
      break;
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
        energy_acc += chirp[i]*chirp[i]*arg->window[i]*arg->window[i];
      }
      if(arg->white_noise){
        for(i=0;i<blocksize;i++){
          float v = (drand48()-drand48())*2.45; /* (0dB RMS white noise) */
          chirp[i]+=v*arg->white_noise;
        }
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
      float r = A*cos(P);
      float e = (chirp[i]-r)*arg->window[i];
      error_acc += e*e;
    }
    arg->ssq_energy[y] = energy_acc;
    arg->ssq_error[y] = error_acc;
    arg->iterations[y] = arg->max_iterations-ret;
    feclearexcept(FE_ALL_EXCEPT);
    feenableexcept(except);
  }

  if(localinit)free(chirp);
  return NULL;
}

static char subtitle1[320];
static char subtitle2[320];
static char subtitle3[320];
static char filebase[320];

char *dim_to_abbrv(int dim){
  switch(dim){
  case DIM_ESTIMATE_A:
    return "estA";
  case DIM_ESTIMATE_P:
    return "estP";
  case DIM_ESTIMATE_W:
    return "estW";
  case DIM_ESTIMATE_dA:
    return "estdA";
  case DIM_ESTIMATE_dW:
    return "estdW";
  case DIM_ESTIMATE_ddA:
    return "estddA";
  case DIM_CHIRP_A:
    return "A";
  case DIM_CHIRP_P:
    return "P";
  case DIM_CHIRP_W:
    return "W";
  case DIM_CHIRP_dA:
    return "dA";
  case DIM_CHIRP_dW:
    return "dW";
  case DIM_CHIRP_ddA:
    return "ddA";
  case DIM_ALPHA_W:
    return "alphaW";
  case DIM_ALPHA_dW:
    return "alphadW";
  }
  return NULL;
}

void setup_titles_1chirp(graph_1chirp_arg *arg){
  if(!arg->subtitle1){

    int fits = (arg->fit_W || (arg->fit_dA && arg->fit_nonlinear==0)) ? 1:0;
    fits |= (arg->fit_dA || (arg->fit_W && arg->fit_nonlinear==0)) ? 2:0;
    fits |= (arg->fit_dW || (arg->fit_ddA && arg->fit_nonlinear==0)) ? 4:0;
    fits |= (arg->fit_ddA || (arg->fit_dW && arg->fit_nonlinear==0)) ? 8:0;

    subtitle1[0]=0;
    switch(arg->fit_nonlinear){
    case 0: /* linear estimation */
      strcat(subtitle1,"Linear estimation,");
      break;
    case 1: /* partial nonlinear estimation */
      strcat(subtitle1,"Partial nonlinear estimation,");
      break;
    case 2: /* full nonlinear estimation */
      strcat(subtitle1,"Full nonlinear estimation,");
      break;
    default:
      fprintf(stderr,"Unknown nonlinear setting\n");
      exit(1);
    }

    switch(fits){
    case 0:
      strcat(subtitle1," zero-order fit");
      break;
    case 1:
      strcat(subtitle1," first-order fit (no dA)");
      break;
    case 2:
      strcat(subtitle1," first-order fit (no W)");
      break;
    case 3:
      strcat(subtitle1," first-order fit");
      break;

    case 4:
      strcat(subtitle1," second-order fit (no W, dA, ddA)");
      break;
    case 5:
      strcat(subtitle1," second-order fit (no dA, ddA)");
      break;
    case 6:
      strcat(subtitle1," second-order fit (no W, ddA)");
      break;
    case 7:
      strcat(subtitle1," second-order fit (no ddA)");
      break;

    case 8:
      strcat(subtitle1," second-order fit (no W, dA, dW)");
      break;
    case 9:
      strcat(subtitle1," second-order fit (no dA, dW)");
      break;
    case 10:
      strcat(subtitle1," second-order fit (no W, dW)");
      break;
    case 11:
      strcat(subtitle1," second-order fit (no dW)");
      break;

    case 12:
      strcat(subtitle1," second-order fit (no W, dA)");
      break;
    case 13:
      strcat(subtitle1," second-order fit (no dA)");
      break;
    case 14:
      strcat(subtitle1," second-order fit (no W)");
      break;
    case 15:
      strcat(subtitle1," second-order fit");
      break;
    }

    if(arg->white_noise != 0.){
      char buf[80];
      snprintf(buf,80,", white noise @ %.1fdB",todB(arg->white_noise));
      strcat(subtitle1,buf);
    }

    arg->subtitle1 = subtitle1;
  }

  /* subtitle2 */
  if(!arg->subtitle2){
    int zeroes=0;
    int swept=0;
    int expl=0;

    char buf[80];
    subtitle2[0]=0;
    strcat(subtitle2,"chirp:[");

    if(arg->min_chirp_A==0 && (arg->max_chirp_A==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_A && arg->y_dim != DIM_CHIRP_A)
      zeroes++;
    if(arg->min_chirp_P==0 && (arg->max_chirp_P==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_P && arg->y_dim != DIM_CHIRP_P)
      zeroes++;
    if(arg->min_chirp_W==0 && (arg->max_chirp_W==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_W && arg->y_dim != DIM_CHIRP_W)
      zeroes++;
    if(arg->min_chirp_dA==0 && (arg->max_chirp_dA==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_dA && arg->y_dim != DIM_CHIRP_dA)
      zeroes++;
    if(arg->min_chirp_dW==0 && (arg->max_chirp_dW==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_dW && arg->y_dim != DIM_CHIRP_dW)
      zeroes++;
    if(arg->min_chirp_ddA==0 && (arg->max_chirp_ddA==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_ddA && arg->y_dim != DIM_CHIRP_ddA)
      zeroes++;

    if(arg->min_chirp_A==arg->max_chirp_A || arg->sweep_steps<2){
      if(arg->min_chirp_A!=0 || zeroes<2){
        if(arg->x_dim != DIM_CHIRP_A && arg->y_dim != DIM_CHIRP_A){
          snprintf(buf,80,"A=%.0fdB",todB(arg->min_chirp_A));
          strcat(subtitle2,buf);
          expl++;
        }
      }
    }

    if(arg->min_chirp_P==arg->max_chirp_P || arg->sweep_steps<2){
      if(arg->min_chirp_P!=0 || zeroes<2){
        if(arg->x_dim != DIM_CHIRP_P && arg->y_dim != DIM_CHIRP_P){
          if(expl)strcat(subtitle2,", ");
          snprintf(buf,80,"P=%.1f",arg->min_chirp_P);
          strcat(subtitle2,buf);
          expl++;
        }
      }
    }

    if(arg->min_chirp_W==arg->max_chirp_W || arg->sweep_steps<2){
      if(arg->min_chirp_W!=0 || zeroes<2){
        if(arg->x_dim != DIM_CHIRP_W && arg->y_dim != DIM_CHIRP_W){
          if(expl)strcat(subtitle2,", ");
          snprintf(buf,80,"W=Nyquist/%.0f",(arg->blocksize/2)/arg->min_chirp_W);
          strcat(subtitle2,buf);
          expl++;
        }
      }
    }

    if(arg->min_chirp_dA==arg->max_chirp_dA || arg->sweep_steps<2){
      if(arg->min_chirp_dA!=0 || zeroes<2){
        if(arg->x_dim != DIM_CHIRP_dA && arg->y_dim != DIM_CHIRP_dA){
          if(expl)strcat(subtitle2,", ");
          snprintf(buf,80,"dA=%.1f",arg->min_chirp_dA);
          strcat(subtitle2,buf);
          expl++;
        }
      }
    }

    if(arg->min_chirp_dW==arg->max_chirp_dW || arg->sweep_steps<2){
      if(arg->min_chirp_dW!=0 || zeroes<2){
        if(arg->x_dim != DIM_CHIRP_dW && arg->y_dim != DIM_CHIRP_dW){
          if(expl)strcat(subtitle2,", ");
          snprintf(buf,80,"dW=%.1f",arg->min_chirp_dW);
          strcat(subtitle2,buf);
          expl++;
        }
      }
    }

    if(arg->min_chirp_ddA==arg->max_chirp_ddA || arg->sweep_steps<2){
      if(arg->min_chirp_ddA!=0 || zeroes<2){
        if(arg->x_dim != DIM_CHIRP_ddA && arg->y_dim != DIM_CHIRP_ddA){
          if(expl)strcat(subtitle2,", ");
          snprintf(buf,80,"ddA=%.1f",arg->min_chirp_ddA);
          strcat(subtitle2,buf);
          expl++;
        }
      }
    }

    if(expl && zeroes>1)
      strcat(subtitle2,", ");

    if(arg->min_chirp_A==0 && (arg->max_chirp_A==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_A && arg->y_dim != DIM_CHIRP_A)
      strcat(subtitle2,"A=");
    if(arg->min_chirp_P==0 && (arg->max_chirp_P==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_P && arg->y_dim != DIM_CHIRP_P)
      strcat(subtitle2,"P=");
    if(arg->min_chirp_W==0 && (arg->max_chirp_W==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_W && arg->y_dim != DIM_CHIRP_W)
      strcat(subtitle2,"W=");
    if(arg->min_chirp_dA==0 && (arg->max_chirp_dA==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_dA && arg->y_dim != DIM_CHIRP_dA)
      strcat(subtitle2,"dA=");
    if(arg->min_chirp_dW==0 && (arg->max_chirp_dW==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_dW && arg->y_dim != DIM_CHIRP_dW)
      strcat(subtitle2,"dW=");
    if(arg->min_chirp_ddA==0 && (arg->max_chirp_ddA==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_ddA && arg->y_dim != DIM_CHIRP_ddA)
      strcat(subtitle2,"ddA=");
    if(zeroes>1)
      strcat(subtitle2,"0");

    {
      char buf[320];
      buf[0]=0;
      if(arg->min_chirp_A!=arg->max_chirp_A && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_A && arg->y_dim!=DIM_CHIRP_A){
        strcat(buf,"A");
        swept++;
      }
      if(arg->min_chirp_P!=arg->max_chirp_P && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_P && arg->y_dim!=DIM_CHIRP_P){
        if(swept)strcat(buf,",");
        strcat(buf,"P");
        swept++;
      }
      if(arg->min_chirp_W!=arg->max_chirp_W && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_W && arg->y_dim!=DIM_CHIRP_W){
        if(swept)strcat(buf,",");
        strcat(buf,"W");
        swept++;
      }
      if(arg->min_chirp_dA!=arg->max_chirp_dA && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_dA && arg->y_dim!=DIM_CHIRP_dA){
        if(swept)strcat(buf,",");
        strcat(buf,"dA");
        swept++;
      }
      if(arg->min_chirp_dW!=arg->max_chirp_dW && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_dW && arg->y_dim!=DIM_CHIRP_dW){
        if(swept)strcat(buf,",");
        strcat(buf,"dW");
        swept++;
      }
      if(arg->min_chirp_ddA!=arg->max_chirp_ddA && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_ddA && arg->y_dim!=DIM_CHIRP_ddA){
        if(swept)strcat(buf,",");
        strcat(buf,"ddA");
        swept++;
      }

      if(swept){
        if(expl || zeroes>1)
          strcat(subtitle2,", ");

        strcat(subtitle2,"swept ");
        strcat(subtitle2,buf);
      }
    }

    strcat(subtitle2,"] estimate:[");
    zeroes=0;
    expl=0;
    swept=0;
    if(arg->min_est_A==0 && (arg->max_est_A==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_A && arg->y_dim != DIM_ESTIMATE_A &&
       !arg->rel_est_A)
      zeroes++;
    if(arg->min_est_P==0 && (arg->max_est_P==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_P && arg->y_dim != DIM_ESTIMATE_P &&
       !arg->rel_est_P)
      zeroes++;
    if(arg->min_est_W==0 && (arg->max_est_W==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_W && arg->y_dim != DIM_ESTIMATE_W &&
       !arg->rel_est_W)
      zeroes++;
    if(arg->min_est_dA==0 && (arg->max_est_dA==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_dA && arg->y_dim != DIM_ESTIMATE_dA &&
       !arg->rel_est_dA)
      zeroes++;
    if(arg->min_est_dW==0 && (arg->max_est_dW==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_dW && arg->y_dim != DIM_ESTIMATE_dW &&
       !arg->rel_est_dW)
      zeroes++;
    if(arg->min_est_ddA==0 && (arg->max_est_ddA==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_ddA && arg->y_dim != DIM_ESTIMATE_ddA &&
       !arg->rel_est_ddA)
      zeroes++;

    if(arg->min_est_A==arg->max_est_A || arg->sweep_steps<2){
      if(arg->min_est_A==0 && arg->rel_est_A){
        strcat(subtitle2,"A=chirp A");
        expl++;
      }else
        if(arg->min_est_A!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_A && arg->y_dim != DIM_ESTIMATE_A){
            snprintf(buf,80,"A=%.0fdB",todB(arg->min_est_A));
            strcat(subtitle2,buf);
            if(arg->rel_est_A)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->min_est_P==arg->max_est_P || arg->sweep_steps<2){
      if(arg->min_est_P==0 && arg->rel_est_P){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"P=chirp P");
        expl++;
      }else
        if(arg->min_est_P!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_P && arg->y_dim != DIM_ESTIMATE_P){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"P=%.1f",arg->min_est_P);
            strcat(subtitle2,buf);
            if(arg->rel_est_P)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->min_est_W==arg->max_est_W || arg->sweep_steps<2){
      if(arg->min_est_W==0 && arg->rel_est_W){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"W=chirp W");
        expl++;
      }else
        if(arg->min_est_W!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_W && arg->y_dim != DIM_ESTIMATE_W){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"W=Nyquist/%.0f",(arg->blocksize/2)/arg->min_est_W);
            strcat(subtitle2,buf);
            if(arg->rel_est_W)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->min_est_dA==arg->max_est_dA || arg->sweep_steps<2){
      if(arg->min_est_dA==0 && arg->rel_est_dA){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"dA=chirp dA");
        expl++;
      }else
        if(arg->min_est_dA!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_dA && arg->y_dim != DIM_ESTIMATE_dA){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"dA=%.1f",arg->min_est_dA);
            strcat(subtitle2,buf);
            if(arg->rel_est_dA)strcat(subtitle2,"(relative)");
          expl++;
          }
        }
    }

    if(arg->min_est_dW==arg->max_est_dW || arg->sweep_steps<2){
      if(arg->min_est_dW==0 && arg->rel_est_dW){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"dW=chirp dW");
        expl++;
      }else
        if(arg->min_est_dW!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_dW && arg->y_dim != DIM_ESTIMATE_dW){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"dW=%.1f",arg->min_est_dW);
            strcat(subtitle2,buf);
            if(arg->rel_est_dW)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->min_est_ddA==arg->max_est_ddA || arg->sweep_steps<2){
      if(arg->min_est_ddA==0 && arg->rel_est_ddA){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"ddA=chirp ddA");
        expl++;
      }else
        if(arg->min_est_ddA!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_ddA && arg->y_dim != DIM_ESTIMATE_ddA){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"ddA=%.1f",arg->min_est_ddA);
            strcat(subtitle2,buf);
            if(arg->rel_est_ddA)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }
    
    if(expl && zeroes>1)
      strcat(subtitle2,", ");

    if(arg->min_est_A==0 && (arg->max_est_A==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_A &&
       arg->y_dim != DIM_ESTIMATE_A && !arg->rel_est_A)
      strcat(subtitle2,"A=");
    if(arg->min_est_P==0 && (arg->max_est_P==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_P &&
       arg->y_dim != DIM_ESTIMATE_P && !arg->rel_est_P)
      strcat(subtitle2,"P=");
    if(arg->min_est_W==0 && (arg->max_est_W==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_W &&
       arg->y_dim != DIM_ESTIMATE_W && !arg->rel_est_W)
      strcat(subtitle2,"W=");
    if(arg->min_est_dA==0 && (arg->max_est_dA==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_dA &&
       arg->y_dim != DIM_ESTIMATE_dA && !arg->rel_est_dA)
      strcat(subtitle2,"dA=");
    if(arg->min_est_dW==0 && (arg->max_est_dW==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_dW &&
       arg->y_dim != DIM_ESTIMATE_dW && !arg->rel_est_dW)
      strcat(subtitle2,"dW=");
    if(arg->min_est_ddA==0 && (arg->max_est_ddA==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_ddA &&
       arg->y_dim != DIM_ESTIMATE_ddA && !arg->rel_est_ddA)
      strcat(subtitle2,"ddA=");
    if(zeroes>1)
      strcat(subtitle2,"0");

    {
      char buf[320];
      buf[0]=0;
      if(arg->min_est_A!=arg->max_est_A && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_A && arg->y_dim!=DIM_ESTIMATE_A){
        strcat(buf,"A");
        if(arg->rel_est_A)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->min_est_P!=arg->max_est_P && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_P && arg->y_dim!=DIM_ESTIMATE_P){
        if(swept)strcat(buf,",");
        strcat(buf,"P");
        if(arg->rel_est_P)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->min_est_W!=arg->max_est_W && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_W && arg->y_dim!=DIM_ESTIMATE_W){
        if(swept)strcat(buf,",");
        strcat(buf,"W");
        if(arg->rel_est_W)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->min_est_dA!=arg->max_est_dA && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_dA && arg->y_dim!=DIM_ESTIMATE_dA){
        if(swept)strcat(buf,",");
        strcat(buf,"dA");
        if(arg->rel_est_dA)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->min_est_dW!=arg->max_est_dW && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_dW && arg->y_dim!=DIM_ESTIMATE_dW){
        if(swept)strcat(buf,",");
        strcat(buf,"dW");
        if(arg->rel_est_dW)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->min_est_ddA!=arg->max_est_ddA && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_ddA && arg->y_dim!=DIM_ESTIMATE_ddA){
        if(swept)strcat(buf,",");
        strcat(buf,"ddA");
        if(arg->rel_est_ddA)strcat(buf,"(relative)");
        swept++;
      }

      if(swept){
        if(expl || zeroes>1)
          strcat(subtitle2,", ");

        strcat(subtitle2," swept ");
        strcat(subtitle2,buf);
      }
    }

    strcat(subtitle2,"]");
    arg->subtitle2 = subtitle2;
  }

  if(!arg->subtitle3){
    char buf[80];
    subtitle3[0]=0;

    if(arg->window == window_functions.rectangle)
      strcat(subtitle3,"rectangular window");
    if(arg->window == window_functions.sine)
      strcat(subtitle3,"sine window");
    if(arg->window == window_functions.hanning)
      strcat(subtitle3,"hanning window");
    if(arg->window == window_functions.vorbis)
      strcat(subtitle3,"vorbis window");
    if(arg->window == window_functions.blackman_harris)
      strcat(subtitle3,"blackmann-harris window");
    if(arg->window == window_functions.tgauss_deep)
      strcat(subtitle3,"unimodal triangular/gaussian window");
    if(arg->window == window_functions.dolphcheb)
      strcat(subtitle3,"dolph-chebyshev window");
    if(arg->window == window_functions.maxwell1)
      strcat(subtitle3,"maxwell (optimized) window");

    if(arg->x_dim != DIM_ALPHA_W && arg->y_dim != DIM_ALPHA_W){
      if(arg->fit_W_alpha_min==arg->fit_W_alpha_max || arg->sweep_steps<2){
        if(arg->fit_W_alpha_min!=1.0){
          snprintf(buf,80,", alpha_W=%.2f",arg->fit_W_alpha_min);
          strcat(subtitle3,buf);
        }
      }
      if(arg->fit_W_alpha_min!=arg->fit_W_alpha_max && arg->sweep_steps>1){
        snprintf(buf,80,", swept alpha_W");
        strcat(subtitle3,buf);
      }
    }

    if(arg->x_dim != DIM_ALPHA_dW && arg->y_dim != DIM_ALPHA_dW){
      if(arg->fit_dW_alpha_min==arg->fit_dW_alpha_max || arg->sweep_steps<2){
        if(arg->fit_dW_alpha_min!=1.0){
          snprintf(buf,80,", alpha_dW=%.3f",arg->fit_dW_alpha_min);
          strcat(subtitle3,buf);
        }
      }
      if(arg->fit_dW_alpha_min!=arg->fit_dW_alpha_max && arg->sweep_steps>1){
        snprintf(buf,80,", swept alpha_dW");
        strcat(subtitle3,buf);
      }
    }
    arg->subtitle3=subtitle3;
  }

  if(!arg->xaxis_label){
    switch(arg->x_dim){
    case DIM_ESTIMATE_A:
      if(arg->rel_est_A)
        arg->xaxis_label="initial estimate distance from A";
      else
        arg->xaxis_label="initial estimated A";
      break;
    case DIM_ESTIMATE_P:
      if(arg->rel_est_P)
        arg->xaxis_label="initial estimate distance from P (radians)";
      else
        arg->xaxis_label="initial estimated P (radians)";
      break;
    case DIM_ESTIMATE_W:
      if(arg->rel_est_W)
        arg->xaxis_label="initial estimate distance from W (cycles/block)";
      else
        arg->xaxis_label="initial estimated W (cycles/block)";
      break;
    case DIM_ESTIMATE_dA:
      if(arg->rel_est_dA)
        arg->xaxis_label="initial estimate distance from dA";
      else
        arg->xaxis_label="initial estimated dA";
      break;
    case DIM_ESTIMATE_dW:
      if(arg->rel_est_dW)
        arg->xaxis_label="initial estimate distance from dW (cycles/block)";
      else
        arg->xaxis_label="initial estimated dW (cycles/block)";
      break;
    case DIM_ESTIMATE_ddA:
      if(arg->rel_est_ddA)
        arg->xaxis_label="initial estimate distance from ddA";
      else
        arg->xaxis_label="initial estimated ddA";
      break;
    case DIM_CHIRP_A:
      arg->xaxis_label="A";
      break;
    case DIM_CHIRP_P:
      arg->xaxis_label="P (radians)";
      break;
    case DIM_CHIRP_W:
      arg->xaxis_label="W (cycles/block)";
      break;
    case DIM_CHIRP_dA:
      arg->xaxis_label="dA";
      break;
    case DIM_CHIRP_dW:
      arg->xaxis_label="dW (cycles/block)";
      break;
    case DIM_CHIRP_ddA:
      arg->xaxis_label="ddA";
      break;
    case DIM_ALPHA_W:
      arg->xaxis_label="alpha_W";
      break;
    case DIM_ALPHA_dW:
      arg->xaxis_label="alpha_dW";
      break;
    }
  }

  if(!arg->yaxis_label){
    switch(arg->y_dim){
    case DIM_ESTIMATE_A:
      if(arg->rel_est_A)
        arg->yaxis_label="initial estimate distance from A";
      else
        arg->yaxis_label="initial estimated A";
      break;
    case DIM_ESTIMATE_P:
      if(arg->rel_est_P)
        arg->yaxis_label="initial estimate distance from P (radians)";
      else
        arg->yaxis_label="initial estimated P (radians)";
      break;
    case DIM_ESTIMATE_W:
      if(arg->rel_est_W)
        arg->yaxis_label="initial estimate distance from W (cycles/block)";
      else
        arg->yaxis_label="initial estimated W (cycles/block)";
      break;
    case DIM_ESTIMATE_dA:
      if(arg->rel_est_dA)
        arg->yaxis_label="initial estimate distance from dA";
      else
        arg->yaxis_label="initial estimated dA";
      break;
    case DIM_ESTIMATE_dW:
      if(arg->rel_est_dW)
        arg->yaxis_label="initial estimate distance from dW (cycles/block)";
      else
        arg->yaxis_label="initial estimated dW (cycles/block)";
      break;
    case DIM_ESTIMATE_ddA:
      if(arg->rel_est_ddA)
        arg->yaxis_label="initial estimate distance from ddA";
      else
        arg->yaxis_label="initial estimated ddA";
      break;
    case DIM_CHIRP_A:
      arg->yaxis_label="A";
      break;
    case DIM_CHIRP_P:
      arg->yaxis_label="P (radians)";
      break;
    case DIM_CHIRP_W:
      arg->yaxis_label="W (cycles/block)";
      break;
    case DIM_CHIRP_dA:
      arg->yaxis_label="dA";
      break;
    case DIM_CHIRP_dW:
      arg->yaxis_label="dW (cycles/block)";
      break;
    case DIM_CHIRP_ddA:
      arg->yaxis_label="ddA";
      break;
    case DIM_ALPHA_W:
      arg->yaxis_label="alpha_W";
      break;
    case DIM_ALPHA_dW:
      arg->yaxis_label="alpha_dW";
      break;
    }
  }

  if(!arg->filebase){
    filebase[0]=0;

    switch(arg->fit_nonlinear){
    case 0:
      strcat(filebase,"linear-");
      break;
    case 1:
      strcat(filebase,"partial-nonlinear-");
      break;
    case 2:
      strcat(filebase,"full-nonlinear-");
      break;
    }

    strcat(filebase,dim_to_abbrv(arg->y_dim));
    strcat(filebase,"-vs-");
    strcat(filebase,dim_to_abbrv(arg->x_dim));
    strcat(filebase,"-");

    if(arg->window == window_functions.rectangle)
      strcat(filebase,"rectangular");
    if(arg->window == window_functions.sine)
      strcat(filebase,"sine");
    if(arg->window == window_functions.hanning)
      strcat(filebase,"hanning");
    if(arg->window == window_functions.vorbis)
      strcat(filebase,"vorbis");
    if(arg->window == window_functions.blackman_harris)
      strcat(filebase,"blackmann-harris");
    if(arg->window == window_functions.tgauss_deep)
      strcat(filebase,"unimodal");
    if(arg->window == window_functions.dolphcheb)
      strcat(filebase,"dolph-chebyshev");
    if(arg->window == window_functions.maxwell1)
      strcat(filebase,"maxwell");

    arg->filebase=filebase;
  }
}

/* performs a W initial estimate error vs chirp W plot.  Ignores the
   est and chirp arguments for W; these are pulled from the x and y setup */

void graph_1chirp(char *filepre,graph_1chirp_arg *inarg){
  graph_1chirp_arg args=*inarg;
  graph_1chirp_arg *arg=&args;
  int threads=arg->threads;
  int blocksize = arg->blocksize;
  float window[blocksize];
  float in[blocksize];
  int i,xi,yi;

  int x_n = arg->x_steps;
  int y_n = arg->y_steps;

  cairo_t *cC_w=NULL;
  cairo_t *cA_w=NULL;
  cairo_t *cP_w=NULL;
  cairo_t *cW_w=NULL;
  cairo_t *cdA_w=NULL;
  cairo_t *cdW_w=NULL;
  cairo_t *cddA_w=NULL;
  cairo_t *cRMS_w=NULL;

  cairo_t *cC_m=NULL;
  cairo_t *cA_m=NULL;
  cairo_t *cP_m=NULL;
  cairo_t *cW_m=NULL;
  cairo_t *cdA_m=NULL;
  cairo_t *cdW_m=NULL;
  cairo_t *cddA_m=NULL;
  cairo_t *cRMS_m=NULL;

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

  float ret_sqA[y_n];
  float ret_enA[y_n];
  float ret_sqP[y_n];
  float ret_sqW[y_n];
  float ret_sqdA[y_n];
  float ret_sqdW[y_n];
  float ret_sqddA[y_n];
  float ret_sqERR[y_n];
  float ret_sqENE[y_n];
  float ret_sumiter[y_n];

  float minX=0,maxX=0;
  float minY=0,maxY=0;
  int x0s,x1s;
  int y0s,y1s;
  int xmajori,ymajori;
  int xminori,yminori;

  char *filebase;

  struct timeval last;
  gettimeofday(&last,NULL);
  setup_titles_1chirp(arg);

  filebase=calloc(strlen(filepre)+strlen(arg->filebase?arg->filebase:"")+1,
                  sizeof(*filebase));
  strcat(filebase,filepre);
  strcat(filebase,arg->filebase);

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

  if(arg->white_noise != 0.) chirp_swept=1;

  if(arg->graph_convergence_av)
    cC_m = draw_page(!swept?"Convergence":"Average Convergence",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Iterations:",
                     DT_iterations,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  if(swept && arg->graph_convergence_max)
    cC_w = draw_page("Worst Case Convergence",
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

  if(arg->graph_Aerror_av)
    cA_m = draw_page(!swept?"A (Amplitude) Error":"A (Amplitude) Mean Squared Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percent Error:",
                     DT_percent,
                   arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  if(swept && arg->graph_Aerror_max)
    cA_w = draw_page("A (Amplitude) Worst Case Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percent Error:",
                     DT_percent,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);
  if(swept && arg->graph_Aerror_delta)
    cA_d = draw_page("A (Amplitude) Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percent Error Delta:",
                     DT_percent,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(arg->graph_Perror_av)
    cP_m = draw_page(!swept?"P (Phase) Error":"P (Phase) Mean Squared Error",
                   arg->subtitle1,
                   arg->subtitle2,
                   arg->subtitle3,
                   arg->xaxis_label,
                   arg->yaxis_label,
                   "Error (radians):",
                   DT_abserror,
                   arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(swept && arg->graph_Perror_max)
    cP_w = draw_page("P (Phase) Worst Case Error",
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
    if(arg->graph_Werror_av)
      cW_m = draw_page(!swept?"W (Frequency) Error":"W (Frequency) Mean Squared Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Error (cycles/block):",
                     DT_abserror,
                     arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

    if(swept && arg->graph_Werror_max)
      cW_w = draw_page("W (Frequency) Worst Case Error",
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
    if(arg->graph_dAerror_av)
      cdA_m = draw_page(!swept?"dA (Amplitude Modulation) Error":"dA (Amplitude Modulation) Mean Squared Error",
                      arg->subtitle1,
                      arg->subtitle2,
                      arg->subtitle3,
                      arg->xaxis_label,
                      arg->yaxis_label,
                      "Error:",
                      DT_abserror,
                      arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

    if(swept && arg->graph_dAerror_max)
      cdA_w = draw_page("dA (Amplitude Modulation) Worst Case Error",
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
    if(arg->graph_dWerror_av)
      cdW_m = draw_page(!swept?"dW (Chirp Rate) Error":"dW (Chirp Rate) Mean Squared Error",
                      arg->subtitle1,
                      arg->subtitle2,
                      arg->subtitle3,
                      arg->xaxis_label,
                      arg->yaxis_label,
                      "Error (cycles/block):",
                      DT_abserror,
                      arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

    if(swept && arg->graph_dWerror_max)
      cdW_w = draw_page("dW (Chirp Rate) Worst Case Error",
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
    if(arg->graph_ddAerror_av)
      cddA_m = draw_page(!swept?"ddA (Amplitude Modulation Squared) Error":
                         "ddA (Amplitude Modulation Squared) Mean Squared Error",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Error:",
                       DT_abserror,
                       arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

    if(swept && arg->graph_ddAerror_max)
      cddA_w = draw_page("ddA (Amplitude Modulation Squared) Worst Case Error",
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

  if(arg->graph_RMSerror_av)
    cRMS_m = draw_page(!swept ? "RMS Fit Error" : "RMS Fit Error (Full Sweep)",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Percentage Error:",
                       DT_percent,
                       arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(swept && arg->graph_RMSerror_max)
    cRMS_w = draw_page("RMS Worst Case Fit Error",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Percentage Error:",
                       DT_percent,
                       arg->x_dim==DIM_CHIRP_W ||arg->x_dim==DIM_ESTIMATE_W);

  if(swept && arg->graph_RMSerror_delta)
    cRMS_d = draw_page("RMS Fit Error Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Percentage Error:",
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
    float error[y_n];
    float energy[y_n];
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
      targ[i].white_noise=arg->white_noise;
      targ[i].ssq_error=error;
      targ[i].ssq_energy=energy;
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
          float Aen = chirps[i].A;
          float Ae = chirps[i].A - estimates[i].A;
          float Pe = circular_distance(chirps[i].P,estimates[i].P);
          float We = chirps[i].W - estimates[i].W;
          float dAe = chirps[i].dA - estimates[i].dA;
          float dWe = chirps[i].dW - estimates[i].dW;
          float ddAe = chirps[i].ddA - estimates[i].ddA;
          ret_sqA[i]=(ret_minA[i]=ret_maxA[i]=Ae)*Ae;
          ret_enA[i]=Aen*Aen;
          ret_sqP[i]=(ret_minP[i]=ret_maxP[i]=Pe)*Pe;
          ret_sqW[i]=(ret_minW[i]=ret_maxW[i]=We)*We;
          ret_sqdA[i]=(ret_mindA[i]=ret_maxdA[i]=dAe)*dAe;
          ret_sqdW[i]=(ret_mindW[i]=ret_maxdW[i]=dWe)*dWe;
          ret_sqddA[i]=(ret_minddA[i]=ret_maxddA[i]=ddAe)*ddAe;
          ret_minRMS[i]=ret_maxRMS[i]=
            (energy[i]>1e-20?sqrt(error[i])/sqrt(energy[i]):1.);
          ret_sqERR[i]=error[i];
          ret_sqENE[i]=energy[i];
          ret_sumiter[i]=ret_miniter[i]=ret_maxiter[i]=iter[i];
        }
      }else{
        for(i=0;i<y_n;i++){
          float v = chirps[i].A - estimates[i].A;
          if(ret_minA[i]>v)ret_minA[i]=v;
          if(ret_maxA[i]<v)ret_maxA[i]=v;
          ret_sqA[i]+=v*v;
          ret_enA[i]+=chirps[i].A*chirps[i].A;

          v = circular_distance(chirps[i].P, estimates[i].P);
          if(ret_minP[i]>v)ret_minP[i]=v;
          if(ret_maxP[i]<v)ret_maxP[i]=v;
          ret_sqP[i]+=v*v;

          v = chirps[i].W - estimates[i].W;
          if(ret_minW[i]>v)ret_minW[i]=v;
          if(ret_maxW[i]<v)ret_maxW[i]=v;
          ret_sqW[i]+=v*v;

          v = chirps[i].dA - estimates[i].dA;
          if(ret_mindA[i]>v)ret_mindA[i]=v;
          if(ret_maxdA[i]<v)ret_maxdA[i]=v;
          ret_sqdA[i]+=v*v;

          v = chirps[i].dW - estimates[i].dW;
          if(ret_mindW[i]>v)ret_mindW[i]=v;
          if(ret_maxdW[i]<v)ret_maxdW[i]=v;
          ret_sqdW[i]+=v*v;

          v = chirps[i].ddA - estimates[i].ddA;
          if(ret_minddA[i]>v)ret_minddA[i]=v;
          if(ret_maxddA[i]<v)ret_maxddA[i]=v;
          ret_sqddA[i]+=v*v;

          v=(energy[i]>1e-20?sqrt(error[i])/sqrt(energy[i]):1.);
          if(ret_minRMS[i]>v)ret_minRMS[i]=v;
          if(ret_maxRMS[i]<v)ret_maxRMS[i]=v;
          ret_sqERR[i]+=error[i];
          ret_sqENE[i]+=energy[i];

          if(ret_miniter[i]>iter[i])ret_miniter[i]=iter[i];
          if(ret_maxiter[i]<iter[i])ret_maxiter[i]=iter[i];
          ret_sumiter[i]+=iter[i];

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

      /* Average convergence graph */
      if(cC_m){
        set_iter_color(cC_m,rint(ret_sumiter[yi]/sn),a);
        cairo_rectangle(cC_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cC_m);
      }

      /* Worst case convergence graph */
      if(cC_w){
        set_iter_color(cC_w,ret_maxiter[yi],a);
        cairo_rectangle(cC_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cC_w);
      }

      /* Convergence delta graph */
      if(cC_d){
        set_iter_color(cC_d,ret_maxiter[yi]-ret_miniter[yi],a);
        cairo_rectangle(cC_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cC_d);
      }

      /* A MSE graph */
      if(cA_m){
        set_error_color(cA_m,(ret_enA[yi]>1e-20?
                            sqrt(ret_sqA[yi])/sqrt(ret_enA[yi]) : 1.),a);
        cairo_rectangle(cA_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cA_m);
      }

      /* A peak error graph */
      if(cA_w){
        set_error_color(cA_w,MAX(fabs(ret_maxA[yi]),fabs(ret_minA[yi])),a);
        cairo_rectangle(cA_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cA_w);
      }

      /* A delta graph */
      if(cA_d){
        set_error_color(cA_d,ret_maxA[yi]-ret_minA[yi],a);
        cairo_rectangle(cA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cA_d);
      }

      /* P MSE graph */
      if(cP_m){
        set_error_color(cP_m,sqrt(ret_sqP[yi]/sn),a);
        cairo_rectangle(cP_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cP_m);
      }

      /* P peak error graph */
      if(cP_w){
        set_error_color(cP_w,MAX(fabs(ret_maxP[yi]),fabs(ret_minP[yi])),a);
        cairo_rectangle(cP_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cP_w);
      }

      /* P delta graph */
      if(cP_d){
        set_error_color(cP_d,circular_distance(ret_maxP[yi],ret_minP[yi]),a);
        cairo_rectangle(cP_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cP_d);
      }

      if(cW_m){
        /* W MSE graph */
        set_error_color(cW_m,sqrt(ret_sqW[yi]/sn)/2./M_PI*blocksize,a);
        cairo_rectangle(cW_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cW_m);
      }

      if(cW_w){
        /* W peak error graph */
        set_error_color(cW_w,MAX(fabs(ret_maxW[yi]),fabs(ret_minW[yi]))/2./M_PI*blocksize,a);
        cairo_rectangle(cW_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cW_w);
      }

        /* W delta graph */
      if(cW_d){
        set_error_color(cW_d,(ret_maxW[yi]-ret_minW[yi])/2./M_PI*blocksize,a);
        cairo_rectangle(cW_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cW_d);
      }

      if(cdA_m){
        /* dA MSE graph */
        set_error_color(cdA_m,sqrt(ret_sqdA[yi]/sn)*blocksize,a);
        cairo_rectangle(cdA_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdA_m);
      }

      if(cdA_w){
        /* dA peak error graph */
        set_error_color(cdA_w,MAX(fabs(ret_maxdA[yi]),fabs(ret_mindA[yi]))*blocksize,a);
        cairo_rectangle(cdA_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdA_w);
      }

        /* dA delta graph */
      if(cdA_d){
        set_error_color(cdA_d,(ret_maxdA[yi]-ret_mindA[yi])*blocksize,a);
        cairo_rectangle(cdA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdA_d);
      }

      if(cdW_m){
        /* dW MSE graph */
        set_error_color(cdW_m,sqrt(ret_sqdW[yi]/sn)/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdW_m);
      }

      if(cdW_w){
        /* dW peak error graph */
        set_error_color(cdW_w,MAX(fabs(ret_maxdW[yi]),fabs(ret_mindW[yi]))/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdW_w);
      }

      /* dW delta graph */
      if(cdW_d){
        set_error_color(cdW_d,(ret_maxdW[yi]-ret_mindW[yi])/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cdW_d);
      }

      if(cddA_m){
        /* ddA MSE graph */
        set_error_color(cddA_m,sqrt(ret_sqddA[yi]/sn)*blocksize*blocksize,a);
        cairo_rectangle(cddA_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cddA_m);
      }

      if(cddA_w){
        /* ddA peak error graph */
        set_error_color(cddA_w,MAX(fabs(ret_maxddA[yi]),fabs(ret_minddA[yi]))*blocksize*blocksize,a);
        cairo_rectangle(cddA_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cddA_w);
      }

      /* dA delta graph */
      if(cddA_d){
        set_error_color(cddA_d,(ret_maxddA[yi]-ret_minddA[yi])*blocksize*blocksize,a);
        cairo_rectangle(cddA_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cddA_d);
      }

      /* RMS error graph */
      if(cRMS_m){
        set_error_color(cRMS_m,
                        ret_sqENE[yi]>1e-20?
                        sqrt(ret_sqERR[yi])/sqrt(ret_sqENE[yi]):1,a);
        cairo_rectangle(cRMS_m,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cRMS_m);
      }

      /* RMS peak error graph */
      if(cRMS_w){
        set_error_color(cRMS_w,ret_maxRMS[yi],a);
        cairo_rectangle(cRMS_w,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cRMS_w);
      }

      if(cRMS_d){
        set_error_color(cRMS_d,ret_maxRMS[yi]-ret_minRMS[yi],a);
        cairo_rectangle(cRMS_d,xi+leftpad,yi+toppad,1,1);
        cairo_fill(cRMS_d);
      }
    }

    /* dump a graph update every 20 seconds */
    {
      struct timeval now;
      gettimeofday(&now,NULL);
      if(now.tv_sec-last.tv_sec + (now.tv_usec-last.tv_usec)/1000000. > 20.){
        last=now;

        to_png(cC_w,filebase,"convmax");
        to_png(cA_w,filebase,"Amaxerror");
        to_png(cP_w,filebase,"Pmaxerror");
        to_png(cW_w,filebase,"Wmaxerror");
        to_png(cdA_w,filebase,"dAmaxerror");
        to_png(cdW_w,filebase,"dWmaxerror");
        to_png(cddA_w,filebase,"ddAmaxerror");
        to_png(cRMS_w,filebase,"RMSmaxerror");

        to_png(cC_m,filebase,"convmean");
        to_png(cA_m,filebase,"Amse");
        to_png(cP_m,filebase,"Pmse");
        to_png(cW_m,filebase,"Wmse");
        to_png(cdA_m,filebase,"dAmse");
        to_png(cdW_m,filebase,"dWmse");
        to_png(cddA_m,filebase,"ddAmse");
        to_png(cRMS_m,filebase,"RMSmse");

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

  to_png(cC_w,filebase,"convmax");
  to_png(cA_w,filebase,"Amaxerror");
  to_png(cP_w,filebase,"Pmaxerror");
  to_png(cW_w,filebase,"Wmaxerror");
  to_png(cdA_w,filebase,"dAmaxerror");
  to_png(cdW_w,filebase,"dWmaxerror");
  to_png(cddA_w,filebase,"ddAmaxerror");
  to_png(cRMS_w,filebase,"RMSmaxerror");

  to_png(cC_m,filebase,"convmean");
  to_png(cA_m,filebase,"Amse");
  to_png(cP_m,filebase,"Pmse");
  to_png(cW_m,filebase,"Wmse");
  to_png(cdA_m,filebase,"dAmse");
  to_png(cdW_m,filebase,"dWmse");
  to_png(cddA_m,filebase,"ddAmse");
  to_png(cRMS_m,filebase,"RMSmse");

  to_png(cC_d,filebase,"convdelta");
  to_png(cA_d,filebase,"Adelta");
  to_png(cP_d,filebase,"Pdelta");
  to_png(cW_d,filebase,"Wdelta");
  to_png(cdA_d,filebase,"dAdelta");
  to_png(cdW_d,filebase,"dWdelta");
  to_png(cddA_d,filebase,"ddAdelta");
  to_png(cRMS_d,filebase,"RMSdelta");

  destroy_page(cC_w);
  destroy_page(cA_w);
  destroy_page(cP_w);
  destroy_page(cW_w);
  destroy_page(cdA_w);
  destroy_page(cdW_w);
  destroy_page(cddA_w);
  destroy_page(cRMS_w);

  destroy_page(cC_m);
  destroy_page(cA_m);
  destroy_page(cP_m);
  destroy_page(cW_m);
  destroy_page(cdA_m);
  destroy_page(cdW_m);
  destroy_page(cddA_m);
  destroy_page(cRMS_m);

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

void init_arg(graph_1chirp_arg *arg){
  *arg=(graph_1chirp_arg){
    /* fontsize */      18,
    /* titles */        0,0,0,0,0,0,
    /* blocksize */     256,
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

    /* x dimension */   0,
    /* x steps */       1001,
    /* x major */       1.,
    /* x minor */       .25,
    /* y dimension */   0,
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
    /* ch W range */    0.,0.,
    /* ch dA range */   0.,0.,
    /* ch dW range */   0.,0.,
    /* ch ddA range */  0.,0.,

    /* additive white noise */ 0.,

    /* error graphs: average/MSE, worst case (peak), delta */
    /* converge */     0,1,0,
    /* A error  */     0,1,0,
    /* P error */      0,1,0,
    /* W error */      0,1,0,
    /* dA error */     0,1,0,
    /* dW error */     0,1,0,
    /* ddA error */    0,1,0,
    /* RMS error */    0,1,0,
  };
}


int main(){
  graph_1chirp_arg arg;

  /* Graphs for linear v. partial-nonlinear v. full-nonlinear ***************/
  /* dW vs W ****************************************************************/
  init_arg(&arg);
  arg.max_chirp_W=10.;
  arg.min_chirp_dW=-2.5;
  arg.max_chirp_dW=2.5;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_CHIRP_dW;

  arg.fit_nonlinear=0;
  //graph_1chirp("algo-",&arg);

  arg.fit_nonlinear=1;
  //graph_1chirp("algo-",&arg);

  arg.fit_nonlinear=2;
  //graph_1chirp("algo-",&arg);

  /* Graphs for linear v. partial-nonlinear v. full-nonlinear ***************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.max_chirp_W=10.;
  arg.min_est_W=-2.5;
  arg.max_est_W=2.5;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;

  arg.fit_nonlinear=0;
  //graph_1chirp("algo-",&arg);

  arg.fit_nonlinear=1;
  //graph_1chirp("algo-",&arg);

  arg.fit_nonlinear=2;
  //graph_1chirp("algo-",&arg);

  /* Graphs for comparison of various windows *******************************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.min_est_W=-2.5;
  arg.max_est_W=2.5;
  arg.max_chirp_W=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;

  arg.fit_nonlinear = 0;

  arg.window = window_functions.rectangle;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.sine;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.hanning;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.maxwell1;
  //graph_1chirp("win-",&arg);

  arg.min_est_W = -15;
  arg.max_est_W =  15;
  arg.max_chirp_W =  25;
  arg.fit_nonlinear = 2;
  arg.window = window_functions.rectangle;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.sine;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.hanning;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("win-",&arg);
  arg.window = window_functions.maxwell1;
  //graph_1chirp("win-",&arg);

  /* Graphs for .5, 1, 1.5, 2nd order ***************************************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.min_est_W=-3;
  arg.max_est_W=3;
  arg.max_chirp_W=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;
  arg.fit_W = 1;
  arg.fit_dA = 0;
  arg.fit_dW = 0;
  arg.fit_ddA = 0;
  arg.window = window_functions.hanning;

  arg.fit_nonlinear = 2;
  //graph_1chirp("order.5-",&arg);

  arg.fit_dA=1;

  arg.fit_nonlinear = 0;
  //graph_1chirp("order1-",&arg);

  arg.fit_nonlinear = 2;
  //graph_1chirp("order1-",&arg);

  arg.fit_dW=1;

  arg.fit_nonlinear = 2;
  //graph_1chirp("order1.5-",&arg);

  arg.fit_ddA=1;

  arg.fit_nonlinear = 0;
  //graph_1chirp("order2-",&arg);

  arg.fit_nonlinear = 2;
  //graph_1chirp("order2-",&arg);

  /* Symmetric norm *********************************************************/
  /* dW vs W ****************************************************************/
  init_arg(&arg);
  arg.min_chirp_dW = -2.5;
  arg.max_chirp_dW =  2.5;
  arg.max_chirp_W=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_CHIRP_dW;

  arg.fit_symm_norm = 1;

  arg.fit_nonlinear=0;
  //graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=1;
  //graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=2;
  //graph_1chirp("symmetric-",&arg);

  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.min_est_W=-2.5;
  arg.max_est_W=2.5;
  arg.max_chirp_W=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;

  arg.fit_symm_norm = 1;

  arg.fit_nonlinear=0;
  //graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=1;
  //graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=2;
  //graph_1chirp("symmetric-",&arg);

  /* W alpha ****************************************************************/
  /* estW vs alphaW *********************************************************/
  init_arg(&arg);
  arg.min_chirp_W = arg.max_chirp_W = rint(arg.blocksize/4);
  arg.fit_W_alpha_min = 0;
  arg.fit_W_alpha_max = 2.01612903225806451612;
  arg.min_est_W = -3;
  arg.max_est_W =  3;
  arg.x_dim = DIM_ALPHA_W;
  arg.y_dim = DIM_ESTIMATE_W;
  arg.x_minor=.0625;

  arg.fit_nonlinear = 2;

  arg.window = window_functions.rectangle;
  //graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.sine;
  //graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.hanning;
  //graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.maxwell1;
  //graph_1chirp("alphaW-",&arg);

  /* dW alpha ***************************************************************/
  /* estW vs alphadW ********************************************************/
  init_arg(&arg);
  arg.min_chirp_W = arg.max_chirp_W = rint(arg.blocksize/4);
  arg.fit_dW_alpha_min = 0;
  arg.fit_dW_alpha_max = 3.125;
  arg.min_est_W = -3;
  arg.max_est_W =  3;
  arg.x_dim = DIM_ALPHA_dW;
  arg.y_dim = DIM_ESTIMATE_W;
  arg.x_minor=.0625;
  arg.fit_nonlinear = 2;

  arg.graph_Aerror_max=0;
  arg.graph_Perror_max=0;
  arg.graph_Werror_max=0;
  arg.graph_dAerror_max=0;
  arg.graph_dWerror_max=0;
  arg.graph_ddAerror_max=0;
  arg.graph_RMSerror_max=0;
  arg.white_noise=fromdB(-80.);

  arg.window = window_functions.rectangle;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.sine;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.hanning;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.maxwell1;
  //graph_1chirp("alphadW-",&arg);

  arg.y_dim = DIM_CHIRP_dW;
  arg.min_est_W = 0;
  arg.max_est_W = 0;
  arg.min_chirp_dW = -3;
  arg.max_chirp_dW =  3;

  arg.window = window_functions.rectangle;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.sine;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.hanning;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.maxwell1;
  //graph_1chirp("alphadW-",&arg);

  /* replot algo fits with opt dW alpha *************************************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.min_est_W = -9.375;
  arg.max_est_W =  9.375;
  arg.min_chirp_W =  0;
  arg.max_chirp_W =  25;
  arg.x_dim = DIM_CHIRP_W;
  arg.y_dim = DIM_ESTIMATE_W;

  arg.fit_nonlinear = 2;

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.rectangle;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max=2.25;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.sine;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.711;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.hanning;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.618;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.5;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.maxwell1;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.554;
  //graph_1chirp("opt-",&arg);

  /* dW vs W **************************************************************/
  arg.min_chirp_dW = -9.375;
  arg.max_chirp_dW = 9.375;
  arg.min_est_W = 0;
  arg.max_est_W = 0;
  arg.y_dim = DIM_CHIRP_dW;

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.rectangle;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 2.25;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.sine;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.711;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.hanning;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.618;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.tgauss_deep;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.5;
  //graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.;
  arg.window = window_functions.maxwell1;
  //graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.554;
  //graph_1chirp("opt-",&arg);

  /* Noise graphs; replot algo***********************************************/
  /* Graphs for linear v. partial-nonlinear v. full-nonlinear ***************/

  {
    int noise;
    for(noise=-80;noise<0;noise+=20){
      int win;
      for(win=0;win<5;win++){
        char buf[80];

        /* dW vs W **********************************************************/
        init_arg(&arg);
        arg.max_chirp_W=10.;
        arg.min_chirp_dW=-2.5;
        arg.max_chirp_dW=2.5;
        arg.x_dim=DIM_CHIRP_W;
        arg.y_dim=DIM_CHIRP_dW;

        arg.graph_convergence_av=1;
        arg.graph_Aerror_av=1;
        arg.graph_Perror_av=1;
        arg.graph_Werror_av=1;
        arg.graph_dAerror_av=1;
        arg.graph_dWerror_av=1;
        arg.graph_ddAerror_av=1;
        arg.graph_RMSerror_av=1;

        arg.graph_Aerror_max=0;
        arg.graph_Perror_max=0;
        arg.graph_Werror_max=0;
        arg.graph_dAerror_max=0;
        arg.graph_dWerror_max=0;
        arg.graph_ddAerror_max=0;
        arg.graph_RMSerror_max=0;

        switch(win){
        case 0:
          arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 2.25;
          arg.window = window_functions.rectangle;
          break;
        case 1:
          arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.711;
          arg.window = window_functions.sine;
          break;
        case 2:
          arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.618;
          arg.window = window_functions.hanning;
          break;
        case 3:
          arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.5;
          arg.window = window_functions.tgauss_deep;
          break;
        case 4:
          arg.fit_dW_alpha_min = arg.fit_dW_alpha_max = 1.554;
          arg.window = window_functions.maxwell1;
          break;
        }

        arg.white_noise=fromdB(noise);
        snprintf(buf,80,"n%d-",-noise);
        arg.fit_nonlinear=0;
        graph_1chirp(buf,&arg);
        arg.fit_nonlinear=1;
        graph_1chirp(buf,&arg);
        arg.fit_nonlinear=2;
        graph_1chirp(buf,&arg);

        /* estW vs W ********************************************************/
        arg.min_chirp_dW=0;
        arg.max_chirp_dW=0;
        arg.max_chirp_W=10.;
        arg.min_est_W=-2.5;
        arg.max_est_W=2.5;
        arg.x_dim=DIM_CHIRP_W;
        arg.y_dim=DIM_ESTIMATE_W;

        arg.fit_nonlinear=0;
        graph_1chirp(buf,&arg);
        arg.fit_nonlinear=1;
        graph_1chirp(buf,&arg);
        arg.fit_nonlinear=2;
        graph_1chirp(buf,&arg);
      }
    }
  }

  /* Two chirp discrimination graphs ****************************************/


  return 0;
}

