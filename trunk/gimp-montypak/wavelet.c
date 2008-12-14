/*
 *   Wavelet Denoise filter for GIMP - The GNU Image Manipulation Program
 *
 *   Copyright (C) 2008 Monty
 *   Code based on research by Crystal Wagner and Prof. Ivan Selesnik, 
 *   Polytechnic University, Brooklyn, NY
 *   See: http://taco.poly.edu/selesi/DoubleSoftware/
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include <string.h>
#include <string.h>
#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* convolution code below assumes FSZ is even */
#define FSZ 14 

static double analysis_fs[2][3][FSZ]={
  {
    {
      +0.00000000000000,
      +0.00069616789827,
      -0.02692519074183,
      -0.04145457368920,
      +0.19056483888763,
      +0.58422553883167,
      +0.58422553883167,
      +0.19056483888763,
      -0.04145457368920,
      -0.02692519074183,
      +0.00069616789827,
      +0.00000000000000,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      -0.00014203017443,
      +0.00549320005590,
      +0.01098019299363,
      -0.13644909765612,
      -0.21696226276259,
      +0.33707999754362,
      +0.33707999754362,
      -0.21696226276259,
      -0.13644909765612,
      +0.01098019299363,
      +0.00549320005590,
      -0.00014203017443,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00014203017443,
      -0.00549320005590,
      -0.00927404236573,
      +0.07046152309968,
      +0.13542356651691,
      -0.64578354990472,
      +0.64578354990472,
      -0.13542356651691,
      -0.07046152309968,
      +0.00927404236573,
      +0.00549320005590,
      -0.00014203017443,
      +0.00000000000000,
    }
  },
  {
    {
      +0.00000000000000, 
      +0.00000000000000, 
      +0.00069616789827,
      -0.02692519074183,
      -0.04145457368920,
      +0.19056483888763,
      +0.58422553883167,
      +0.58422553883167,
      +0.19056483888763,
      -0.04145457368920,
      -0.02692519074183,
      +0.00069616789827,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      -0.00014203017443,
      +0.00549320005590,
      +0.01098019299363,
      -0.13644909765612,
      -0.21696226276259,
      +0.33707999754362,
      +0.33707999754362,
      -0.21696226276259,
      -0.13644909765612,
      +0.01098019299363,
      +0.00549320005590,
      -0.00014203017443,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00014203017443,
      -0.00549320005590,
      -0.00927404236573,
      +0.07046152309968,
      +0.13542356651691,
      -0.64578354990472,
      +0.64578354990472,
      -0.13542356651691,
      -0.07046152309968,
      +0.00927404236573,
      +0.00549320005590,
      -0.00014203017443,
    }
  }
};

static double analysis[2][3][FSZ]={
  {
    {
      +0.00000000000000, 
      +0.00000000000000, 
      +0.00017870679071, 
      -0.02488304194507, 
      +0.00737700819766, 
      +0.29533805776119, 
      +0.59529279993637, 
      +0.45630440337458, 
      +0.11239376309619, 
      -0.01971220693439, 
      -0.00813549683439, 
      +0.00005956893024, 
      +0.00000000000000, 
      +0.00000000000000, 
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      -0.00012344587034,
      +0.01718853971559,
      -0.00675291099550,
      +0.02671809818132,
      -0.64763513288874,
      +0.47089724990858,
      +0.16040017815754,
      -0.01484700537727,
      -0.00588868840296,
      +0.00004311757177,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00001437252392,
      -0.00200122286479,
      +0.00027261232228,
      +0.06840460220387,
      +0.01936710587994,
      -0.68031992557818,
      +0.42976785708978,
      +0.11428688385011,
      +0.05057805218407,
      -0.00037033761102,
      +0.00000000000000,
      +0.00000000000000,
    }
  },
  {
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00005956893024,
      -0.00813549683439,
      -0.01971220693439,
      +0.11239376309619,
      +0.45630440337458,
      +0.59529279993637,
      +0.29533805776119,
      +0.00737700819766,
      -0.02488304194507,
      +0.00017870679071,
      +0.00000000000000,
      +0.00000000000000,
    },

    {
      +0.00000000000000,
      +0.00000000000000,
      -0.00037033761102,
      +0.05057805218407,
      +0.11428688385011,
      +0.42976785708978,
      -0.68031992557818,
      +0.01936710587994,
      +0.06840460220387,
      +0.00027261232228,
      -0.00200122286479,
      +0.00001437252392,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00004311757177,  
      -0.00588868840296,  
      -0.01484700537727,  
      +0.16040017815754,  
      +0.47089724990858,  
      -0.64763513288874,  
      +0.02671809818132,  
      -0.00675291099550,  
      +0.01718853971559,  
      -0.00012344587034,  
      +0.00000000000000,
      +0.00000000000000,
    }
  }
};

