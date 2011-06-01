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
  float A_0;
  float A_1;
  int   A_rel;

  float P_0;
  float P_1;
  int   P_rel;

  float W_0;
  float W_1;
  int   W_rel;

  float dA_0;
  float dA_1;
  int   dA_rel;

  float dW_0;
  float dW_1;
  int   dW_rel;

  float ddA_0;
  float ddA_1;
  int   ddA_rel;
} rel_param;

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

  float fit_W_alpha_0;
  float fit_W_alpha_1;
  float fit_dW_alpha_0;
  float fit_dW_alpha_1;

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

  rel_param est;   /* params may be relative to chirp */
  rel_param chirp; /* params may be relative to alt chirp */

  /* optionally add a second est/chirp */
  rel_param est_alt; /* params may be relative to alt chirp */
  rel_param chirp_alt; /* params may be relative to chirp */
  int alt_p;

  /* optionally add noise */
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
  int    alt_chirp_p;
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
               int blocksize,
               float xmin, float xmax,
               int xi, int xn, int xdim,
               float ymin, float ymax,
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
    A0 = xmin;
    A1 = xmax;
    An = xn-1;
    Ai = xi;
    break;
  case DIM_ESTIMATE_P:
    P0 = xmin;
    P1 = xmax;
    Pn = xn-1;
    Pi = xi;
    break;
  case DIM_ESTIMATE_W:
    W0 = xmin;
    W1 = xmax;
    Wn = xn-1;
    Wi = xi;
    break;
  case DIM_ESTIMATE_dA:
    dA0 = xmin;
    dA1 = xmax;
    dAn = xn-1;
    dAi = xi;
    break;
  case DIM_ESTIMATE_dW:
    dW0 = xmin;
    dW1 = xmax;
    dWn = xn-1;
    dWi = xi;
    break;
  case DIM_ESTIMATE_ddA:
    ddA0 = xmin;
    ddA1 = xmax;
    ddAn = xn-1;
    ddAi = xi;
    break;
  }

  switch(ydim){
  case DIM_ESTIMATE_A:
    A0 = ymin;
    A1 = ymax;
    An = yn-1;
    Ai = yi;
    break;
  case DIM_ESTIMATE_P:
    P0 = ymin;
    P1 = ymax;
    Pn = yn-1;
    Pi = yi;
    break;
  case DIM_ESTIMATE_W:
    W0 = ymin;
    W1 = ymax;
    Wn = yn-1;
    Wi = yi;
    break;
  case DIM_ESTIMATE_dA:
    dA0 = ymin;
    dA1 = ymax;
    dAn = yn-1;
    dAi = yi;
    break;
  case DIM_ESTIMATE_dW:
    dW0 = ymin;
    dW1 = ymax;
    dWn = yn-1;
    dWi = yi;
    break;
  case DIM_ESTIMATE_ddA:
    ddA0 = ymin;
    ddA1 = ymax;
    ddAn = yn-1;
    ddAi = yi;
    break;
  }

  P0*=2.*M_PI;
  P1*=2.*M_PI;
  W0*=2.*M_PI/blocksize;
  W1*=2.*M_PI/blocksize;
  dA0/=blocksize;
  dA1/=blocksize;
  dW0*=2.*M_PI/blocksize/blocksize;
  dW1*=2.*M_PI/blocksize/blocksize;
  ddA0/=blocksize/blocksize;
  ddA1/=blocksize/blocksize;

  c->A = fromdB(A0 + (A1-A0) / An * Ai);
  if(todB(c->A)<-120)c->A=0.;

  c->P = P0 + (P1-P0) / Pn * Pi;
  c->W = W0 + (W1-W0) / Wn * Wi;
  c->dA = dA0 + (dA1-dA0) / dAn * dAi;
  c->dW = dW0 + (dW1-dW0) / dWn * dWi;
  c->ddA = ddA0 + (ddA1-ddA0) / ddAn * ddAi;

}

float W_alpha(float A0, float A1,
              float xmin, float xmax,
              int xi, int xn, int xdim,
              float ymin, float ymax,
              int yi, int yn, int ydim,
              int stepi, int stepn, int rand_p){

  float Ai,An;

  if(stepn<2)stepn=2;
  An=stepn-1;
  Ai = (rand_p ? drand48()*An : stepi);

  if(xdim==DIM_ALPHA_W){
    A0 = xmin;
    A1 = xmax;
    An = xn-1;
    Ai = xi;
  }

  if(ydim==DIM_ALPHA_W){
    A0 = ymin;
    A1 = ymax;
    An = yn-1;
    Ai = yi;
  }

  return A0 + (A1-A0) / An * Ai;
}

