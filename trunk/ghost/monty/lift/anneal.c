/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 1994-2007              *
 * the Xiph.Org FOundation http://www.xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: implement adaptive simulated annealing
 last mod: $Id$

 ********************************************************************/

/* see: _Digital IIR Filter Design Using Adaptive Simpulated
   Annealing_ by S. Chen, R. Istepanian, and B. L. Luk */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static double g(double Ti){
  double ui = drand48();
  if(ui<.5)
    return -Ti * (pow(1.+1./Ti, fabs(2*ui-1))-1);
  else    
    return  Ti * (pow(1.+1./Ti, fabs(2*ui-1))-1);
}

static inline double todB(double x){
  return ((x)==0?-400:log((x)*(x))*4.34294480f);
}

void anneal(double *w, int m, 
	    double c, double U, double V, double epsilon,
	    int N_accept, int N_generate, int N_break,
	    double (*cost_func)(double *)){

  // (i)

  double cost = cost_func(w);
  double cost_best = cost;
  double w_best[m];
  double Tc0 = cost;
  double Tc = Tc0;
  double kc = 0.;
  double T[m];
  double k[m];
  int accepted = 0;
  int generated = 0;
  int bests = 0;
  int last_bests = 0;
  int break_counter = 0;
  int i;
  
  memcpy(w_best,w,sizeof(w_best));
  for(i=0;i<m;i++){
    T[i] = 1.;
    k[i] = 0.; 
  }

  fprintf(stderr,"\nannealing... %d/%d break:%d cost:%g        ",
	  accepted,generated,N_break-break_counter,todB(cost_best)/2);

  while(break_counter<N_break){

    // (ii) generate a new point

    double w_new[m];
    for(i=0; i<m; i++){
      while(1){
	w_new[i] = w[i] + g(T[i])*(V-U);
	if(w_new[i]<U)continue;
	if(w_new[i]>V)continue;
	break;
      }
    }

    double cost_new = cost_func(w_new);
    double P_accept = 1. / (1. + exp( (cost_new - cost)/Tc));
    double P_unif = drand48();

    if(P_unif <= P_accept){
      cost = cost_new;
      memcpy(w,w_new,sizeof(w_new));
      accepted++;

      if(cost < cost_best){
	cost_best = cost;
	memcpy(w_best,w,sizeof(w_best));
	bests++;

	fprintf(stderr,"\rannealing... %d/%d break:%d cost:%g        ",
		accepted,generated,N_break-break_counter,todB(cost_best)/2);
      }
    }
    generated++;

    // (iii) reannealing
    if(accepted >= N_accept){
      double s[m];
      for(i=0; i<m; i++){
	memcpy(w_new,w_best,sizeof(w_best));
	w_new[i]+=epsilon;

	s[i] = fabs((cost_func(w_new) - cost_best) / epsilon);
      }
      double s_max = s[0];
      for(i=1; i<m; i++)
	if(s_max < s[i]) s_max = s[i];

      for(i=0;i<m;i++){
	T[i] *= s_max/s[i];
	k[i] = pow(-log(T[i])/c, m);
      }

      Tc0 = cost;
      Tc = cost_best;
      kc = pow(-log(Tc/Tc0)/c, m);

      if(last_bests == bests) 
	break_counter++;
      else
	break_counter=0;

      last_bests = bests;

      fprintf(stderr,"\rannealing... %d/%d break:%d cost:%g        ",
	      accepted,generated,N_break-break_counter,todB(cost_best)/2);
      accepted = 0;
    }

    // (iv) temperature annealing
    if(generated >= N_generate){

      for(i=0;i<m;i++){
	k[i] += 1.;
	T[i] = exp(-c * pow(k[i], 1./m));
      }

      kc += 1.;
      Tc = Tc0 * exp(-c * pow(kc, 1./m));
      
      fprintf(stderr,"\rannealing... %d/%d break:%d cost:%g        ",
	      accepted,generated,N_break-break_counter,todB(cost_best)/2);
      generated=0;
    }
  }

  fprintf(stderr,"\rannealed.  %d/%d, cost:%g        ",
	  accepted,generated,todB(cost_best)/2);

  memcpy(w,w_best,sizeof(w_best));
}