static double synthesis_fs[2][3][FSZ]={
  {
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00000000000000,
      +0.00069616789827,
      -0.02692519074183,
      -0.04145457368920,
      +0.19056483888763,
      +0.58422553883167,
      +0.58422553883167,
      +0.19056483888763,
      -0.04145457368920,
      -0.02692519074183,
      +0.00069616789827,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      -0.00014203017443,
      +0.00549320005590,
      +0.01098019299363,
      -0.13644909765612,
      -0.21696226276259,
      +0.33707999754362,
      +0.33707999754362,
      -0.21696226276259,
      -0.13644909765612,
      +0.01098019299363,
      +0.00549320005590,
      -0.00014203017443,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      -0.00014203017443,
      +0.00549320005590,
      +0.00927404236573,
      -0.07046152309968,
      -0.13542356651691,
      +0.64578354990472,
      -0.64578354990472,
      +0.13542356651691,
      +0.07046152309968,
      -0.00927404236573,
      -0.00549320005590,
      +0.00014203017443,
      +0.00000000000000,
    }
  },
  {
    {
      +0.00000000000000, 
      +0.00000000000000, 
      +0.00069616789827, 
      -0.02692519074183, 
      -0.04145457368920, 
      +0.19056483888763, 
      +0.58422553883167, 
      +0.58422553883167, 
      +0.19056483888763, 
      -0.04145457368920, 
      -0.02692519074183, 
      +0.00069616789827, 
      +0.00000000000000, 
      +0.00000000000000, 
    },
    {
      -0.00014203017443, 
      +0.00549320005590, 
      +0.01098019299363, 
      -0.13644909765612, 
      -0.21696226276259,
      +0.33707999754362,
      +0.33707999754362,
      -0.21696226276259,
      -0.13644909765612,
      +0.01098019299363,
      +0.00549320005590,
      -0.00014203017443,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      -0.00014203017443,
      +0.00549320005590,
      +0.00927404236573,
      -0.07046152309968,
      -0.13542356651691,
      +0.64578354990472,
      -0.64578354990472,
      +0.13542356651691,
      +0.07046152309968,
      -0.00927404236573,
      -0.00549320005590,
      +0.00014203017443,
      +0.00000000000000,
      +0.00000000000000,
    }
  }
};