float dW_alpha(float A0, float A1,
              float xmin, float xmax,
              int xi, int xn, int xdim,
              float ymin, float ymax,
              int yi, int yn, int ydim,
              int stepi, int stepn, int rand_p){

  float Ai,An;

  if(stepn<2)stepn=2;
  An=stepn-1;
  Ai = (rand_p ? drand48()*An : stepi);

  if(xdim==DIM_ALPHA_dW){
    A0 = xmin;
    A1 = xmax;
    An = xn-1;
    Ai = xi;
  }

  if(ydim==DIM_ALPHA_dW){
    A0 = ymin;
    A1 = ymax;
    An = yn-1;
    Ai = yi;
  }

  return A0 + (A1-A0) / An * Ai;
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
  float *cv = malloc(sizeof(*cv)*blocksize);
  int cimult=(arg->alt_chirp_p?2:1);
  int ym;

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
    ym=y*cimult;
    arg->estimate[ym].label=0;

    for(i=0;i<blocksize;i++){
      double jj = i - blocksize/2 + .5;
      double A = arg->chirp[ym].A + (arg->chirp[ym].dA + arg->chirp[ym].ddA*jj)*jj;
      double P = arg->chirp[ym].P + (arg->chirp[ym].W  + arg->chirp[ym].dW *jj)*jj;
      cv[i] = A*cos(P);
      energy_acc += cv[i]*cv[i]*arg->window[i]*arg->window[i];
    }
    if(arg->white_noise){
      for(i=0;i<blocksize;i++){
        float v = (drand48()-drand48())*2.45; /* (0dB RMS white noise) */
        cv[i]+=v*arg->white_noise;
      }
    }
    if(arg->alt_chirp_p){
      arg->estimate[ym+1].label=1;
      for(i=0;i<blocksize;i++){
        double jj = i - blocksize/2 + .5;
        double A = arg->chirp[ym+1].A + (arg->chirp[ym+1].dA + arg->chirp[ym+1].ddA*jj)*jj;
        double P = arg->chirp[ym+1].P + (arg->chirp[ym+1].W  + arg->chirp[ym+1].dW *jj)*jj;
        cv[i] += A*cos(P);
      }
    }

    ret=estimate_chirps(cv,arg->window,blocksize,
                        arg->estimate+ym,cimult,
                        arg->fit_tolerance,
                        arg->max_iterations,
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

    if(arg->alt_chirp_p && arg->estimate[ym].label==1){
      chirp tmp = arg->estimate[ym];
      arg->estimate[ym]=arg->estimate[ym+1];
      arg->estimate[ym+1]=tmp;
    }

    for(i=0;i<blocksize;i++){
      double jj = i - blocksize/2 + .5;
      double r=0;
      int j;
      for(j=ym;j<ym+cimult;j++){
        double A = arg->estimate[j].A + (arg->estimate[j].dA + arg->estimate[j].ddA*jj)*jj;
        double P = arg->estimate[j].P + (arg->estimate[j].W  + arg->estimate[j].dW *jj)*jj;
        r += A*cos(P);
      }
      float e = (cv[i]-r)*arg->window[i];
      error_acc += e*e;
    }
    arg->ssq_energy[y] = energy_acc;
    arg->ssq_error[y] = error_acc;
    arg->iterations[y] = arg->max_iterations-ret;
  }

  free(cv);
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

    if(arg->chirp.A_0<-120 && (arg->chirp.A_1<-120 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_A && arg->y_dim != DIM_CHIRP_A &&
       !arg->chirp.A_rel)
      zeroes++;
    if(arg->chirp.P_0==0 && (arg->chirp.P_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_P && arg->y_dim != DIM_CHIRP_P &&
       !arg->chirp.A_rel)
      zeroes++;
    if(arg->chirp.W_0==0 && (arg->chirp.W_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_W && arg->y_dim != DIM_CHIRP_W &&
       !arg->chirp.W_rel)
      zeroes++;
    if(arg->chirp.dA_0==0 && (arg->chirp.dA_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_dA && arg->y_dim != DIM_CHIRP_dA &&
       !arg->chirp.dA_rel)
      zeroes++;
    if(arg->chirp.dW_0==0 && (arg->chirp.dW_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_dW && arg->y_dim != DIM_CHIRP_dW &&
       !arg->chirp.dW_rel)
      zeroes++;
    if(arg->chirp.ddA_0==0 && (arg->chirp.ddA_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_CHIRP_ddA && arg->y_dim != DIM_CHIRP_ddA &&
       !arg->chirp.ddA_rel)
      zeroes++;

    if(arg->chirp.A_0==arg->chirp.A_1 || arg->sweep_steps<2){
      if(arg->chirp.A_0<-120 && arg->chirp.A_rel){
        strcat(subtitle2,"A=reference A");
        expl++;
      }else
        if(arg->chirp.A_0>=-120 || zeroes<2){
          if(arg->x_dim != DIM_CHIRP_A && arg->y_dim != DIM_CHIRP_A){
            snprintf(buf,80,"A=%.0fdB",arg->chirp.A_0);
            strcat(subtitle2,buf);
            if(arg->chirp.A_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->chirp.P_0==arg->chirp.P_1 || arg->sweep_steps<2){
      if(arg->chirp.P_0==0 && arg->chirp.P_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"P=reference P");
        expl++;
      }else
        if(arg->chirp.P_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_CHIRP_P && arg->y_dim != DIM_CHIRP_P){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"P=%.1f",arg->chirp.P_0);
            strcat(subtitle2,buf);
            if(arg->chirp.P_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->chirp.W_0==arg->chirp.W_1 || arg->sweep_steps<2){
      if(arg->chirp.W_0==0 && arg->chirp.W_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"W=reference W");
        expl++;
      }else
        if(arg->chirp.W_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_CHIRP_W && arg->y_dim != DIM_CHIRP_W){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"W=Nyquist/%.0f",(arg->blocksize/2)/arg->chirp.W_0);
            strcat(subtitle2,buf);
            if(arg->chirp.W_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->chirp.dA_0==arg->chirp.dA_1 || arg->sweep_steps<2){
      if(arg->chirp.dA_0==0 && arg->chirp.dA_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"dA=reference dA");
        expl++;
      }else
        if(arg->chirp.dA_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_CHIRP_dA && arg->y_dim != DIM_CHIRP_dA){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"dA=%.1f",arg->chirp.dA_0);
            strcat(subtitle2,buf);
            if(arg->chirp.dA_rel)strcat(subtitle2,"(relative)");
          expl++;
          }
        }
    }

    if(arg->chirp.dW_0==arg->chirp.dW_1 || arg->sweep_steps<2){
      if(arg->chirp.dW_0==0 && arg->chirp.dW_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"dW=reference dW");
        expl++;
      }else
        if(arg->chirp.dW_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_CHIRP_dW && arg->y_dim != DIM_CHIRP_dW){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"dW=%.1f",arg->chirp.dW_0);
            strcat(subtitle2,buf);
            if(arg->chirp.dW_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->chirp.ddA_0==arg->chirp.ddA_1 || arg->sweep_steps<2){
      if(arg->chirp.ddA_0==0 && arg->chirp.ddA_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"ddA=reference ddA");
        expl++;
      }else
        if(arg->chirp.ddA_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_CHIRP_ddA && arg->y_dim != DIM_CHIRP_ddA){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"ddA=%.1f",arg->chirp.ddA_0);
            strcat(subtitle2,buf);
            if(arg->chirp.ddA_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(expl && zeroes>1)
      strcat(subtitle2,", ");

    if(arg->chirp.A_0<-120 && (arg->chirp.A_1<-120 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_A &&
       arg->y_dim != DIM_CHIRP_A && !arg->chirp.A_rel)
      strcat(subtitle2,"A=");
    if(arg->chirp.P_0==0 && (arg->chirp.P_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_P &&
       arg->y_dim != DIM_CHIRP_P && !arg->chirp.P_rel)
      strcat(subtitle2,"P=");
    if(arg->chirp.W_0==0 && (arg->chirp.W_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_W &&
       arg->y_dim != DIM_CHIRP_W && !arg->chirp.W_rel)
      strcat(subtitle2,"W=");
    if(arg->chirp.dA_0==0 && (arg->chirp.dA_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_dA &&
       arg->y_dim != DIM_CHIRP_dA && !arg->chirp.dA_rel)
      strcat(subtitle2,"dA=");
    if(arg->chirp.dW_0==0 && (arg->chirp.dW_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_dW &&
       arg->y_dim != DIM_CHIRP_dW && !arg->chirp.dW_rel)
      strcat(subtitle2,"dW=");
    if(arg->chirp.ddA_0==0 && (arg->chirp.ddA_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_CHIRP_ddA &&
       arg->y_dim != DIM_CHIRP_ddA && !arg->chirp.ddA_rel)
      strcat(subtitle2,"ddA=");
    if(zeroes>1)
      strcat(subtitle2,"0");

    {
      char buf[320];
      buf[0]=0;
      if(arg->chirp.A_0!=arg->chirp.A_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_A && arg->y_dim!=DIM_CHIRP_A){
        strcat(buf,"A");
        if(arg->chirp.A_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->chirp.P_0!=arg->chirp.P_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_P && arg->y_dim!=DIM_CHIRP_P){
        if(swept)strcat(buf,",");
        strcat(buf,"P");
        if(arg->chirp.P_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->chirp.W_0!=arg->chirp.W_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_W && arg->y_dim!=DIM_CHIRP_W){
        if(swept)strcat(buf,",");
        strcat(buf,"W");
        if(arg->chirp.W_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->chirp.dA_0!=arg->chirp.dA_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_dA && arg->y_dim!=DIM_CHIRP_dA){
        if(swept)strcat(buf,",");
        strcat(buf,"dA");
        if(arg->chirp.dA_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->chirp.dW_0!=arg->chirp.dW_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_dW && arg->y_dim!=DIM_CHIRP_dW){
        if(swept)strcat(buf,",");
        strcat(buf,"dW");
        if(arg->chirp.dW_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->chirp.ddA_0!=arg->chirp.ddA_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_CHIRP_ddA && arg->y_dim!=DIM_CHIRP_ddA){
        if(swept)strcat(buf,",");
        strcat(buf,"ddA");
        if(arg->chirp.ddA_rel)strcat(buf,"(relative)");
        swept++;
      }

      if(swept){
        if(expl || zeroes>1)
          strcat(subtitle2,", ");

        if(arg->sweep_or_rand_p)
          strcat(subtitle2,"randomized ");
        else
          strcat(subtitle2,"swept ");
        strcat(subtitle2,buf);
      }
    }

    strcat(subtitle2,"] estimate:[");
    zeroes=0;
    expl=0;
    swept=0;
    if(arg->est.A_0<-120 && (arg->est.A_1<-120 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_A && arg->y_dim != DIM_ESTIMATE_A &&
       !arg->est.A_rel)
      zeroes++;
    if(arg->est.P_0==0 && (arg->est.P_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_P && arg->y_dim != DIM_ESTIMATE_P &&
       !arg->est.P_rel)
      zeroes++;
    if(arg->est.W_0==0 && (arg->est.W_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_W && arg->y_dim != DIM_ESTIMATE_W &&
       !arg->est.W_rel)
      zeroes++;
    if(arg->est.dA_0==0 && (arg->est.dA_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_dA && arg->y_dim != DIM_ESTIMATE_dA &&
       !arg->est.dA_rel)
      zeroes++;
    if(arg->est.dW_0==0 && (arg->est.dW_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_dW && arg->y_dim != DIM_ESTIMATE_dW &&
       !arg->est.dW_rel)
      zeroes++;
    if(arg->est.ddA_0==0 && (arg->est.ddA_1==0 || arg->sweep_steps<2) &&
       arg->x_dim != DIM_ESTIMATE_ddA && arg->y_dim != DIM_ESTIMATE_ddA &&
       !arg->est.ddA_rel)
      zeroes++;

    if(arg->est.A_0==arg->est.A_1 || arg->sweep_steps<2){
      if(arg->est.A_0<-120 && arg->est.A_rel){
        strcat(subtitle2,"A=chirp A");
        expl++;
      }else
        if(arg->est.A_0>=-120 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_A && arg->y_dim != DIM_ESTIMATE_A){
            snprintf(buf,80,"A=%.0fdB",arg->est.A_0);
            strcat(subtitle2,buf);
            if(arg->est.A_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->est.P_0==arg->est.P_1 || arg->sweep_steps<2){
      if(arg->est.P_0==0 && arg->est.P_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"P=chirp P");
        expl++;
      }else
        if(arg->est.P_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_P && arg->y_dim != DIM_ESTIMATE_P){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"P=%.1f",arg->est.P_0);
            strcat(subtitle2,buf);
            if(arg->est.P_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->est.W_0==arg->est.W_1 || arg->sweep_steps<2){
      if(arg->est.W_0==0 && arg->est.W_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"W=chirp W");
        expl++;
      }else
        if(arg->est.W_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_W && arg->y_dim != DIM_ESTIMATE_W){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"W=Nyquist/%.0f",(arg->blocksize/2)/arg->est.W_0);
            strcat(subtitle2,buf);
            if(arg->est.W_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->est.dA_0==arg->est.dA_1 || arg->sweep_steps<2){
      if(arg->est.dA_0==0 && arg->est.dA_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"dA=chirp dA");
        expl++;
      }else
        if(arg->est.dA_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_dA && arg->y_dim != DIM_ESTIMATE_dA){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"dA=%.1f",arg->est.dA_0);
            strcat(subtitle2,buf);
            if(arg->est.dA_rel)strcat(subtitle2,"(relative)");
          expl++;
          }
        }
    }

    if(arg->est.dW_0==arg->est.dW_1 || arg->sweep_steps<2){
      if(arg->est.dW_0==0 && arg->est.dW_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"dW=chirp dW");
        expl++;
      }else
        if(arg->est.dW_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_dW && arg->y_dim != DIM_ESTIMATE_dW){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"dW=%.1f",arg->est.dW_0);
            strcat(subtitle2,buf);
            if(arg->est.dW_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(arg->est.ddA_0==arg->est.ddA_1 || arg->sweep_steps<2){
      if(arg->est.ddA_0==0 && arg->est.ddA_rel){
        if(expl)strcat(subtitle2,", ");
        strcat(subtitle2,"ddA=chirp ddA");
        expl++;
      }else
        if(arg->est.ddA_0!=0 || zeroes<2){
          if(arg->x_dim != DIM_ESTIMATE_ddA && arg->y_dim != DIM_ESTIMATE_ddA){
            if(expl)strcat(subtitle2,", ");
            snprintf(buf,80,"ddA=%.1f",arg->est.ddA_0);
            strcat(subtitle2,buf);
            if(arg->est.ddA_rel)strcat(subtitle2,"(relative)");
            expl++;
          }
        }
    }

    if(expl && zeroes>1)
      strcat(subtitle2,", ");

    if(arg->est.A_0<-120 && (arg->est.A_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_A &&
       arg->y_dim != DIM_ESTIMATE_A && !arg->est.A_rel)
      strcat(subtitle2,"A=");
    if(arg->est.P_0==0 && (arg->est.P_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_P &&
       arg->y_dim != DIM_ESTIMATE_P && !arg->est.P_rel)
      strcat(subtitle2,"P=");
    if(arg->est.W_0==0 && (arg->est.W_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_W &&
       arg->y_dim != DIM_ESTIMATE_W && !arg->est.W_rel)
      strcat(subtitle2,"W=");
    if(arg->est.dA_0==0 && (arg->est.dA_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_dA &&
       arg->y_dim != DIM_ESTIMATE_dA && !arg->est.dA_rel)
      strcat(subtitle2,"dA=");
    if(arg->est.dW_0==0 && (arg->est.dW_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_dW &&
       arg->y_dim != DIM_ESTIMATE_dW && !arg->est.dW_rel)
      strcat(subtitle2,"dW=");
    if(arg->est.ddA_0==0 && (arg->est.ddA_1==0 || arg->sweep_steps<2) &&
       zeroes>1 && arg->x_dim != DIM_ESTIMATE_ddA &&
       arg->y_dim != DIM_ESTIMATE_ddA && !arg->est.ddA_rel)
      strcat(subtitle2,"ddA=");
    if(zeroes>1)
      strcat(subtitle2,"0");

    {
      char buf[320];
      buf[0]=0;
      if(arg->est.A_0!=arg->est.A_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_A && arg->y_dim!=DIM_ESTIMATE_A){
        strcat(buf,"A");
        if(arg->est.A_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->est.P_0!=arg->est.P_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_P && arg->y_dim!=DIM_ESTIMATE_P){
        if(swept)strcat(buf,",");
        strcat(buf,"P");
        if(arg->est.P_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->est.W_0!=arg->est.W_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_W && arg->y_dim!=DIM_ESTIMATE_W){
        if(swept)strcat(buf,",");
        strcat(buf,"W");
        if(arg->est.W_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->est.dA_0!=arg->est.dA_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_dA && arg->y_dim!=DIM_ESTIMATE_dA){
        if(swept)strcat(buf,",");
        strcat(buf,"dA");
        if(arg->est.dA_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->est.dW_0!=arg->est.dW_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_dW && arg->y_dim!=DIM_ESTIMATE_dW){
        if(swept)strcat(buf,",");
        strcat(buf,"dW");
        if(arg->est.dW_rel)strcat(buf,"(relative)");
        swept++;
      }
      if(arg->est.ddA_0!=arg->est.ddA_1 && arg->sweep_steps>1 &&
         arg->x_dim!=DIM_ESTIMATE_ddA && arg->y_dim!=DIM_ESTIMATE_ddA){
        if(swept)strcat(buf,",");
        strcat(buf,"ddA");
        if(arg->est.ddA_rel)strcat(buf,"(relative)");
        swept++;
      }

      if(swept){
        if(expl || zeroes>1)
          strcat(subtitle2,", ");

        if(arg->sweep_or_rand_p)
          strcat(subtitle2,"randomized ");
        else
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
      if(arg->fit_W_alpha_0==arg->fit_W_alpha_1 || arg->sweep_steps<2){
        if(arg->fit_W_alpha_0!=1.0){
          snprintf(buf,80,", alpha_W=%.2f",arg->fit_W_alpha_0);
          strcat(subtitle3,buf);
        }
      }
      if(arg->fit_W_alpha_0!=arg->fit_W_alpha_1 && arg->sweep_steps>1){
        snprintf(buf,80,", swept alpha_W");
        strcat(subtitle3,buf);
      }
    }

    if(arg->x_dim != DIM_ALPHA_dW && arg->y_dim != DIM_ALPHA_dW){
      if(arg->fit_dW_alpha_0==arg->fit_dW_alpha_1 || arg->sweep_steps<2){
        if(arg->fit_dW_alpha_0!=1.0){
          snprintf(buf,80,", alpha_dW=%.3f",arg->fit_dW_alpha_0);
          strcat(subtitle3,buf);
        }
      }
      if(arg->fit_dW_alpha_0!=arg->fit_dW_alpha_1 && arg->sweep_steps>1){
        snprintf(buf,80,", swept alpha_dW");
        strcat(subtitle3,buf);
      }
    }

    {
      snprintf(buf,80,", blocksize=%d",arg->blocksize);
      strcat(subtitle3,buf);
    }

    arg->subtitle3=subtitle3;
  }

  if(!arg->xaxis_label){
    switch(arg->x_dim){
    case DIM_ESTIMATE_A:
      if(arg->est.A_rel)
        arg->xaxis_label="initial estimate distance from A (dB)";
      else
        arg->xaxis_label="initial estimated A (dB)";
      break;
    case DIM_ESTIMATE_P:
      if(arg->est.P_rel)
        arg->xaxis_label="initial estimate distance from P (radians)";
      else
        arg->xaxis_label="initial estimated P (radians)";
      break;
    case DIM_ESTIMATE_W:
      if(arg->est.W_rel)
        arg->xaxis_label="initial estimate distance from W (cycles/block)";
      else
        arg->xaxis_label="initial estimated W (cycles/block)";
      break;
    case DIM_ESTIMATE_dA:
      if(arg->est.dA_rel)
        arg->xaxis_label="initial estimate distance from dA";
      else
        arg->xaxis_label="initial estimated dA";
      break;
    case DIM_ESTIMATE_dW:
      if(arg->est.dW_rel)
        arg->xaxis_label="initial estimate distance from dW (cycles/block)";
      else
        arg->xaxis_label="initial estimated dW (cycles/block)";
      break;
    case DIM_ESTIMATE_ddA:
      if(arg->est.ddA_rel)
        arg->xaxis_label="initial estimate distance from ddA";
      else
        arg->xaxis_label="initial estimated ddA";
      break;
    case DIM_CHIRP_A:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference A (dB)";
      else
        arg->xaxis_label="A (dB)";
      break;
    case DIM_CHIRP_P:
      if(arg->chirp.P_rel)
        arg->xaxis_label="chirp distance from reference P (radians)";
      else
        arg->xaxis_label="P (radians)";
      break;
    case DIM_CHIRP_W:
      if(arg->chirp.W_rel)
        arg->xaxis_label="chirp distance from reference W (cycles/block)";
      else
        arg->xaxis_label="W (cycles/block)";
      break;
    case DIM_CHIRP_dA:
      if(arg->chirp.dA_rel)
        arg->xaxis_label="chirp distance from reference dA";
      else
        arg->xaxis_label="dA";
      break;
    case DIM_CHIRP_dW:
      if(arg->chirp.dW_rel)
        arg->xaxis_label="chirp distance from reference dW (cycles/block)";
      else
        arg->xaxis_label="dW (cycles/block)";
      break;
    case DIM_CHIRP_ddA:
      if(arg->chirp.ddA_rel)
        arg->xaxis_label="chirp distance from reference ddA";
      else
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
      if(arg->est.A_rel)
        arg->yaxis_label="initial estimate distance from A (dB)";
      else
        arg->yaxis_label="initial estimated A (dB)";
      break;
    case DIM_ESTIMATE_P:
      if(arg->est.P_rel)
        arg->yaxis_label="initial estimate distance from P (radians)";
      else
        arg->yaxis_label="initial estimated P (radians)";
      break;
    case DIM_ESTIMATE_W:
      if(arg->est.W_rel)
        arg->yaxis_label="initial estimate distance from W (cycles/block)";
      else
        arg->yaxis_label="initial estimated W (cycles/block)";
      break;
    case DIM_ESTIMATE_dA:
      if(arg->est.dA_rel)
        arg->yaxis_label="initial estimate distance from dA";
      else
        arg->yaxis_label="initial estimated dA";
      break;
    case DIM_ESTIMATE_dW:
      if(arg->est.dW_rel)
        arg->yaxis_label="initial estimate distance from dW (cycles/block)";
      else
        arg->yaxis_label="initial estimated dW (cycles/block)";
      break;
    case DIM_ESTIMATE_ddA:
      if(arg->est.ddA_rel)
        arg->yaxis_label="initial estimate distance from ddA";
      else
        arg->yaxis_label="initial estimated ddA";
      break;
    case DIM_CHIRP_A:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference A (dB)";
      else
        arg->yaxis_label="A (dB)";
      break;
    case DIM_CHIRP_P:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference P (radians)";
      else
        arg->yaxis_label="P (radians)";
      break;
    case DIM_CHIRP_W:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference W (cycles/block)";
      else
        arg->yaxis_label="W (cycles/block)";
      break;
    case DIM_CHIRP_dA:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference dA";
      else
        arg->yaxis_label="dA";
      break;
    case DIM_CHIRP_dW:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference dW (cycles/block)";
      else
        arg->yaxis_label="dW (cycles/block)";
      break;
    case DIM_CHIRP_ddA:
      if(arg->chirp.A_rel)
        arg->xaxis_label="chirp distance from reference ddA";
      else
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
  int xdB=0;
  int ydB=0;

  char *filebase;
  int except;
  struct timeval last;

  except=fegetexcept();
  fedisableexcept(FE_INEXACT);
  fedisableexcept(FE_UNDERFLOW);
  feenableexcept(FE_ALL_EXCEPT & ~(FE_INEXACT|FE_UNDERFLOW));

  gettimeofday(&last,NULL);
  setup_titles_1chirp(arg);

  filebase=calloc(strlen(filepre)+strlen(arg->filebase?arg->filebase:"")+1,
                  sizeof(*filebase));
  strcat(filebase,filepre);
  strcat(filebase,arg->filebase);

  switch(arg->x_dim){
  case DIM_ESTIMATE_A:
    minX = arg->est.A_0;
    maxX = arg->est.A_1;
    xdB = 1;
    break;
  case DIM_ESTIMATE_P:
    minX = arg->est.P_0;
    maxX = arg->est.P_1;
    break;
  case DIM_ESTIMATE_W:
    minX = arg->est.W_0;
    maxX = arg->est.W_1;
    break;
  case DIM_ESTIMATE_dA:
    minX = arg->est.dA_0;
    maxX = arg->est.dA_1;
    break;
  case DIM_ESTIMATE_dW:
    minX = arg->est.dW_0;
    maxX = arg->est.dW_1;
    break;
  case DIM_ESTIMATE_ddA:
    minX = arg->est.ddA_0;
    maxX = arg->est.ddA_1;
    break;
  case DIM_CHIRP_A:
    minX = arg->chirp.A_0;
    maxX = arg->chirp.A_1;
    xdB = 1;
    break;
  case DIM_CHIRP_P:
    minX = arg->chirp.P_0;
    maxX = arg->chirp.P_1;
    break;
  case DIM_CHIRP_W:
    minX = arg->chirp.W_0;
    maxX = arg->chirp.W_1;
    break;
  case DIM_CHIRP_dA:
    minX = arg->chirp.dA_0;
    maxX = arg->chirp.dA_1;
    break;
  case DIM_CHIRP_dW:
    minX = arg->chirp.dW_0;
    maxX = arg->chirp.dW_1;
    break;
  case DIM_CHIRP_ddA:
    minX = arg->chirp.ddA_0;
    maxX = arg->chirp.ddA_1;
    break;
  case DIM_ALPHA_W:
    minX = arg->fit_W_alpha_0;
    maxX = arg->fit_W_alpha_1;
    break;
  case DIM_ALPHA_dW:
    minX = arg->fit_dW_alpha_0;
    maxX = arg->fit_dW_alpha_1;
    break;
  }

  switch(arg->y_dim){
  case DIM_ESTIMATE_A:
    minY = arg->est.A_0;
    maxY = arg->est.A_1;
    ydB = 1;
    break;
  case DIM_ESTIMATE_P:
    minY = arg->est.P_0;
    maxY = arg->est.P_1;
    break;
  case DIM_ESTIMATE_W:
    minY = arg->est.W_0;
    maxY = arg->est.W_1;
    break;
  case DIM_ESTIMATE_dA:
    minY = arg->est.dA_0;
    maxY = arg->est.dA_1;
    break;
  case DIM_ESTIMATE_dW:
    minY = arg->est.dW_0;
    maxY = arg->est.dW_1;
    break;
  case DIM_ESTIMATE_ddA:
    minY = arg->est.ddA_0;
    maxY = arg->est.ddA_1;
    break;
  case DIM_CHIRP_A:
    minY = arg->chirp.A_0;
    maxY = arg->chirp.A_1;
    ydB = 1;
    break;
  case DIM_CHIRP_P:
    minY = arg->chirp.P_0;
    maxY = arg->chirp.P_1;
    break;
  case DIM_CHIRP_W:
    minY = arg->chirp.W_0;
    maxY = arg->chirp.W_1;
    break;
  case DIM_CHIRP_dA:
    minY = arg->chirp.dA_0;
    maxY = arg->chirp.dA_1;
    break;
  case DIM_CHIRP_dW:
    minY = arg->chirp.dW_0;
    maxY = arg->chirp.dW_1;
    break;
  case DIM_CHIRP_ddA:
    minY = arg->chirp.ddA_0;
    maxY = arg->chirp.ddA_1;
    break;
  case DIM_ALPHA_W:
    minY = arg->fit_W_alpha_0;
    maxY = arg->fit_W_alpha_1;
    break;
  case DIM_ALPHA_dW:
    minY = arg->fit_dW_alpha_0;
    maxY = arg->fit_dW_alpha_1;
    break;
  }

  x0s = rint((x_n-1)/fabsf(maxX-minX)*minX);
  y0s = rint((y_n-1)/fabsf(maxY-minY)*minY);
  x1s = rint((x_n-1)/fabsf(maxX-minX)*maxX);
  y1s = rint((y_n-1)/fabsf(maxY-minY)*maxY);

  xminori = rint((x_n-1)/fabsf(maxX-minX)*arg->x_minor);
  yminori = rint((y_n-1)/fabsf(maxY-minY)*arg->y_minor);
  xmajori = rint(xminori/arg->x_minor*arg->x_major);
  ymajori = rint(yminori/arg->y_minor*arg->y_major);

  if(xminori<1 || yminori<1 || xmajori<1 || ymajori<1){
    fprintf(stderr,"Bad xmajor/xminor/ymajor/yminor value.\n");
    exit(1);
  }

  if( rint(xmajori*fabsf(maxX-minX)/arg->x_major) != x_n-1){
    float adj = (x_n-1.)/xmajori - fabsf(maxX-minX);
    if(minX==0){
      if(minX<maxX){
        maxX+=adj;
      }else{
        maxX-=adj;
      }
    }else if(maxX==0){
      if(minX<maxX){
        minX-=adj;
      }else{
        minX+=adj;
      }
    }else{
      if(minX<maxX){
        minX-=adj/2;
        maxX+=adj/2;
      }else{
        minX+=adj/2;
        maxX-=adj/2;
      }
    }
  }

  if( rint(ymajori*fabsf(maxY-minY)/arg->y_major) != y_n-1){
    float adj = (y_n-1.)/ymajori - fabsf(maxY-minY);
    if(minY==0){
      if(minY<maxY){
        maxY+=adj;
      }else{
        maxY-=adj;
      }
    }else if(maxY==0){
      if(minY<maxY){
        minY-=adj;
      }else{
        minY+=adj;
      }
    }else{
      if(minY<maxY){
        minY-=adj/2;
        maxY+=adj/2;
      }else{
        minY+=adj/2;
        maxY-=adj/2;
      }
    }
  }

  /* determine ~ padding needed */
  setup_graphs(MIN(x0s,x1s),MAX(x0s,x1s),xmajori,arg->x_major,
               MIN(y0s,y1s),MAX(y0s,y1s),ymajori,arg->y_major,
               (arg->subtitle1!=0)+(arg->subtitle2!=0)+(arg->subtitle3!=0),
               arg->fontsize);

  if(arg->sweep_steps>1){
    if(!(arg->x_dim==DIM_ESTIMATE_A || arg->y_dim==DIM_ESTIMATE_A) &&
       arg->est.A_0 != arg->est.A_1) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_P || arg->y_dim==DIM_ESTIMATE_P) &&
       arg->est.P_0 != arg->est.P_1) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_W || arg->y_dim==DIM_ESTIMATE_W) &&
       arg->est.W_0 != arg->est.W_1) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_dA || arg->y_dim==DIM_ESTIMATE_dA) &&
       arg->est.dA_0 != arg->est.dA_1) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_dW || arg->y_dim==DIM_ESTIMATE_dW) &&
       arg->est.dW_0 != arg->est.dW_1) est_swept=1;
    if(!(arg->x_dim==DIM_ESTIMATE_ddA || arg->y_dim==DIM_ESTIMATE_ddA) &&
       arg->est.ddA_0 != arg->est.ddA_1) est_swept=1;

    if(!(arg->x_dim==DIM_CHIRP_A || arg->y_dim==DIM_CHIRP_A) &&
       arg->chirp.A_0 != arg->chirp.A_1) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_P || arg->y_dim==DIM_CHIRP_P) &&
       arg->chirp.P_0 != arg->chirp.P_1) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_W || arg->y_dim==DIM_CHIRP_W) &&
       arg->chirp.W_0 != arg->chirp.W_1) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_dA || arg->y_dim==DIM_CHIRP_dA) &&
       arg->chirp.dA_0 != arg->chirp.dA_1) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_dW || arg->y_dim==DIM_CHIRP_dW) &&
       arg->chirp.dW_0 != arg->chirp.dW_1) chirp_swept=1;
    if(!(arg->x_dim==DIM_CHIRP_ddA || arg->y_dim==DIM_CHIRP_ddA) &&
       arg->chirp.ddA_0 != arg->chirp.ddA_1) chirp_swept=1;

    if(!(arg->x_dim==DIM_ALPHA_W || arg->y_dim==DIM_ALPHA_W) &&
       arg->fit_W_alpha_0 != arg->fit_W_alpha_1) fit_swept=1;
    if(!(arg->x_dim==DIM_ALPHA_dW || arg->y_dim==DIM_ALPHA_dW) &&
       arg->fit_dW_alpha_0 != arg->fit_dW_alpha_1) fit_swept=1;
  }

  swept = est_swept | chirp_swept | fit_swept;

  if(arg->graph_convergence_av)
    cC_m = draw_page(!swept?"Convergence":"Average Convergence",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Iterations:",
                     DT_iterations,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
  if(swept && arg->graph_convergence_max)
    cC_w = draw_page("Worst Case Convergence",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Iterations:",
                     DT_iterations,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
  if(swept && arg->graph_convergence_delta)
    cC_d = draw_page("Convergence Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Iteration span:",
                     DT_iterations,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(arg->graph_Aerror_av)
    cA_m = draw_page(!swept?"A (Amplitude) Error":"A (Amplitude) Mean Squared Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percent Error:",
                     DT_percent,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
  if(swept && arg->graph_Aerror_max)
    cA_w = draw_page("A (Amplitude) Worst Case Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percent Error:",
                     DT_percent,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
  if(swept && arg->graph_Aerror_delta)
    cA_d = draw_page("A (Amplitude) Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Percent Error Delta:",
                     DT_percent,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(arg->graph_Perror_av)
    cP_m = draw_page(!swept?"P (Phase) Error":"P (Phase) Mean Squared Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Error (radians):",
                     DT_abserror,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(swept && arg->graph_Perror_max)
    cP_w = draw_page("P (Phase) Worst Case Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Error (radians):",
                     DT_abserror,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(swept && arg->graph_Perror_delta)
    cP_d = draw_page("Phase Delta",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Delta (radians):",
                     DT_abserror,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

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
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_Werror_max)
      cW_w = draw_page("W (Frequency) Worst Case Error",
                     arg->subtitle1,
                     arg->subtitle2,
                     arg->subtitle3,
                     arg->xaxis_label,
                     arg->yaxis_label,
                     "Error (cycles/block):",
                     DT_abserror,
                     (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                     (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_Werror_delta)
      cW_d = draw_page("Frequency Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Delta (cycles/block):",
                       DT_abserror,
                       (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                       (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
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
                        (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                        (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_dAerror_max)
      cdA_w = draw_page("dA (Amplitude Modulation) Worst Case Error",
                      arg->subtitle1,
                      arg->subtitle2,
                      arg->subtitle3,
                      arg->xaxis_label,
                      arg->yaxis_label,
                      "Error:",
                      DT_abserror,
                        (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                        (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_dAerror_delta)
      cdA_d = draw_page("Amplitude Modulation Delta",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        arg->xaxis_label,
                        arg->yaxis_label,
                        "Delta:",
                        DT_abserror,
                        (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                        (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
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
                        (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                        (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_dWerror_max)
      cdW_w = draw_page("dW (Chirp Rate) Worst Case Error",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        arg->xaxis_label,
                        arg->yaxis_label,
                        "Error (cycles/block):",
                        DT_abserror,
                        (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                        (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_dWerror_delta)
      cdW_d = draw_page("Chirp Rate Delta",
                        arg->subtitle1,
                        arg->subtitle2,
                        arg->subtitle3,
                        arg->xaxis_label,
                        arg->yaxis_label,
                        "Delta (cycles/block):",
                        DT_abserror,
                        (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                        (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
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
                         (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                         (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_ddAerror_max)
      cddA_w = draw_page("ddA (Amplitude Modulation Squared) Worst Case Error",
                         arg->subtitle1,
                         arg->subtitle2,
                         arg->subtitle3,
                         arg->xaxis_label,
                         arg->yaxis_label,
                         "Error:",
                         DT_abserror,
                         (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                         (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

    if(swept && arg->graph_ddAerror_delta)
      cddA_d = draw_page("Amplitude Modulation Squared Delta",
                         arg->subtitle1,
                         arg->subtitle2,
                         arg->subtitle3,
                         arg->xaxis_label,
                         arg->yaxis_label,
                         "Delta:",
                         DT_abserror,
                         (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                         (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));
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
                       (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                       (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(swept && arg->graph_RMSerror_max)
    cRMS_w = draw_page("RMS Worst Case Fit Error",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Percentage Error:",
                       DT_percent,
                       (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                       (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(swept && arg->graph_RMSerror_delta)
    cRMS_d = draw_page("RMS Fit Error Delta",
                       arg->subtitle1,
                       arg->subtitle2,
                       arg->subtitle3,
                       arg->xaxis_label,
                       arg->yaxis_label,
                       "Percentage Error:",
                       DT_percent,
                       (arg->x_dim==DIM_CHIRP_W && !arg->chirp.W_rel)||
                       (arg->x_dim==DIM_ESTIMATE_W && !arg->est.W_rel));

  if(arg->window)
    arg->window(window,blocksize);
  else
    for(i=0;i<blocksize;i++)
      window[i]=1.;

  /* graph computation */
  for(xi=0;xi<x_n;xi++){
    chirp chirps[y_n*2];
    chirp estimates[y_n*2];
    colvec fitvec[y_n];
    int iter[y_n];
    float error[y_n];
    float energy[y_n];
    int si,sn=(swept && arg->sweep_steps>1 ? arg->sweep_steps : 1);

    fprintf(stderr,"\r%s: column %d/%d...",filebase,xi,x_n-1);

    memset(targ,0,sizeof(targ));

    for(i=0;i<threads;i++){
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
      targ[i].alt_chirp_p=arg->alt_p;
    }

    /* if we're sweeping a parameter, we're going to iterate here for a bit. */
    for(si=0;si<sn;si++){
      max_y=y_n;
      next_y=0;

      /* compute/set chirp and est parameters, potentially compute the
         chirp waveform */
      for(yi=0;yi<y_n;yi++){
        int ym = yi*(arg->alt_p?2:1);

        fitvec[yi].fit_W_alpha = W_alpha(arg->fit_W_alpha_0,
                                         arg->fit_W_alpha_1,
                                         minX,maxX,
                                         xi,x_n,arg->x_dim,
                                         minY,maxY,
                                         y_n-yi-1,y_n,arg->y_dim,
                                         si,sn,
                                         arg->sweep_or_rand_p);
        fitvec[yi].fit_dW_alpha = dW_alpha(arg->fit_dW_alpha_0,
                                           arg->fit_dW_alpha_1,
                                           minX,maxX,
                                           xi,x_n,arg->x_dim,
                                           minY,maxY,
                                           y_n-yi-1,y_n,arg->y_dim,
                                           si,sn,
                                           arg->sweep_or_rand_p);

        set_chirp(chirps+ym,
                  blocksize,
                  minX,maxX,
                  xi,x_n,
                  arg->x_dim&DIM_CHIRP_MASK,
                  minY,maxY,
                  y_n-yi-1,y_n,
                  arg->y_dim&DIM_CHIRP_MASK,
                  si,sn,
                  arg->sweep_or_rand_p,
                  arg->chirp.A_0,
                  arg->chirp.A_1,
                  arg->chirp.P_0,
                  arg->chirp.P_1,
                  arg->chirp.W_0,
                  arg->chirp.W_1,
                  arg->chirp.dA_0,
                  arg->chirp.dA_1,
                  arg->chirp.dW_0,
                  arg->chirp.dW_1,
                  arg->chirp.ddA_0,
                  arg->chirp.ddA_1);

        set_chirp(estimates+ym,
                  blocksize,
                  minX,maxX,
                  xi,x_n,
                  arg->x_dim&DIM_ESTIMATE_MASK,
                  minY,maxY,
                  y_n-yi-1,y_n,
                  arg->y_dim&DIM_ESTIMATE_MASK,
                  si,sn,
                  arg->sweep_or_rand_p,
                  arg->est.A_0,
                  arg->est.A_1,
                  arg->est.P_0,
                  arg->est.P_1,
                  arg->est.W_0,
                  arg->est.W_1,
                  arg->est.dA_0,
                  arg->est.dA_1,
                  arg->est.dW_0,
                  arg->est.dW_1,
                  arg->est.ddA_0,
                  arg->est.ddA_1);

        if(arg->alt_p){
          set_chirp(chirps+ym+1,
                    blocksize,
                    0,0,0,0,0,
                    0,0,0,0,0,
                    si,sn,
                    arg->sweep_or_rand_p,
                    arg->chirp_alt.A_0,
                    arg->chirp_alt.A_1,
                    arg->chirp_alt.P_0,
                    arg->chirp_alt.P_1,
                    arg->chirp_alt.W_0,
                    arg->chirp_alt.W_1,
                    arg->chirp_alt.dA_0,
                    arg->chirp_alt.dA_1,
                    arg->chirp_alt.dW_0,
                    arg->chirp_alt.dW_1,
                    arg->chirp_alt.ddA_0,
                    arg->chirp_alt.ddA_1);

          set_chirp(estimates+ym+1,
                    blocksize,
                    0,0,0,0,0,
                    0,0,0,0,0,
                    si,sn,
                    arg->sweep_or_rand_p,
                    arg->est_alt.A_0,
                    arg->est_alt.A_1,
                    arg->est_alt.P_0,
                    arg->est_alt.P_1,
                    arg->est_alt.W_0,
                    arg->est_alt.W_1,
                    arg->est_alt.dA_0,
                    arg->est_alt.dA_1,
                    arg->est_alt.dW_0,
                    arg->est_alt.dW_1,
                    arg->est_alt.ddA_0,
                    arg->est_alt.ddA_1);

          /* alt chirp can be relative to chirp */
          if(arg->chirp_alt.A_rel)
            chirps[ym+1].A = fromdB(todB(chirps[ym].A) + todB(chirps[ym+1].A));
          if(arg->chirp_alt.P_rel) chirps[ym+1].P += chirps[ym].P;
          if(arg->chirp_alt.W_rel) chirps[ym+1].W += chirps[ym].W;
          if(arg->chirp_alt.dA_rel) chirps[ym+1].dA += chirps[ym].dA;
          if(arg->chirp_alt.dW_rel) chirps[ym+1].dW += chirps[ym].dW;
          if(arg->chirp_alt.ddA_rel) chirps[ym+1].ddA += chirps[ym].ddA;

          /* chirp can be relative to alt chirp */
          if(arg->chirp.A_rel)
            chirps[ym].A = fromdB(todB(chirps[ym].A) + todB(chirps[ym+1].A));
          if(arg->chirp.P_rel) chirps[ym].P += chirps[ym+1].P;
          if(arg->chirp.W_rel) chirps[ym].W += chirps[ym+1].W;
          if(arg->chirp.dA_rel) chirps[ym].dA += chirps[ym+1].dA;
          if(arg->chirp.dW_rel) chirps[ym].dW += chirps[ym+1].dW;
          if(arg->chirp.ddA_rel) chirps[ym].ddA += chirps[ym+1].ddA;

          /* alt estimate can be relative to alt chirp */
          if(arg->est_alt.A_rel)
            estimates[ym+1].A = fromdB(todB(estimates[ym+1].A) + todB(chirps[ym+1].A));
          if(arg->est_alt.P_rel) estimates[ym+1].P += chirps[ym+1].P;
          if(arg->est_alt.W_rel) estimates[ym+1].W += chirps[ym+1].W;
          if(arg->est_alt.dA_rel) estimates[ym+1].dA += chirps[ym+1].dA;
          if(arg->est_alt.dW_rel) estimates[ym+1].dW += chirps[ym+1].dW;
          if(arg->est_alt.ddA_rel) estimates[ym+1].ddA += chirps[ym+1].ddA;
        }

        /* estimate can be relative to chirp */
        if(arg->est.A_rel) estimates[ym].A =
                             fromdB(todB(estimates[ym].A) + todB(chirps[ym].A));
        if(arg->est.P_rel) estimates[ym].P += chirps[ym].P;
        if(arg->est.W_rel) estimates[ym].W += chirps[ym].W;
        if(arg->est.dA_rel) estimates[ym].dA += chirps[ym].dA;
        if(arg->est.dW_rel) estimates[ym].dW += chirps[ym].dW;
        if(arg->est.ddA_rel) estimates[ym].ddA += chirps[ym].ddA;

      }

      /* compute column */
      for(i=0;i<threads;i++)
        pthread_create(threadlist+i,NULL,compute_column,targ+i);
      for(i=0;i<threads;i++)
        pthread_join(threadlist[i],NULL);

      /* accumulate results for this pass */
      if(si==0){
        for(i=0;i<y_n;i++){
          int im = i*(arg->alt_p?2:1);
          float Aen = chirps[im].A;
          float Ae = chirps[im].A - estimates[im].A;
          float Pe = circular_distance(chirps[im].P,estimates[im].P);
          float We = chirps[im].W - estimates[im].W;
          float dAe = chirps[im].dA - estimates[im].dA;
          float dWe = chirps[im].dW - estimates[im].dW;
          float ddAe = chirps[im].ddA - estimates[im].ddA;
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
          int im = i*(arg->alt_p?2:1);
          float v = chirps[im].A - estimates[im].A;
          if(ret_minA[i]>v)ret_minA[i]=v;
          if(ret_maxA[i]<v)ret_maxA[i]=v;
          ret_sqA[i]+=v*v;
          ret_enA[i]+=chirps[im].A*chirps[im].A;

          v = circular_distance(chirps[im].P, estimates[im].P);
          if(ret_minP[i]>v)ret_minP[i]=v;
          if(ret_maxP[i]<v)ret_maxP[i]=v;
          ret_sqP[i]+=v*v;

          v = chirps[im].W - estimates[im].W;
          if(ret_minW[i]>v)ret_minW[i]=v;
          if(ret_maxW[i]<v)ret_maxW[i]=v;
          ret_sqW[i]+=v*v;

          v = chirps[im].dA - estimates[im].dA;
          if(ret_mindA[i]>v)ret_mindA[i]=v;
          if(ret_maxdA[i]<v)ret_maxdA[i]=v;
          ret_sqdA[i]+=v*v;

          v = chirps[im].dW - estimates[im].dW;
          if(ret_mindW[i]>v)ret_mindW[i]=v;
          if(ret_maxdW[i]<v)ret_maxdW[i]=v;
          ret_sqdW[i]+=v*v;

          v = chirps[im].ddA - estimates[im].ddA;
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
      int x=MIN(x0s,x1s)+xi;
      int y=MAX(y0s,y1s)-yi;
      int xp=(x0s<x1s?xi:x_n-xi-1)+leftpad;
      int yp=(y0s<y1s?yi:y_n-yi-1)+toppad;
      float a = 1.;
      if(x%xminori==0 || y%yminori==0) a = .8;
      if(x%xmajori==0 || y%ymajori==0) a = .3;

      /* Average convergence graph */
      if(cC_m){
        set_iter_color(cC_m,rint(ret_sumiter[yi]/sn),a);
        cairo_rectangle(cC_m,xp,yp,1,1);
        cairo_fill(cC_m);
      }

      /* Worst case convergence graph */
      if(cC_w){
        set_iter_color(cC_w,ret_maxiter[yi],a);
        cairo_rectangle(cC_w,xp,yp,1,1);
        cairo_fill(cC_w);
      }

      /* Convergence delta graph */
      if(cC_d){
        set_iter_color(cC_d,ret_maxiter[yi]-ret_miniter[yi],a);
        cairo_rectangle(cC_d,xp,yp,1,1);
        cairo_fill(cC_d);
      }

      /* A MSE graph */
      if(cA_m){
        set_error_color(cA_m,(ret_enA[yi]>1e-20?
                            sqrt(ret_sqA[yi])/sqrt(ret_enA[yi]) : 1.),a);
        cairo_rectangle(cA_m,xp,yp,1,1);
        cairo_fill(cA_m);
      }

      /* A peak error graph */
      if(cA_w){
        set_error_color(cA_w,MAX(fabs(ret_maxA[yi]),fabs(ret_minA[yi])),a);
        cairo_rectangle(cA_w,xp,yp,1,1);
        cairo_fill(cA_w);
      }

      /* A delta graph */
      if(cA_d){
        set_error_color(cA_d,ret_maxA[yi]-ret_minA[yi],a);
        cairo_rectangle(cA_d,xp,yp,1,1);
        cairo_fill(cA_d);
      }

      /* P MSE graph */
      if(cP_m){
        set_error_color(cP_m,sqrt(ret_sqP[yi]/sn),a);
        cairo_rectangle(cP_m,xp,yp,1,1);
        cairo_fill(cP_m);
      }

      /* P peak error graph */
      if(cP_w){
        set_error_color(cP_w,MAX(fabs(ret_maxP[yi]),fabs(ret_minP[yi])),a);
        cairo_rectangle(cP_w,xp,yp,1,1);
        cairo_fill(cP_w);
      }

      /* P delta graph */
      if(cP_d){
        set_error_color(cP_d,circular_distance(ret_maxP[yi],ret_minP[yi]),a);
        cairo_rectangle(cP_d,xp,yp,1,1);
        cairo_fill(cP_d);
      }

      if(cW_m){
        /* W MSE graph */
        set_error_color(cW_m,sqrt(ret_sqW[yi]/sn)/2./M_PI*blocksize,a);
        cairo_rectangle(cW_m,xp,yp,1,1);
        cairo_fill(cW_m);
      }

      if(cW_w){
        /* W peak error graph */
        set_error_color(cW_w,MAX(fabs(ret_maxW[yi]),fabs(ret_minW[yi]))/2./M_PI*blocksize,a);
        cairo_rectangle(cW_w,xp,yp,1,1);
        cairo_fill(cW_w);
      }

        /* W delta graph */
      if(cW_d){
        set_error_color(cW_d,(ret_maxW[yi]-ret_minW[yi])/2./M_PI*blocksize,a);
        cairo_rectangle(cW_d,xp,yp,1,1);
        cairo_fill(cW_d);
      }

      if(cdA_m){
        /* dA MSE graph */
        set_error_color(cdA_m,sqrt(ret_sqdA[yi]/sn)*blocksize,a);
        cairo_rectangle(cdA_m,xp,yp,1,1);
        cairo_fill(cdA_m);
      }

      if(cdA_w){
        /* dA peak error graph */
        set_error_color(cdA_w,MAX(fabs(ret_maxdA[yi]),fabs(ret_mindA[yi]))*blocksize,a);
        cairo_rectangle(cdA_w,xp,yp,1,1);
        cairo_fill(cdA_w);
      }

        /* dA delta graph */
      if(cdA_d){
        set_error_color(cdA_d,(ret_maxdA[yi]-ret_mindA[yi])*blocksize,a);
        cairo_rectangle(cdA_d,xp,yp,1,1);
        cairo_fill(cdA_d);
      }

      if(cdW_m){
        /* dW MSE graph */
        set_error_color(cdW_m,sqrt(ret_sqdW[yi]/sn)/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_m,xp,yp,1,1);
        cairo_fill(cdW_m);
      }

      if(cdW_w){
        /* dW peak error graph */
        set_error_color(cdW_w,MAX(fabs(ret_maxdW[yi]),fabs(ret_mindW[yi]))/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_w,xp,yp,1,1);
        cairo_fill(cdW_w);
      }

      /* dW delta graph */
      if(cdW_d){
        set_error_color(cdW_d,(ret_maxdW[yi]-ret_mindW[yi])/2./M_PI*blocksize*blocksize,a);
        cairo_rectangle(cdW_d,xp,yp,1,1);
        cairo_fill(cdW_d);
      }

      if(cddA_m){
        /* ddA MSE graph */
        set_error_color(cddA_m,sqrt(ret_sqddA[yi]/sn)*blocksize*blocksize,a);
        cairo_rectangle(cddA_m,xp,yp,1,1);
        cairo_fill(cddA_m);
      }

      if(cddA_w){
        /* ddA peak error graph */
        set_error_color(cddA_w,MAX(fabs(ret_maxddA[yi]),fabs(ret_minddA[yi]))*blocksize*blocksize,a);
        cairo_rectangle(cddA_w,xp,yp,1,1);
        cairo_fill(cddA_w);
      }

      /* dA delta graph */
      if(cddA_d){
        set_error_color(cddA_d,(ret_maxddA[yi]-ret_minddA[yi])*blocksize*blocksize,a);
        cairo_rectangle(cddA_d,xp,yp,1,1);
        cairo_fill(cddA_d);
      }

      /* RMS error graph */
      if(cRMS_m){
        set_error_color(cRMS_m,
                        ret_sqENE[yi]>1e-20?
                        sqrt(ret_sqERR[yi])/sqrt(ret_sqENE[yi]):1,a);
        cairo_rectangle(cRMS_m,xp,yp,1,1);
        cairo_fill(cRMS_m);
      }

      /* RMS peak error graph */
      if(cRMS_w){
        set_error_color(cRMS_w,ret_maxRMS[yi],a);
        cairo_rectangle(cRMS_w,xp,yp,1,1);
        cairo_fill(cRMS_w);
      }

      if(cRMS_d){
        set_error_color(cRMS_d,ret_maxRMS[yi]-ret_minRMS[yi],a);
        cairo_rectangle(cRMS_d,xp,yp,1,1);
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

  feclearexcept(FE_ALL_EXCEPT);
  feenableexcept(except);

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
    /* W_alpha_0 */   1.,
    /* W_alpha_1 */   1.,
    /* dW_alpha_0 */  1.,
    /* dW_alpha_1 */  1.,

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

    /* ESTIMATE */
    (rel_param){
      /* est A range */  -999.,-999.,  0, /* relative flag */
      /* est P range */     0.,  0.,  0, /* relative flag */
      /* est W range */     0.,  0.,  1, /* relative flag */
      /* est dA range */    0.,  0.,  0, /* relative flag */
      /* est dW range */    0.,  0.,  0, /* relative flag */
      /* est ddA range */   0.,  0.,  0, /* relative flag */
    },

    /* CHIRP */
    (rel_param){
      /* ch A range */    0.,0., 0,
      /* ch P range */    0.,1.-1./32., 0,
      /* ch W range */    0.,0., 0,
      /* ch dA range */   0.,0., 0,
      /* ch dW range */   0.,0., 0,
      /* ch ddA range */  0.,0., 0,
    },

    /* ALT_ESTIMATE */
    (rel_param){
      /* alt est A range */  -999.,-999.,  0, /* relative flag */
      /* alt est P range */     0.,  0.,  0, /* relative flag */
      /* alt est W range */     0.,  0.,  1, /* relative flag */
      /* alt est dA range */    0.,  0.,  0, /* relative flag */
      /* alt est dW range */    0.,  0.,  0, /* relative flag */
      /* alt est ddA range */   0.,  0.,  0, /* relative flag */
    },

    /* ALT_CHIRP */
    (rel_param){
      /* alt ch A range */    0.,0., 0,
      /* alt ch P range */    0.,1.-1./32., 0,
      /* alt ch W range */    0.,0., 0,
      /* alt ch dA range */   0.,0., 0,
      /* alt ch dW range */   0.,0., 0,
      /* alt ch ddA range */  0.,0., 0,
    },
    /* alt chirp p */ 0,

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

#if 0
  /* Graphs for linear v. partial-nonlinear v. full-nonlinear ***************/
  /* dW vs W ****************************************************************/
  init_arg(&arg);
  arg.chirp.W_1=10.;
  arg.chirp.dW_0=-2.5;
  arg.chirp.dW_1=2.5;
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
  arg.chirp.W_1=10.;
  arg.est.W_0=-2.5;
  arg.est.W_1=2.5;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;

  arg.fit_nonlinear=0;
  graph_1chirp("algo-",&arg);

  arg.fit_nonlinear=1;
  graph_1chirp("algo-",&arg);

  arg.fit_nonlinear=2;
  graph_1chirp("algo-",&arg);
#endif

#if 0
  /* Graphs for comparison of various windows *******************************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.est.W_0=-2.5;
  arg.est.W_1=2.5;
  arg.chirp.W_1=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;

  arg.fit_nonlinear = 0;

  arg.window = window_functions.rectangle;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("win-",&arg);

  arg.est.W_0 = -15;
  arg.est.W_1 =  15;
  arg.chirp.W_1 =  25;
  arg.fit_nonlinear = 2;
  arg.window = window_functions.rectangle;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("win-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("win-",&arg);
#endif

#if 0
  /* Graphs for .5, 1, 1.5, 2nd order ***************************************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.est.W_0=-3;
  arg.est.W_1=3;
  arg.chirp.W_1=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;
  arg.fit_W = 1;
  arg.fit_dA = 0;
  arg.fit_dW = 0;
  arg.fit_ddA = 0;
  arg.window = window_functions.hanning;

  arg.fit_nonlinear = 2;
  graph_1chirp("order.5-",&arg);

  arg.fit_dA=1;

  arg.fit_nonlinear = 0;
  graph_1chirp("order1-",&arg);

  arg.fit_nonlinear = 2;
  graph_1chirp("order1-",&arg);

  arg.fit_dW=1;

  arg.fit_nonlinear = 2;
  graph_1chirp("order1.5-",&arg);

  arg.fit_ddA=1;

  arg.fit_nonlinear = 0;
  graph_1chirp("order2-",&arg);

  arg.fit_nonlinear = 2;
  graph_1chirp("order2-",&arg);
#endif

#if 0
  /* Symmetric norm *********************************************************/
  /* dW vs W ****************************************************************/
  init_arg(&arg);
  arg.chirp.dW_0 = -2.5;
  arg.chirp.dW_1 =  2.5;
  arg.chirp.W_1=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_CHIRP_dW;

  arg.fit_symm_norm = 1;

  arg.fit_nonlinear=0;
  graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=1;
  graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=2;
  graph_1chirp("symmetric-",&arg);

  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.est.W_0=-2.5;
  arg.est.W_1=2.5;
  arg.chirp.W_1=10;
  arg.x_dim=DIM_CHIRP_W;
  arg.y_dim=DIM_ESTIMATE_W;

  arg.fit_symm_norm = 1;

  arg.fit_nonlinear=0;
  graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=1;
  graph_1chirp("symmetric-",&arg);
  arg.fit_nonlinear=2;
  graph_1chirp("symmetric-",&arg);
#endif

#if 0
  /* W alpha ****************************************************************/
  /* estW vs alphaW *********************************************************/
  init_arg(&arg);
  arg.chirp.W_0 = arg.chirp.W_1 = rint(arg.blocksize/4);
  arg.fit_W_alpha_0 = 0;
  arg.fit_W_alpha_1 = 2.01612903225806451612;
  arg.est.W_0 = -3;
  arg.est.W_1 =  3;
  arg.x_dim = DIM_ALPHA_W;
  arg.y_dim = DIM_ESTIMATE_W;
  arg.x_minor=.0625;

  arg.fit_nonlinear = 2;

  arg.window = window_functions.rectangle;
  graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.sine;
  graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.hanning;
  graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.tgauss_deep;
  graph_1chirp("alphaW-",&arg);

  arg.window = window_functions.maxwell1;
  graph_1chirp("alphaW-",&arg);
#endif

#if 0
  /* dW alpha ***************************************************************/
  /* estW vs alphadW ********************************************************/
  init_arg(&arg);
  arg.chirp.W_0 = arg.chirp.W_1 = rint(arg.blocksize/4);
  arg.fit_dW_alpha_0 = 0;
  arg.fit_dW_alpha_1 = 3.125;
  arg.est.W_0 = -3;
  arg.est.W_1 =  3;
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
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("alphadW-",&arg);

  arg.y_dim = DIM_CHIRP_dW;
  arg.est.W_0 = 0;
  arg.est.W_1 = 0;
  arg.chirp.dW_0 = -3;
  arg.chirp.dW_1 =  3;

  arg.window = window_functions.rectangle;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("alphadW-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("alphadW-",&arg);
#endif

#if 0
  /* replot algo fits with opt dW alpha *************************************/
  /* estW vs W **************************************************************/
  init_arg(&arg);
  arg.est.W_0 = -9.375;
  arg.est.W_1 =  9.375;
  arg.chirp.W_0 =  0;
  arg.chirp.W_1 =  25;
  arg.x_dim = DIM_CHIRP_W;
  arg.y_dim = DIM_ESTIMATE_W;

  arg.fit_nonlinear = 2;

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.rectangle;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1=2.25;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.sine;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.hanning;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.maxwell1;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
  graph_1chirp("opt-",&arg);

  /* dW vs W **************************************************************/
  arg.chirp.dW_0 = -9.375;
  arg.chirp.dW_1 = 9.375;
  arg.est.W_0 = 0;
  arg.est.W_1 = 0;
  arg.y_dim = DIM_CHIRP_dW;

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.rectangle;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 2.25;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.sine;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.hanning;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
  graph_1chirp("opt-",&arg);

  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.;
  arg.window = window_functions.maxwell1;
  graph_1chirp("nonopt-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
  graph_1chirp("opt-",&arg);
#endif

#if 0
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
        arg.chirp.W_1=10.;
        arg.chirp.dW_0=-2.5;
        arg.chirp.dW_1=2.5;
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
          arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 2.25;
          arg.window = window_functions.rectangle;
          break;
        case 1:
          arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
          arg.window = window_functions.sine;
          break;
        case 2:
          arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
          arg.window = window_functions.hanning;
          break;
        case 3:
          arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
          arg.window = window_functions.tgauss_deep;
          break;
        case 4:
          arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
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
        arg.chirp.dW_0=0;
        arg.chirp.dW_1=0;
        arg.chirp.W_1=10.;
        arg.est.W_0=-2.5;
        arg.est.W_1=2.5;
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
#endif

#if 1
  /* Two chirp discrimination graphs ****************************************/
  /* First, graphs using a poor initial estimate with a zero
     amplitude.  This defeats the sorting/energy subtraction and
     illustrates sidelobe capture. */
  /* A vs W *****************************************************************/
  init_arg(&arg);

  arg.chirp_alt.W_0 = rint(arg.blocksize/4)-1;
  arg.chirp_alt.W_1 = rint(arg.blocksize/4)+1;
  arg.alt_p=1;
  arg.sweep_or_rand_p=1;
  arg.sweep_steps=128;

  arg.x_dim=DIM_CHIRP_W;
  arg.chirp.W_0 = 0;
  arg.chirp.W_1 = +20;
  arg.chirp.W_rel = 1; /* relative to alt chirp W */
  arg.x_major = 1;
  arg.x_minor = .5;
  arg.xaxis_label = "test chirp relative W (cycles/block)";

  arg.y_dim=DIM_CHIRP_A;
  arg.chirp.A_0 = -100.;
  arg.chirp.A_1 = 0.;
  arg.y_major = 20;
  arg.y_minor = 5;
  arg.yaxis_label = "test chirp amplitude (dB)";

  arg.fit_nonlinear=0;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-0A-",&arg);

  arg.fit_nonlinear=1;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-0A-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-0A-",&arg);

  arg.fit_nonlinear=2;
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 2.25;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-0A-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
  arg.window = window_functions.sine;
  graph_1chirp("2ch-0A-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-0A-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-0A-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-0A-",&arg);

  /* Alter the above graphs to use an accurate estimate */
  arg.est.W_0=arg.est.W_1=0;
  arg.est_alt.W_0=arg.est_alt.W_1=0;

  arg.est.A_rel=1;
  arg.est_alt.A_rel=1;
  arg.est.A_0=-.1;
  arg.est.A_1=.1;
  arg.est_alt.A_0=-.1;
  arg.est_alt.A_1=.1;

  arg.est.P_rel=1;
  arg.est_alt.P_rel=1;
  arg.est.P_0=-.1;
  arg.est.P_1=.1;
  arg.est_alt.P_0=-.1;
  arg.est_alt.P_1=.1;

  arg.fit_nonlinear=0;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-AA-",&arg);

  arg.fit_nonlinear=1;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-AA-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-AA-",&arg);

  arg.fit_nonlinear=2;
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 2.25;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-AA-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
  arg.window = window_functions.sine;
  graph_1chirp("2ch-AA-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-AA-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-AA-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-AA-",&arg);

  /* Simulate an estimate taken from an initial FFT  */
  arg.est.A_rel=1;
  arg.est_alt.A_rel=1;
  arg.est.A_0=0;
  arg.est.A_1=-6;
  arg.est_alt.A_0=0;
  arg.est_alt.A_1=6;

  arg.est.P_rel=1;
  arg.est_alt.P_rel=1;
  arg.est.P_0=-.1;
  arg.est.P_1=.1;
  arg.est_alt.P_0=-.1;
  arg.est_alt.P_1=.1;

  arg.est.W_0=0;
  arg.est.W_1=.5;
  arg.est_alt.W_0=0;
  arg.est_alt.W_1=-.5;

  arg.fit_nonlinear=0;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-FF-",&arg);

  arg.fit_nonlinear=1;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.sine;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-FF-",&arg);
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-FF-",&arg);

  arg.fit_nonlinear=2;
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 2.25;
  arg.window = window_functions.rectangle;
  graph_1chirp("2ch-FF-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
  arg.window = window_functions.sine;
  graph_1chirp("2ch-FF-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
  arg.window = window_functions.hanning;
  graph_1chirp("2ch-FF-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
  arg.window = window_functions.tgauss_deep;
  graph_1chirp("2ch-FF-",&arg);
  arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
  arg.window = window_functions.maxwell1;
  graph_1chirp("2ch-FF-",&arg);


  /* dW vs W *****************************************************************/
  arg.sweep_steps=32;
  arg.y_dim=DIM_CHIRP_dW;
  arg.chirp.dW_0 = -10.;
  arg.chirp.dW_1 = 10.;
  arg.y_major = 1;
  arg.y_minor = .25;
  arg.yaxis_label = "test chirp dW (cycles/block)";

  {
    int amp;
    for(amp=-60;amp<=0;amp+=20){
      char buf[80];
      snprintf(buf,80,"2ch-%d-",-amp);

      arg.chirp.A_0=arg.chirp.A_1=amp;

      arg.fit_nonlinear=0;
      arg.window = window_functions.rectangle;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.sine;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.hanning;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.tgauss_deep;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.maxwell1;
      graph_1chirp(buf,&arg);

      arg.fit_nonlinear=1;
      arg.window = window_functions.rectangle;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.sine;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.hanning;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.tgauss_deep;
      graph_1chirp(buf,&arg);
      arg.window = window_functions.maxwell1;
      graph_1chirp(buf,&arg);

      arg.fit_nonlinear=2;
      arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 2.25;
      arg.window = window_functions.rectangle;
      graph_1chirp(buf,&arg);
      arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.711;
      arg.window = window_functions.sine;
      graph_1chirp(buf,&arg);
      arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.618;
      arg.window = window_functions.hanning;
      graph_1chirp(buf,&arg);
      arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.5;
      arg.window = window_functions.tgauss_deep;
      graph_1chirp(buf,&arg);
      arg.fit_dW_alpha_0 = arg.fit_dW_alpha_1 = 1.554;
      arg.window = window_functions.maxwell1;
      graph_1chirp(buf,&arg);
    }
  }
#endif

  return 0;
}