static double synthesis[2][3][FSZ]={
  {
    {
      +0.00000000000000, 
      +0.00000000000000, 
      +0.00005956893024, 
      -0.00813549683439, 
      -0.01971220693439, 
      +0.11239376309619, 
      +0.45630440337458, 
      +0.59529279993637, 
      +0.29533805776119, 
      +0.00737700819766, 
      -0.02488304194507, 
      +0.00017870679071, 
      +0.00000000000000, 
      +0.00000000000000, 
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00004311757177,
      -0.00588868840296,
      -0.01484700537727,
      +0.16040017815754,
      +0.47089724990858,
      -0.64763513288874,
      +0.02671809818132,
      -0.00675291099550,
      +0.01718853971559,
      -0.00012344587034,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      -0.00037033761102,
      +0.05057805218407,
      +0.11428688385011,
      +0.42976785708978,
      -0.68031992557818,
      +0.01936710587994,
      +0.06840460220387,
      +0.00027261232228,
      -0.00200122286479,
      +0.00001437252392,
      +0.00000000000000,
      +0.00000000000000,
    }
  },
  {
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00017870679071,
      -0.02488304194507,
      +0.00737700819766,
      +0.29533805776119,
      +0.59529279993637,
      +0.45630440337458,
      +0.11239376309619,
      -0.01971220693439,
      -0.00813549683439,
      +0.00005956893024,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      +0.00001437252392,
      -0.00200122286479,
      +0.00027261232228,
      +0.06840460220387,
      +0.01936710587994,
      -0.68031992557818,
      +0.42976785708978,
      +0.11428688385011,
      +0.05057805218407,
      -0.00037033761102,
      +0.00000000000000,
      +0.00000000000000,
    },
    {
      +0.00000000000000,
      +0.00000000000000,
      -0.00012344587034,  
      +0.01718853971559,  
      -0.00675291099550,  
      +0.02671809818132,  
      -0.64763513288874,  
      +0.47089724990858,  
      +0.16040017815754,  
      -0.01484700537727,  
      -0.00588868840296,  
      +0.00004311757177,  
      +0.00000000000000,
      +0.00000000000000,
    }
  }
};

typedef struct {
  double *x;
  int rows;
  int cols;
} m2D;

typedef struct {
  m2D w[8][2][2];
} wrank;

static m2D alloc_m2D(int m, int n){
  m2D ret;
  ret.rows = m;
  ret.cols = n;
  ret.x = calloc(m*n,sizeof(*(ret.x)));
  return ret;
}

static void free_m2D(m2D *c){
  if(c->x) free(c->x);
  c->x = NULL;
}

static void complex_threshold(m2D set[4], double T, int soft, int pt, int *pc, int(*check)(void)){
  int i,j;
  int N = set[0].rows*set[0].cols;

  if(T>.01){
    double TT = T*T;

    if(soft){
      for(i=0;i<N;){
	for(j=0;j<set[0].cols;i++,j++){
	  double v00 = (set[0].x[i] + set[3].x[i]) * .7071067812;
	  double v11 = (set[0].x[i] - set[3].x[i]) * .7071067812;
	  double v01 = (set[1].x[i] + set[2].x[i]) * .7071067812;
	  double v10 = (set[1].x[i] - set[2].x[i]) * .7071067812;
	  
	  if(v00*v00 + v10*v10 < TT){
	    v00 = 0.;
	    v10 = 0.;
	  }else{
	    double y = sqrt(v00*v00 + v10*v10);
	    double scale = y/(y+T);
	    v00 *= scale;
	    v10 *= scale;
	  }
	  if(v01*v01 + v11*v11 < TT){
	    v01 = 0.;
	    v11 = 0.;
	  }else{
	    double y = sqrt(v01*v01 + v11*v11);
	    double scale = y/(y+T);
	    v01 *= scale;
	    v11 *= scale;
	  }
	  
	  set[0].x[i] = (v00 + v11) * .7071067812;
	  set[3].x[i] = (v00 - v11) * .7071067812;
	  set[1].x[i] = (v01 + v10) * .7071067812;
	  set[2].x[i] = (v01 - v10) * .7071067812;
	}
	if(check && check())return;
      }
    }else{
      for(i=0;i<N;){
	for(j=0;j<set[0].cols;i++,j++){
	  double v00 = (set[0].x[i] + set[3].x[i]) * .7071067812;
	  double v11 = (set[0].x[i] - set[3].x[i]) * .7071067812;
	  double v01 = (set[1].x[i] + set[2].x[i]) * .7071067812;
	  double v10 = (set[1].x[i] - set[2].x[i]) * .7071067812;
	  
	  if(v00*v00 + v10*v10 < TT){
	    v00 = 0.;
	    v10 = 0.;
	  }
	  if(v01*v01 + v11*v11 < TT){
	    v01 = 0.;
	    v11 = 0.;
	  }
	  
	  set[0].x[i] = (v00 + v11) * .7071067812;
	  set[3].x[i] = (v00 - v11) * .7071067812;
	  set[1].x[i] = (v01 + v10) * .7071067812;
	  set[2].x[i] = (v01 - v10) * .7071067812;
	}
	if(check && check())return;
      } 
    }
  }
  if(pt)gimp_progress_update((gdouble)((*pc)++)/pt);
}

/* assume the input is padded, return output that's padded for next stage */
/* FSZ*2-2 padding, +1 additional padding if vector odd */

static m2D convolve_padded(const m2D x, double *f, int pt, int *pc, int(*check)(void)){
  int i,M = x.rows;
  int j,in_N = x.cols;
  int k;
  int comp_N = (in_N - FSZ + 3) / 2;
  int out_N = (comp_N+1)/2*2+FSZ*2-2;

  m2D y=alloc_m2D(M,out_N);
  if(check && check())return y;

  for(i=0;i<M;i++){
    double *xrow = x.x+i*in_N;
    double *yrow = y.x+i*out_N+FSZ-1;
    for(j=0;j<comp_N;j++){
      double a = 0;
      for(k=0;k<FSZ;k++)
	a+=xrow[k]*f[FSZ-k-1];
      xrow+=2;
      yrow[j]=a;
    }
    if(check && check())return y;

    /* extend output padding */
    for(j=0;j<FSZ-1;j++){
      yrow--;
      yrow[0]=yrow[1];
    }
    for(j=FSZ-1+comp_N;j<out_N;j++)
      yrow[j] = yrow[j-1];
    
    if(check && check())return y;

  }
  if(pt)gimp_progress_update((gdouble)((*pc)++)/pt);
  return y;
}

static m2D convolve_transpose_padded(const m2D x, double *f, int pt, int *pc, int(*check)(void)){
  int i,M = x.rows;
  int j,in_N = x.cols;
  int k;
  int comp_N = (in_N - FSZ + 3) / 2;
  int out_N = (comp_N+1)/2*2+FSZ*2-2;

  m2D y=alloc_m2D(out_N,M);
  if(check && check())return y;

  for(i=0;i<M;i++){
    double *xrow = x.x+i*in_N;
    double *ycol = y.x + i + M*(FSZ-1);
    for(j=0;j<comp_N;j++){
      double a = 0;
      for(k=0;k<FSZ;k++)
	a+=xrow[k]*f[FSZ-k-1];
      xrow+=2;
      ycol[j*M]=a;
    }
    if(check && check())return y;

    /* extend output padding */
    for(j=0;j<FSZ-1;j++){
      ycol -= M;
      ycol[0] = ycol[M];
    }
    for(j=FSZ-1+comp_N;j<out_N;j++)
      ycol[j*M] = ycol[(j-1)*M];

    if(check && check())return y;

  }
  if(pt)gimp_progress_update((gdouble)((*pc)++)/pt);
  return y;
}

/* discards the padding added by the matching convolution */

/* y will often be smaller than a full x expansion due to preceeding
   rounds of padding out to even values; this padding is also
   discarded */
static void deconvolve_padded(m2D y, m2D x, double *f, int pt, int *pc, int(*check)(void)){
  int i;
  int j,in_N = x.cols;
  int k;
  int out_N = y.cols;
  int out_M = y.rows;

  for(i=0;i<out_M;i++){
    double *xrow = x.x+i*in_N+FSZ/2;
    double *yrow = y.x+i*out_N;

    for(j=0;j<out_N;j+=2){
      double a = 0;
      for(k=1;k<FSZ;k+=2)
	a+=xrow[k>>1]*f[FSZ-k-1];
      yrow[j]+=a;
      a=0.;
      for(k=1;k<FSZ;k+=2)
	a+=xrow[k>>1]*f[FSZ-k];
      yrow[j+1]+=a;
      xrow++;
    }
    if(check && check()) return;
  }
  
  if(pt)gimp_progress_update((gdouble)((*pc)++)/pt);
}

/* discards the padding added by the matching convolution */

/* y will often be smaller than a full x expansion due to preceeding
   rounds of padding out to even values; this padding is also
   discarded */
static void deconvolve_transpose_padded(m2D y, m2D x, double *f, int pt, int *pc, int(*check)(void)){
  int i;
  int j,in_N = x.cols;
  int k;
  int out_M = y.rows;
  int out_N = y.cols;

  for(i=0;i<out_N;i++){
    double *xrow = x.x+i*in_N+FSZ/2;
    double *ycol = y.x+i;
    for(j=0;j<out_M;j+=2){
      double a = 0;
      for(k=1;k<FSZ;k+=2)
	a+=xrow[k>>1]*f[FSZ-k-1];
      ycol[j*out_N]+=a;
      a=0.;
      for(k=1;k<FSZ;k+=2)
	a+=xrow[k>>1]*f[FSZ-k];
      ycol[(j+1)*out_N]+=a;
      xrow++;
    }
    if(check && check()) return;
  }
  
  if(pt)gimp_progress_update((gdouble)((*pc)++)/pt);
}

/* consumes and replaces lo if free_input set, otherwise overwrites */
static void forward_threshold(m2D lo[4], m2D y[4], 
			      double af[2][3][FSZ], double sf[2][3][FSZ], 
			      double T, int soft,
			      int free_input, int pt, int *pc, 
			      int(*check)(void)){
  m2D x[4] = {{NULL,0,0},{NULL,0,0},{NULL,0,0},{NULL,0,0}};
  m2D temp[4] = {{NULL,0,0},{NULL,0,0},{NULL,0,0},{NULL,0,0}};
  m2D tempw[4] = {{NULL,0,0},{NULL,0,0},{NULL,0,0},{NULL,0,0}};
  m2D temp2[4] = {{NULL,0,0},{NULL,0,0},{NULL,0,0},{NULL,0,0}};
  int r,c,i;
  
  for(i=0;i<4;i++){
    x[i] = lo[i];
    lo[i] = (m2D){NULL,0,0};
  }

  for(i=0;i<4;i++){
    temp[i]  = convolve_transpose_padded(x[i], af[i>>1][2], pt, pc, check);  
    if(check && check())goto cleanup;
    tempw[i] = convolve_padded(temp[i], af[i&1][2], pt, pc, check); /* w 7 */
    if(check && check())goto cleanup;
  }

  r = tempw[0].rows;
  c = tempw[0].cols;
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    temp2[i]=alloc_m2D(c*2 - FSZ*3 + 2, r);
    y[i] = alloc_m2D(temp2[i].rows, temp2[i].cols*2 - FSZ*3 + 2);
    if(check && check())goto cleanup;
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][2], pt, pc, check);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i] = convolve_padded(temp[i], af[i&1][1], pt, pc, check); /* w6 */
    if(check && check())goto cleanup;
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][1], pt, pc, check);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i] = convolve_padded(temp[i], af[i&1][0], pt, pc, check); /* w5 */
    if(check && check())goto cleanup;
    free_m2D(temp+i);
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][0], pt, pc, check);
    if(check && check())goto cleanup;
    deconvolve_padded(y[i],temp2[i],sf[i>>1][2], pt, pc, check);
    if(check && check())goto cleanup;
    temp[i]   = convolve_transpose_padded(x[i], af[i>>1][1], pt, pc, check);
    if(check && check())goto cleanup;
    free_m2D(temp2+i);
    temp2[i]  = alloc_m2D(c*2 - FSZ*3 + 2, r);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i]  = convolve_padded(temp[i], af[i&1][2], pt, pc, check); /* w4 */
    if(check && check())goto cleanup;
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][2], pt, pc, check);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i] = convolve_padded(temp[i], af[i&1][1], pt, pc, check); /* w3 */
    if(check && check())goto cleanup;
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][1], pt, pc, check);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i] = convolve_padded(temp[i], af[i&1][0], pt, pc, check); /* w2 */
    if(check && check())goto cleanup;
    free_m2D(temp+i);
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][0], pt, pc, check);
    if(check && check())goto cleanup;
    deconvolve_padded(y[i],temp2[i],sf[i>>1][1], pt, pc, check);
    if(check && check())goto cleanup;
    temp[i]  = convolve_transpose_padded(x[i], af[i>>1][0], pt, pc, check);
    if(check && check())goto cleanup;
    if(free_input) free_m2D(x+i);
    free_m2D(temp2+i);
    temp2[i] = alloc_m2D(c*2 - FSZ*3 + 2, r);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i] = convolve_padded(temp[i], af[i&1][2], pt, pc, check); /* w1 */
    if(check && check())goto cleanup;
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][2], pt, pc, check);
    if(check && check())goto cleanup;
    free_m2D(tempw+i);
    tempw[i] = convolve_padded(temp[i], af[i&1][1], pt, pc, check); /* w0 */
    if(check && check())goto cleanup;
  }
  complex_threshold(tempw, T, soft, pt, pc, check);
  if(check && check())goto cleanup;

  for(i=0;i<4;i++){
    deconvolve_transpose_padded(temp2[i],tempw[i],sf[i&1][1], pt, pc, check);
    if(check && check())goto cleanup;
    deconvolve_padded(y[i],temp2[i],sf[i>>1][0], pt, pc, check);
    if(check && check())goto cleanup;
    lo[i] = convolve_padded(temp[i], af[i&1][0], pt, pc, check); /* lo */
    if(check && check())goto cleanup;
    free_m2D(temp+i);
    if(check && check())goto cleanup;
  }

 cleanup:
  for(i=0;i<4;i++){
    if(free_input)free_m2D(x+i);
    free_m2D(temp+i);
    free_m2D(tempw+i);
    free_m2D(temp2+i);
  }
}

/* consumes/replaces lo */
static void finish_backward(m2D lo[4], m2D y[4], double sf[2][3][FSZ], int pt, int *pc, int(*check)(void)){
  int r=lo[0].rows,c=lo[0].cols,i;
  m2D temp = {NULL,0,0};
  
  for(i=0;i<4;i++){
    temp=alloc_m2D(c*2 - FSZ*3 + 2, r);
    deconvolve_transpose_padded(temp,lo[i],sf[i&1][0], pt, pc, check);
    free_m2D(lo+i);
    if(check && check()){
      free_m2D(&temp);
      return;
    }
    deconvolve_padded(y[i],temp,sf[i>>1][0], pt, pc, check);
    free_m2D(&temp);
    lo[i]=y[i];
    y[i]=(m2D){NULL,0,0};
    if(check && check()) return;
  }
}

static m2D transform_threshold(m2D x, int J, double T[16], int soft, int pt, int *pc, int (*check)(void)){
  int i,j;
  m2D partial_y[J][4];
  m2D lo[4];

  for(j=0;j<J;j++)
    for(i=0;i<4;i++)
      partial_y[j][i]=(m2D){NULL,0,0};

  lo[0] = lo[1] = lo[2] = lo[3] = x;
  forward_threshold(lo, partial_y[0], analysis_fs, synthesis_fs, T[0], soft, 0, pt, pc, check);
  if(check && check()) goto cleanup3;

  for(j=1;j<J;j++){
    forward_threshold(lo, partial_y[j], analysis, synthesis, T[j], soft, 1, pt, pc, check);
    if(check && check()) goto cleanup3;
  }

  for(j=J-1;j;j--){
    finish_backward(lo, partial_y[j], synthesis, pt, pc, check);
    if(check && check()) goto cleanup3;
  }

  finish_backward(lo, partial_y[0], synthesis_fs, pt, pc, check);
  if(check && check()) goto cleanup3;

  {
    int end = lo[0].rows*lo[0].cols;
    double *y = lo[0].x;
    double *p1 = lo[1].x;
    double *p2 = lo[2].x;
    double *p3 = lo[3].x;
    
    for(j=0;j<end;j++)
      y[j]+=p1[j]+p2[j]+p3[j];
    
    if(check && check()) goto cleanup3;
  }
  
 cleanup3:

  for(j=0;j<J;j++)
    for(i=0;i<4;i++)
      free_m2D(&partial_y[j][i]);

  free_m2D(lo+1);
  free_m2D(lo+2);
  free_m2D(lo+3);
  return lo[0];
  
}

static int wavelet_filter(int width, int height, int planes, guchar *buffer, guchar *mask,
			  int pr, double T[16],int soft, int (*check)(void)){

  int J=4;
  int i,j,p;
  m2D xc={NULL,0,0};
  m2D yc={NULL,0,0};
  int pc=0;
  int pt;

  /* we want J to be as 'deep' as the image to eliminate
     splotchiness with deep coarse settings */
  
  while(1){
    int mult = 1 << (J+1);
    if(width/mult < 1) break;
    if(height/mult < 1) break;
    J++;
  }

  if(J>15)J=15;
  pt=(pr?J*108*planes:0);
  
  /* Input matrix must be pre-padded for first stage convolutions;
     odd->even padding goes on the bottom/right */

  xc = alloc_m2D((height+1)/2*2+FSZ*2-2,
		 (width+1)/2*2+FSZ*2-2),yc;
  if(check && check())goto abort;
  
  /* loop through planes */
  for(p=0;p<planes;p++){
    guchar *ptr = buffer+p; 
    
    /* populate and pad input matrix */
    for(i=0;i<height;i++){
      double *row=xc.x + (i+FSZ-1)*xc.cols;

      /* X padding */
      for(j=0;j<FSZ-1;j++)
	row[j] = *ptr * .5;

      /* X filling */
      for(;j<width+FSZ-1;j++){
	row[j] = *ptr * .5;
	ptr+=planes;
      }
    
      /* X padding */
      for(;j<xc.cols;j++)
	row[j] = row[j-1];
    }

    /* Y padding */
    for(i=FSZ-2;i>=0;i--){
      double *pre=xc.x + (i+1)*xc.cols;
      double *row=xc.x + i*xc.cols;
      for(j=0;j<xc.cols;j++)
	row[j]=pre[j];
    }
    for(i=xc.rows-FSZ+1;i<xc.rows;i++){
      double *pre=xc.x + (i-1)*xc.cols;
      double *row=xc.x + i*xc.cols;
      for(j=0;j<xc.cols;j++)
	row[j]=pre[j];
    }

    if(check && check())goto abort;
    yc=transform_threshold(xc,J,T,soft,pt,&pc,check);
    if(check && check())goto abort;

    /* pull filtered values back out of padded matrix */
    {
      int k=0;
      ptr = buffer+p; 
      for(i=0;i<height;i++){
	double *row = yc.x + (i+FSZ-1)*yc.cols + FSZ-1;

	if(mask){
	  for(j=0;j<width;j++,k++){
	    double del = mask[k]*.0039215686;
	    int v = rint(del*row[j]*.5 + (1.-del)* *ptr);
	    if(v>255)v=255;if(v<0)v=0;
	    *ptr = v;
	    ptr+=planes;
	  }
	}else{
	  for(j=0;j<width;j++){
	    int v = rint(row[j]*.5);
	    if(v>255)v=255;if(v<0)v=0;
	    *ptr = v;
	    ptr+=planes;
	  }
	}
      }
    }
    
    if(check && check())goto abort;
    free_m2D(&yc);
    if(check && check())goto abort;
  }

 abort:
  free_m2D(&yc);
  free_m2D(&xc);
  return (check && check());
}

