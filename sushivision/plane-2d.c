/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <cairo-ft.h>
#include "internal.h"

static inline swapp(void **a, void **b){
  void *tmp=*a;
  *a = *b;
  *b = tmp;
}

static float slow_scale_map(sv_scalespace_t *to, sv_scalespace_t *from,
			    unsigned char *delA, unsigned char *delB, 
			    int *posA, int *posB,
			    int xymul){
  int i;
  int dw = from->pixels;
  int pw = to->pixels;

  long scalenum = _sv_scalespace_scalenum(to,from);
  long scaleden = _sv_scalespace_scaleden(to,from);
  long del = _sv_scalespace_scaleoff(to,from);
  int bin = del / scaleden;
  int discscale = (scaleden>scalenum?scalenum:scaleden);
  int total = xymul*scalenum/discscale;
  del -= bin * scaleden; 
  
  for(i=0;i<pw;i++){
    long del2 = del + scalenum;
    int sizeceil = (del2 + scaleden - 1)/ scaleden; // ceiling
    int sizefloor = del2 / scaleden;

    while(bin<0 && del2>scaleden){
      bin++;
      del = 0;
      del2 -= scaleden;
      sizeceil--;
    }
    
    if(del2 > scaleden && bin>=0 && bin<dw){
      int rem = total;

      delA[i] = ((xymul * (scaleden - del)) + (discscale>>1)) / discscale;
      posA[i] = bin;
      rem -= delA[i];
      rem -= xymul*(sizeceil-2);

      while(bin+sizeceil>dw){
	sizeceil--;
	del2=0;
      }

      del2 %= scaleden;
      if(rem<0){
	delA[i] += rem;
	delB[i] = 0;
      }else{
	delB[i] = rem; // don't leak 
      }
      posB[i] = bin+sizeceil;

    }else{
      if(bin<0 || bin>=dw){
	delA[i] = 0;
	posA[i] = 0;
	delB[i] = 0;
	posB[i] = 0;
      }else{
	delA[i] = xymul;
	posA[i] = bin;
	delB[i] = 0;
	posB[i] = bin+1;
	if(del2 == scaleden)del2=0;
      }
    }

    bin += sizefloor;
    del = del2;
  }
  return (float)xymul/total;
}

static void slow_scale(sv_plane_t *pl, 
		       sv_ucolor_t *work,
		       sv_scalespace_t dx, sv_scalespace_t dy,
		       sv_scalespace_t ix, sv_scalespace_t iy,
		       void (*mapfunc)(int,int, _sv_lcolor_t *), 
		       float alpha, int i){

  // sv_slider_t *scale= pl->scale; XXXXXXXXXXXx
  sv_ucolor_t *image = pl->image;
  int iw = ix.pixels;
  int ih = iy.pixels;
  int dw = dx.pixels;
  int dh = dy.pixels;
  sv_ccolor_t *cwork = work;

  if(ih!=dh || iw!=dw){
    /* resampled row computation; may involve multiple data rows */

    float idel = pl->resample_yscalemul * pl->resample_xscalemul;
    
    /* by column */
    /* XXXXX by row would be far more efficient... */
    int ydelA=pl->resample_ydelA[i];
    int ydelB=pl->resample_ydelB[i];
    int ystart=pl->resample_ynumA[i];
    int yend=pl->resample_ynumB[i];
    int lh = yend - ystart;
    float data[lh*dw];
    float *in_data = pl->data+ystart*dw;
    unsigned char *xdelA = pl->resample_xdelA;
    unsigned char *xdelB = pl->resample_xdelB;
    int *xnumA = pl->resample_xnumA;
    int *xnumB = pl->resample_xnumB;

    for(j=0;j<lh*dw;j++)
      data[j] = _sv_slider_val_to_mapdel(scale, in_data[j])*65535.f;
          
    /* by panel col */
    for(j=0;j<pw;j++){
      
      sv_lcolor_t out = (sv_lcolor_t){0,0,0,0}; 
      int xstart = xnumA[j];
      int xend = xnumB[j];
      int dx = xstart;
      int xA = xdelA[j];
      int xB = xdelB[j];
      int y = ystart;

      // first line
      if(y<yend){
	if(dx<xend)
	  mapfunc(data[dx++], ydelA*xA, &out);
	
	for(; dx < xend-1; dx++)
	  mapfunc(data[dx], ydelA*17, &out);
	
	if(dx<xend)
	  mapfunc(data[dx], ydelA*xB, &out);
	y++;
      }

      // mid lines
      for(;y<yend-1;y++){
	dx = xstart += dw;
	xend += dw;
	if(dx<xend)
	  mapfunc(data[dx++], 15*xA, &out);
	
	for(; dx < xend-1; dx++)
	  mapfunc(data[dx], 255, &out);
	
	if(dx<xend)
	  mapfunc(data[dx], 15*xB, &out);
      }
      
      // last line
      if(y<yend){
	dx = xstart += dw;
	xend += dw;
	if(dx<xend)
	  mapfunc(data[dx++], ydelB*xA, &out);
	
	for(; dx < xend-1; dx++)
	  mapfunc(data[dx], ydelB*17, &out);
	
	if(dx<xend)
	  mapfunc(data[dx], ydelB*xB, &out);
      }

      work[j].a = (u_int32_t)(out.a*idel);
      work[j].r = (u_int32_t)(out.r*idel);
      work[j].g = (u_int32_t)(out.g*idel);
      work[j].b = (u_int32_t)(out.b*idel);
      
    }

  }else{
    /* non-resampling render */

    float data[dw];
    float *in_data = pl->data+i*dw;

    for(j=0;j<dw;j++)
      data[j] = _sv_slider_val_to_mapdel(scale, in_data[j])*65535.f;

    for(j=0;j<pw;j++){

      sv_lcolor_t out = (sv_lcolor_t){0,0,0,0};
      mapfunc(data[j], 255, &out);
	
      work[j].a = (u_int32_t)(out.a);
      work[j].r = (u_int32_t)(out.r);
      work[j].g = (u_int32_t)(out.g);
      work[j].b = (u_int32_t)(out.b);
    }
  }
}

static void fast_scale_map(int *map,
			   sv_scalespace_t new,
			   sv_scalespace_t old){
  int i;
  double old_n = old.pixels;
  double new_n = new.pixels;

  double old_lo = _sv_scalespace_value(&old,0);
  double old_hi = _sv_scalespace_value(&old,old_n);
  double new_lo = _sv_scalespace_value(&new,0);
  double new_hi = _sv_scalespace_value(&new,new_n);
  double newscale = (new_hi-new_lo)/new_n;
  double oldscale = old_n/(old_hi-old_lo);
  for(i=0;i<new_n;i++){
    double val = i*newscale+new_lo;
    map[i] = (int)floor((val-old_lo)*oldscale);
  }
}

static void fast_scale_imagex(sv_ucolor_t *data, 
			      sv_scalespace_t new,
			      sv_scalespace_t old,
			      int *map){
  int i;
  sv_ucolor_t work[new_w];

  for(i=0;i<new.pixels;i++){
    int base = map[i];
    if(base<0 || base>=old.pixels){
      work[i]=NAN;
    }else{
      work[i]=data[base];
    }      
  }
  memcpy(data,work,new.pixels*sizeof(*work));
}

static void fast_scale_imagey(sv_ucolor_t *olddata, 
			      sv_ucolor_t *newdata, 
			      int new_w,
			      int old_w,
			      sv_scalespace_t new,
			      sv_scalespace_t old,
			      int *map){
  int i;

  for(i=0;i<new.pixels;i++){
    int base = map[i];
    if(base<0 || base>=old.pixels){
      *newdata=NAN;
    }else{
      *newdata=olddata[base*old_w];
    }
    newdata+=new_w;
  }
}

static void fast_scale_datax(float *data, 
			     sv_scalespace_t new,
			     sv_scalespace_t old,
			     int *map){
  int i;
  float work[new_w];

  for(i=0;i<new.pixels;i++){
    int base = map[i];
    if(base<0 || base>=old.pixels){
      work[i]=NAN;
    }else{
      work[i]=data[base];
    }      
  }
  memcpy(data,work,new.pixels*sizeof(*work));
}

static void fast_scale_datay(float *olddata, 
			      float *newdata, 
			      int new_w,
			      int old_w,
			      sv_scalespace_t new,
			      sv_scalespace_t old,
			      int *map){
  int i;

  for(i=0;i<new.pixels;i++){
    int base = map[i];
    if(base<0 || base>=old.pixels){
      *newdata=NAN;
    }else{
      *newdata=olddata[base*old_w];
    }
    newdata+=new_w;
  }
}


// called from worker thread
static void recompute_setup(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  sv_dim_data_t *ddx = p->dim_data+p->x_dim;
  sv_dim_data_t *ddy = p->dim_data+p->y_dim;
  int w = p->bg->image_x->pixels;
  int h = p->bg->image_y->pixels;

  pl->pending_data_x = 
    _sv_dim_datascale(ddx, p->bg->image_x, 
		      w * p->oversample_n / p->oversample_d, 0);
  pl->pending_data_y = 
    _sv_dim_datascale(ddy, p->bg->image_y, 
		      h * p->oversample_n / p->oversample_d, 1);
  
  pl->image_serialno++;
  pl->data_waiting=0;
  pl->data_incomplete=0;
  pl->data_next=0;

  pl->image_task=0;

}

// called from worker thread
static int image_resize(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  sv_scalespace_t newx = p->bg->image_x;
  sv_scalespace_t newy = p->bg->image_y;
  sv_scalespace_t oldx = pl->image_x;
  sv_scalespace_t oldy = pl->image_y;
  int new_w = newx.pixels;
  int new_h = newy.pixels;
  int old_w = oldx.pixels;
  int old_h = oldy.pixels;
  int serialno = p->comp_serialno;

  if(pl->image_task==-1) return STATUS_BUSY;

  if(pl->image_task==0){ // realloc
    sv_ucolor_t *image = NULL;
    int *map = NULL;

    // drop locks while alloc()ing/free()ing
    pl->image_task = -1;    
    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);

    image = calloc(new_w*new_h,sizeof(*image));
    map = calloc(max(new_h,new_w),sizeof(*map));
    if(new_w > old_w){
      fast_scale_map(map,new_h,newy,oldy);
    }else{
      fast_scale_map(map,old_w,newx,oldx);
    }

    pthread_rwlock_wrlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    if(p->comp_serialno == serialno){
      
      swapp(&pl->pending_image, &image);
      swapp(&pl->map, &map);
      pl->image_task = 1;
      pl->image_next=0;
      
      if(new_w > old_w){
	// y then x
	pl->image_waiting=old_w;
	pl->image_incomplete=old_w;
      }else{
	// x then y
	pl->image_waiting=old_h;
	pl->image_incomplete=old_h;
      }      
      image = NULL;
      map = NULL;
    }

    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);
      
    if(map)free(map);
    if(image)free(image);
      
    pthread_rwlock_rdlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    return STATUS_WORKING;
  }

  if(pl->image_task==1){ // scale first dim
    int next = pl->image_next++;
    if(pl->image_waiting==0)return STATUS_BUSY;
    pl->image_waiting--;

    if(new_w > old_w){
      // y then x
      pthread_mutex_unlock(pl->status_m);
      fast_scale_imagey(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->image_incomplete==0){

	  pl->image_task=-1;
	  pthread_mutex_unlock(pl->status_m);
	  fast_scale_map(map,new_w,newx,oldx);
	  pthread_mutex_lock(pl->status_m);

	  if(p->comp_serialno == serialno){
	    pl->image_waiting=new_h;
	    pl->image_incomplete=new_h;
	    pl->image_task=2;
	    pl->image_next=0;
	  }
	}
      }
    }else{
      // x then y
      pthread_mutex_unlock(pl->status_m);
      fast_scale_imagex(olddata+next*old_w,newx,oldx,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->image_incomplete==0){

	  pl->image_task=-1;
	  pthread_mutex_unlock(pl->status_m);
	  fast_scale_map(map,new_h,newy,oldy);
	  pthread_mutex_lock(pl->status_m);

	  if(p->comp_serialno == serialno){
	    pl->image_waiting=new_w;
	    pl->image_incomplete=new_w;
	    pl->image_task=2;
	    pl->image_next=0;
	  }
	}
      }
    }
    return STATUS_WORKING;    
  }

  if(pl->image_task==2){ // scale first dim
    int next = pl->image_next++;
    if(pl->image_waiting==0)return STATUS_BUSY;
    pl->image_waiting--;

    if(new_w > old_w){
      // now x
      pthread_mutex_unlock(pl->status_m);
      fast_scale_imagex(newdata+next*new_w,newx,oldx,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->image_incomplete==0)
	  pl->image_task=3;
      }
    }else{
      // now y
      pthread_mutex_unlock(pl->status_m);
      fast_scale_imagey(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->image_incomplete==0)
	  pl->image_task=3;
      }
    }
    return STATUS_WORKING;    
  }

  if(pl->image_task==3){ // commit new data 
    int *flags = NULL;
    int *map = NULL;

    unsigned char   *xdelA = NULL;
    unsigned char   *xdelB = NULL;
    int             *xnumA = NULL;
    int             *xnumB = NULL;
    float            xscalemul;
    unsigned char   *ydelA = NULL;
    unsigned char   *ydelB = NULL;
    int             *ynumA = NULL;
    int             *ynumB = NULL;
    float            yscalemul;

    pl->image_task = -1;
    
    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);

    flags = calloc(new_h,sizeof(*newflags));

    xdelA = calloc(pw,sizeof(*xdelA));
    xdelB = calloc(pw,sizeof(*xdelB));
    xnumA = calloc(pw,sizeof(*xnumA));
    xnumB = calloc(pw,sizeof(*xnumB));
    xscalemul = slow_scale_map(newx, pl->pending_data_x, xdelA, xdelB, xnumA, xnumB, 17);
    
    ydelA = calloc(ph,sizeof(*ydelA));
    ydelB = calloc(ph,sizeof(*ydelB));
    ynumA = calloc(ph,sizeof(*ynumA));
    ynumB = calloc(ph,sizeof(*ynumB));
    yscalemul = slow_scale_map(newy, pl->pending_data_y, ydelA, ydelB, ynumA, ynumB, 15);
 
    pthread_rwlock_wrlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    if(p->comp_serialno == serialno){
      pl->image_x = p->bg->image_x;
      pl->image_y = p->bg->image_y;

      swapp(&flags,&pl->imageflags);
      swapp(&map,&pl->map);

      swapp(&xdelA,&pl->resample_xdelA);
      swapp(&xdelB,&pl->resample_xdelB);
      swapp(&xnumA,&pl->resample_xnumA);
      swapp(&xnumB,&pl->resample_xnumB);
      swapp(&ydelA,&pl->resample_ydelA);
      swapp(&ydelB,&pl->resample_ydelB);
      swapp(&ynumA,&pl->resample_ynumA);
      swapp(&ynumB,&pl->resample_ynumB);
      pl->resample_xscalemul = xscalemul;
      pl->resample_yscalemul = yscalemul;

      pl->image_task = 4;
      pl->image_waiting=0;
      pl->image_incomplete=0;
    }
      
    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);

    if(map)free(map);
    if(flags)free(flags);

    if(xdelA)free(xdelA);
    if(xdelB)free(xdelB);
    if(xnumA)free(xnumA);
    if(xnumB)free(xnumB);
    if(ydelA)free(ydelA);
    if(ydelB)free(ydelB);
    if(ynumA)free(ynumA);
    if(ynumB)free(ynumB);

    pthread_rwlock_rdlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);
    return STATUS_WORKING;
  }

  return STATUS_IDLE;
}

// called from worker thread
static int data_resize(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  sv_scalespace_t newx = pl->pending_data_x;
  sv_scalespace_t newy = pl->pending_data_y;
  sv_scalespace_t oldx = pl->data_x;
  sv_scalespace_t oldy = pl->data_y;
  int new_w = newx.pixels;
  int new_h = newy.pixels;
  int old_w = oldx.pixels;
  int old_h = oldy.pixels;
  int serialno = p->comp_serialno;

  if(pl->data_task==-1) return STATUS_BUSY;

  if(pl->data_task==0){ // realloc
    float *old_data = NULL;
    float *data = NULL;
    int *old_map = NULL;
    int *map = NULL;
    pl->data_task = -1;

    // drop locks while alloc()ing/free()ing
    
    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);

    data = calloc(new_w*new_h,sizeof(*data));
    map = calloc(max(new_h,new_w),sizeof(*map));
    if(new_w > old_w){
      fast_scale_map(map,new_h,newy,oldy);
    }else{
      fast_scale_map(map,old_w,newx,oldx);
    }

    pthread_rwlock_wrlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    if(p->comp_serialno == serialno){

      old_data = pl->pending_data;
      old_map = pl->map;
      pl->pending_data = data;
      pl->map = map;
      pl->data_task = 1;
      pl->data_next=0;
      
      if(new_w > old_w){
	// y then x
	pl->data_waiting=old_w;
	pl->data_incomplete=old_w;
      }else{
	// x then y
	pl->data_waiting=old_h;
	pl->data_incomplete=old_h;
      }      
      data = NULL;
      map = NULL;
    }

    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);
      
    if(old_map)free(old_map);
    if(map)free(map);
    if(old_data)free(old_data);
    if(data)free(data);
      
    pthread_rwlock_rdlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    return STATUS_WORKING;
  }

  if(pl->data_task==1){ // scale first dim
    int next = pl->data_next++;
    if(pl->data_waiting==0)return STATUS_BUSY;
    pl->data_waiting--;

    if(new_w > old_w){
      // y then x
      pthread_mutex_unlock(pl->status_m);
      fast_scale_datay(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->data_incomplete==0){

	  pl->data_task=-1;
	  pthread_mutex_unlock(pl->status_m);
	  fast_scale_map(map,new_w,newx,oldx);
	  pthread_mutex_lock(pl->status_m);

	  if(p->comp_serialno == serialno){
	    pl->data_waiting=new_h;
	    pl->data_incomplete=new_h;
	    pl->data_task=2;
	    pl->data_next=0;
	  }
	}
      }
    }else{
      // x then y
      pthread_mutex_unlock(pl->status_m);
      fast_scale_datax(olddata+next*old_w,newx,oldx,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->data_incomplete==0){

	  pl->data_task=-1;
	  pthread_mutex_unlock(pl->status_m);
	  fast_scale_map(map,new_h,newy,oldy);
	  pthread_mutex_lock(pl->status_m);

	  if(p->comp_serialno == serialno){
	    pl->data_waiting=new_w;
	    pl->data_incomplete=new_w;
	    pl->data_task=2;
	    pl->data_next=0;
	  }
	}
      }
    }
    return STATUS_WORKING;    
  }

  if(pl->data_task==2){ // scale first dim
    int next = pl->data_next++;
    if(pl->data_waiting==0)return STATUS_BUSY;
    pl->data_waiting--;

    if(new_w > old_w){
      // now x
      pthread_mutex_unlock(pl->status_m);
      fast_scale_datax(newdata+next*new_w,newx,oldx,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->data_incomplete==0)
	  pl->data_task=3;
      }
    }else{
      // now y
      pthread_mutex_unlock(pl->status_m);
      fast_scale_datay(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno){
	if(--pl->data_incomplete==0)
	  pl->data_task=3;
      }
    }
    return STATUS_WORKING;    
  }

  if(pl->data_task==3){ // commit new data 
    int *map = NULL;

    pl->data_task = -1;

    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);

    pthread_rwlock_wrlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    if(p->comp_serialno == serialno){
      pl->data_x = p->bg->data_x;
      pl->data_y = p->bg->data_y;
      pl->data_task = 4;
      pl->data_waiting = new_h;
      pl->data_incomplete = new_h;
      map = pl->map;
      pl->map = NULL;
    }
      
    pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);

    if(map)free(map);

    pthread_rwlock_rdlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);
    return STATUS_WORKING;
  }

  return STATUS_IDLE;
}

// called from worker thread
static int image_work(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  
  int h = pl->image_y.pixels;
  int w = pl->image_x.pixels;
  int last = pl->image_next;
  int mapno = pl->map_serialno;
  int i;
  sv_ucolor_t work[w];

  if(pl->waiting == 0) return STATUS_IDLE;
  
  do{
    i = pl->image_next;
    pl->image_next++;
    if(pl->image_next>=h)pl->image_next=0;

    if(pl->image_flags[i]){
      pl->image_flags[i]=0;
      pl->waiting--;
      pthread_mutex_unlock(pl->status_m);
      
      map_one_line(pl,p,i,work);

      pthread_mutex_lock(pl->status_m);
      if(p->comp_serialno == serialno &&
	 pl->map_serialno == mapno){
	
	p->bg->image_flags[i] = 1;

	if(pl->incomplete-- == 0){
	  p->bg_render = 1;
	  p->map_render = 0;
	}
      }
      return STATUS_WORKING;
    }
  }while(i!=last);
  
  // shouldn't get here...
  pl->waiting = 0;
  fprintf(stderr,"sushivision: image render found no work despite status flags\n");
  return STATUS_IDLE;
}

// called from worker thread
static int data_work(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;


}

// called from GTK/API
static void plane_remap(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;


}

// called from GTK/API
static void plane_free(sv_plane_t *pl){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;

  if(pl){
    if(pl->data)free(pl->data);
    if(pl->image)free(pl->image);
    if(pl->image_flags)free(pl->image_flags);
    
    if(pl->resample_xdelA)free(pl->resample_xdelA);
    if(pl->resample_xdelB)free(pl->resample_xdelB);
    if(pl->resample_xnumA)free(pl->resample_xnumA);
    if(pl->resample_xnumB)free(pl->resample_xnumB);
    if(pl->resample_xscalemul)free(pl->resample_xscalemul);
    
    if(pl->resample_ydelA)free(pl->resample_ydelA);
    if(pl->resample_ydelB)free(pl->resample_ydelB);
    if(pl->resample_ynumA)free(pl->resample_ynumA);
    if(pl->resample_ynumB)free(pl->resample_ynumB);
    if(pl->resample_yscalemul)free(pl->resample_yscalemul);
    
    if(pl->mapping)_sv_mapping_free(pl->mapping);
    if(pl->scale)_sv_slider_free(pl->scale);
    if(pl->range_pulldown)gtk_widget_destroy(pl->range_pulldown);
  
    free(pl);
  }
}

sv_plane_t *sv_plane_2d_new(){
  sv_plane_2d_t *ret = calloc(1,sizeof(*ret));
  ret->recompute_setup = recompute_setup;
  ret->image_resize = image_resize;
  ret->data_resize = data_resize;
  ret->image_work = image_work;
  ret->data_work = data_work;
  ret->plane_remap = plane_remap;
  ret->plane_free = plane_free;
  return (sv_plane_t *)ret;
}


// enter unlocked
static void _sv_planez_compute_line(sv_panel_t *p, 
				    _sv_plane2d_t *z,

				    int serialno,

				    int dw,
				    int y,
				    int x_d, 
				    _sv_scalespace_t sxi,
				    double *dim_vals, 
				    _sv_bythread_cache_2d_t *c){

  _sv_panel2d_t *p2 = p->subtype->p2;
  int i,j;
  
  /* cache access is unlocked because the cache is private to this
     worker thread */

  for(j=0;j<dw;j++){
    double *fout = c->fout;
    sv_func_t **f = p2->used_function_list;
    int *obj_y_off = p2->y_fout_offset;
    int *onum = p2->y_obj_to_panel;
    
    /* by function */
    dim_vals[x_d] = _sv_scalespace_value(&sxi,j);   
    for(i=0;i<p2->used_functions;i++){
      (*f)->callback(dim_vals,fout);
      fout += (*f)->outputs;
      f++;
    }
    
    /* process function output by plane type/objective */
    /* 2d panels currently only care about the Y output value */
    
    /* slider map */
    for(i=0;i<p2->y_obj_num;i++){
      float val = (float)_sv_slider_val_to_del(p2->range_scales[*onum++], c->fout[*obj_y_off++]);
      if(isnan(val)){
	c->y_map[i][j] = -1;
      }else{
	if(val<0)val=0;
	if(val>1)val=1;
	c->y_map[i][j] = rint(val * (256.f*256.f*256.f));
      }
    }
  }

  gdk_lock ();
  if(p->private->plot_serialno == serialno){
    for(j=0;j<p2->y_obj_num;j++){
      int *d = p2->y_map[j] + y*dw;
      int *td = c->y_map[j];
      
      memcpy(d,td,dw*sizeof(*d));
      
    }
  }
  gdk_unlock ();
}

typedef struct{
  double x;
  double y;
  double z;
} compute_result;

// used by the legend code. this lets us get away with having only a mapped display pane
// call with lock
static void _sv_panel2d_compute_point(sv_panel_t *p,sv_obj_t *o, double x, double y, compute_result *out){
  double dim_vals[_sv_dimensions];
  int i,j;
  int pflag=0;
  int eflag=0;

  // fill in dimensions
  int x_d = p->private->x_d->number;
  int y_d = p->private->y_d->number;

  for(i=0;i<_sv_dimensions;i++){
    sv_dim_t *dim = _sv_dimension_list[i];
    dim_vals[i]=dim->val;
  }

  gdk_unlock ();

  dim_vals[x_d] = x;
  dim_vals[y_d] = y;

  *out = (compute_result){NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN};

  // compute
  for(i=0;i<_sv_functions;i++){
    sv_func_t *f = _sv_function_list[i];
    int compflag = 0;
    double fout[f->outputs];
    double val;

    // compute and demultiplex output
    for(j=0;j<o->outputs;j++){
      if(o->function_map[j] == i){

	if(!compflag) f->callback(dim_vals,fout);
	compflag = 1;
	
	val = fout[o->output_map[j]];
	switch(o->output_types[j]){
	case 'X':
	  out->x = val;
	  break;
	case 'Y':
	  out->y = val;
	  break;
	case 'Z':
	  out->z = val;
	  break;
	case 'E':
	  if(eflag)
	    out->e2 = val;
	  else
	    out->e1 = val;
	  eflag = 1;
	  break;
	case 'P':
	  if(pflag)
	    out->p2 = val;
	  else
	    out->p1 = val;
	  pflag = 1;
	  break;
	case 'M':
	  out->m = val;
	  break;
	}
      }
    }
  }
  gdk_lock ();

}

// enter with lock; returns zero if thread should sleep / get distracted
static int _sv_panel2d_remap(sv_panel_t *p, _sv_bythread_cache_2d_t *thread_cache){
  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot = PLOT(p->private->graph);

  if(!plot) goto abort;
  int ph = plot->y.pixels;
  int pw = plot->x.pixels;
  
  int plot_serialno = p->private->plot_serialno; 
  int map_serialno = p->private->map_serialno; 

  /* brand new remap indicated by the generic progress indicator being set to 0 */
  if(p->private->map_progress_count == 0){

    p->private->map_progress_count = 1; // 'in progress'
    p->private->map_complete_count = p2->y_obj_num * ph; // count down to 0; 0 indicates completion

    // set up Y plane rendering
    p2->y_next_plane = 0;
    p2->y_next_line = 0;

    // bg mix
    p2->bg_next_line = 0;
    p2->bg_first_line = ph;
    p2->bg_last_line = 0;

    if(!p2->partial_remap)
      _sv_panel2d_mark_map_full(p);
  }

  /* by plane, by line; each plane renders independently */
  /* Y planes */
  if(p2->y_planetodo){
    if(p2->y_next_plane < p2->y_obj_num){
      int status = _sv_panel2d_resample_render_y_plane_line(p, thread_cache, 
							    plot_serialno, map_serialno, 
							    p2->y_next_plane);
      if(status == -1) goto abort;
      if(status == 1){
	p2->y_next_plane++;
	p2->y_next_line = 0;
      }
      return 1;
    }
  }else{
    p->private->map_complete_count = 0;
  }

  /* renders have been completely dispatched, but are they complete? */
  /* the below is effectively a a thread join */
  if(p2->bg_next_line == 0){

    // join still needs to complete....
    if(p->private->map_complete_count){
      // nonzero complete count, not finished.  returning zero will cause
      // this worker thread to sleep or go on to do other things.
      return 0; 
    }else{
      // zero complete count, the planes are done; we can begin
      // background render.  At least one thread is guaranteed to get
      // here, which is enough; we can now wake the others [if they were
      // asleep] and have them look for work here. */
      p->private->map_complete_count = ph; // [ph] lines to render in bg plane
      p2->bg_next_line = 0;

      _sv_wake_workers();
    }
  }

  /* mix new background, again line by line */
  if(p2->bg_next_line < ph){
    int status = _sv_panel2d_render_bg_line(p, plot_serialno, map_serialno);
    if(status == -1) goto abort;
    if(p->private->map_complete_count)return status;
  }else
    return 0; // nothing left to dispatch

  // entirely finished.

  // remap completed; flush background to screen
  _sv_plot_expose_request_partial (plot,0,p2->bg_first_line,
			       pw,p2->bg_last_line - p2->bg_first_line);
  gdk_flush();

  // clean bg todo list
  memset(p2->bg_todo,0,ph*sizeof(*p2->bg_todo));

  // clear 'panel in progress' flag
  p2->partial_remap = 0;
  _sv_panel_clean_map(p);
  return 0;

 abort:
  // reset progress to 'start over'
  return 1;
}

// call while locked 
static void _sv_panel2d_mark_map_line_y(sv_panel_t *p, int line){
  // determine all panel lines this y data line affects
  _sv_panel2d_t *p2 = p->subtype->p2;
  int ph = p2->y.pixels;
  int pw = p2->x.pixels;
  int dw = p2->x_v.pixels;
  int dh = p2->y_v.pixels;
  int i,j;

  p2->partial_remap = 1;

  if(ph!=dh || pw!=dw){
    /* resampled row computation; may involve multiple data rows */
    if(p2->y_planetodo){
      _sv_panel2d_resample_helpers_manage_y(p);
      
      for(i=0;i<ph;i++)
	if(p2->ynumA[i]<=line &&
	   p2->ynumB[i]>line){
	  
	  for(j=0;j<p2->y_obj_num;j++)
	    if(p2->y_planetodo[j])
	      p2->y_planetodo[j][i]=1;
      }
    }
  }else{
    if(p2->y_planetodo)
      if(line>=0 && line<ph)
	for(j=0;j<p2->y_obj_num;j++)
	  if(p2->y_planetodo[j])
	    p2->y_planetodo[j][line]=1;
  }
}

static int _v_swizzle(int y, int height){
  int yy = height >> 5;
  if(y < yy)
    return (y<<5)+31;

  y -= yy;
  yy = (height+16) >> 5;
  if(y < yy)
    return (y<<5)+15;

  y -= yy;
  yy = (height+8) >> 4;
  if(y < yy)
    return (y<<4)+7;

  y -= yy;
  yy = (height+4) >> 3;
  if(y < yy)
    return (y<<3)+3;

  y -= yy;
  yy = (height+2) >> 2;
  if(y < yy)
    return (y<<2)+1;

  y -= yy;
  return y<<1;
}

void _sv_panel2d_maintain_cache(sv_panel_t *p, _sv_bythread_cache_2d_t *c, int w){
  _sv_panel2d_t *p2 = p->subtype->p2;
  
  /* toplevel initialization */
  if(c->fout == 0){
    int i,count=0;
    
    /* allocate output temporary buffer */
    for(i=0;i<p2->used_functions;i++){
      int fnum = p2->used_function_list[i]->number;
      sv_func_t *f = _sv_function_list[fnum];
      count += f->outputs;
    }
    c->fout = calloc(count, sizeof(*c->fout));

    /* objective line buffer index */
    c->y_map = calloc(p2->y_obj_num,sizeof(*c->y_map));
    for(i=0;i<p2->y_obj_num;i++)
      c->y_map[i] = calloc(w,sizeof(**c->y_map));
    c->storage_width = w;
  }
  
  /* anytime the data width changes */
  if(c->storage_width != w){
    int i;
    c->storage_width = w;
    
    for(i=0;i<p2->y_obj_num;i++)
      c->y_map[i] = realloc(c->y_map[i],w*sizeof(**c->y_map));

  }
}


// subtype entry point for plot remaps; lock held
static int _sv_panel2d_map_redraw(sv_panel_t *p, _sv_bythread_cache_t *c){
  return _sv_panel2d_remap(p,&c->p2);
}

// subtype entry point for legend redraws; lock held
static int _sv_panel2d_legend_redraw(sv_panel_t *p){
  _sv_plot_t *plot = PLOT(p->private->graph);

  if(p->private->legend_progress_count)return 0;
  p->private->legend_progress_count++;
  _sv_panel2d_update_legend(p);
  _sv_panel_clean_legend(p);

  gdk_unlock();
  _sv_plot_draw_scales(plot);
  gdk_lock();

  _sv_plot_expose_request(plot);
  return 1;
}

// subtype entry point for recomputation; lock held
static int _sv_panel2d_compute(sv_panel_t *p,
			       _sv_bythread_cache_t *c){

  _sv_panel2d_t *p2 = p->subtype->p2;
  _sv_plot_t *plot;
  
  int pw,ph,dw,dh,i,d;
  int serialno;
  double x_min, x_max;
  double y_min, y_max;
  int x_d=-1, y_d=-1;
  _sv_scalespace_t sx,sx_v,sx_i;
  _sv_scalespace_t sy,sy_v,sy_i;

  plot = PLOT(p->private->graph);
  pw = plot->x.pixels;
  ph = plot->y.pixels;
  
  x_d = p->private->x_d->number;
  y_d = p->private->y_d->number;

  // beginning of computation init
  if(p->private->plot_progress_count==0){
    int remapflag = 0;

    _sv_scalespace_t old_x = p2->x;
    _sv_scalespace_t old_y = p2->y;
    _sv_scalespace_t old_xv = p2->x_v;
    _sv_scalespace_t old_yv = p2->y_v;

    // generate new scales
    _sv_dim_scales(p->private->x_d, 
		   p->private->x_d->bracket[0],
		   p->private->x_d->bracket[1],
		   pw,pw * p->private->oversample_n / p->private->oversample_d,
		   plot->scalespacing,
		   p->private->x_d->legend,
		   &sx,
		   &sx_v,
		   &sx_i);
    _sv_dim_scales(p->private->y_d, 
		   p->private->y_d->bracket[1],
		   p->private->y_d->bracket[0],
		   ph,ph * p->private->oversample_n / p->private->oversample_d,
		   plot->scalespacing,
		   p->private->y_d->legend,
		   &sy,
		   &sy_v,
		   &sy_i);
    
    p2->x = sx;
    p2->x_v = sx_v;
    p2->x_i = sx_i;
    p2->y = sy;
    p2->y_v = sy_v;
    p2->y_i = sy_i;

    plot->x = sx;
    plot->y = sy;
    plot->x_v = sx_v;
    plot->y_v = sy_v;

    p->private->plot_progress_count++;
    p->private->plot_serialno++; // we're about to free the old data rectangles

    // realloc/fast scale the current data contents if appropriate
    if(memcmp(&sx_v,&old_xv,sizeof(sx_v)) || memcmp(&sy_v,&old_yv,sizeof(sy_v))){
      
      // maintain data planes
      for(i=0;i<p2->y_obj_num;i++){
	// allocate new storage
	int *newmap = calloc(sx_v.pixels*sy_v.pixels,sizeof(*newmap));
	int *oldmap = p2->y_map[i];
	int j;
	
	for(j=0;j<sx_v.pixels*sy_v.pixels;j++)
	  newmap[j]=-1;
	
	// zoom scale data in map planes as placeholder for render
	if(oldmap){
	  _sv_panel2d_fast_scale(p->private->spinner,newmap, sx_v, sy_v,
				 oldmap,old_xv, old_yv);
	  free(oldmap);
	}
	p2->y_map[i] = newmap; 
      }
      remapflag = 1;
    }

    // realloc render planes if appropriate
    if(memcmp(&sx,&old_x,sizeof(sx)) || memcmp(&sy,&old_y,sizeof(sy))){
      for(i=0;i<p2->y_obj_num;i++){

	// y planes
	if(p2->y_planes[i])
	  free(p2->y_planes[i]);
	p2->y_planes[i] = calloc(sx.pixels*sy.pixels,sizeof(**p2->y_planes));

	// todo lists
	if(p2->y_planetodo[i])
	  free(p2->y_planetodo[i]);
	p2->y_planetodo[i] = calloc(sy.pixels,sizeof(**p2->y_planetodo));
	
      }

      if(p2->bg_todo)
	free(p2->bg_todo);
      p2->bg_todo=calloc(ph,sizeof(*p2->bg_todo));
      
      remapflag = 1;
    }

    if(remapflag){
      _sv_panel2d_mark_map_full(p);
      _sv_panel_dirty_map(p);

      gdk_unlock ();      
      _sv_plot_draw_scales(plot); // this should happen outside lock
      gdk_lock ();      
    }

    _sv_map_set_throttle_time(p); // swallow the first 'throttled' remap which would only be a single line;

    return 1;
  }else{
    sx = p2->x;
    sx_v = p2->x_v;
    sx_i = p2->x_i;
    sy = p2->y;
    sy_v = p2->y_v;
    sy_i = p2->y_i;
    serialno = p->private->plot_serialno; 
  }

  dw = sx_v.pixels;
  dh = sy_v.pixels;

  if(p->private->plot_progress_count>dh) return 0;

  _sv_panel2d_maintain_cache(p,&c->p2,dw);
  
  d = p->dimensions;

  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[_sv_dimensions];
  int y = _v_swizzle(p->private->plot_progress_count-1,dh);
  p->private->plot_progress_count++;

  x_min = _sv_scalespace_value(&p2->x_i,0);
  x_max = _sv_scalespace_value(&p2->x_i,dw);

  y_min = _sv_scalespace_value(&p2->y_i,0);
  y_max = _sv_scalespace_value(&p2->y_i,dh);

  // Initialize local dimension value array
  for(i=0;i<_sv_dimensions;i++){
    sv_dim_t *dim = _sv_dimension_list[i];
    dim_vals[i]=dim->val;
  }

  /* unlock for computation */
  gdk_unlock ();
    
  dim_vals[y_d]=_sv_scalespace_value(&sy_i, y);
  _sv_panel2d_compute_line(p, serialno, dw, y, x_d, sx_i, dim_vals, &c->p2);

  gdk_lock ();

  if(p->private->plot_serialno == serialno){
    p->private->plot_complete_count++;
    _sv_panel2d_mark_map_line_y(p,y);
    if(p->private->plot_complete_count>=dh){ 
      _sv_panel_dirty_map(p);
      _sv_panel_dirty_legend(p);
      _sv_panel_clean_plot(p);
    }else
      _sv_panel_dirty_map_throttled(p); 
  }

  return 1;
}

static void _sv_panel2d_realize(sv_panel_t *p){
  _sv_panel2d_t *p2 = p->subtype->p2;
  int i;

  _sv_undo_suspend();

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sv_clean_exit), (void *)SIGINT);
 
  // add border to sides with hbox/padding
  GtkWidget *borderbox =  gtk_hbox_new(0,0);
  gtk_container_add (GTK_CONTAINER (p->private->toplevel), borderbox);

  // main layout vbox
  p->private->topbox = gtk_vbox_new(0,0);
  gtk_box_pack_start(GTK_BOX(borderbox), p->private->topbox, 1,1,4);
  gtk_container_set_border_width (GTK_CONTAINER (p->private->toplevel), 1);

  /* spinner, top bar */
  {
    GtkWidget *hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), hbox, 0,0,0);
    gtk_box_pack_end(GTK_BOX(hbox),GTK_WIDGET(p->private->spinner),0,0,0);
  }

  /* plotbox, graph */
  {
    p->private->graph = GTK_WIDGET(_sv_plot_new(_sv_panel2d_recompute_callback,p,
						(void *)(void *)_sv_panel2d_crosshairs_callback,p,
						_sv_panel2d_box_callback,p,0)); 
    p->private->plotbox = p->private->graph;
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p->private->plotbox, 1,1,2);
  }

  /* obj box */
  {
    p2->obj_table = gtk_table_new(p->objectives, 5, 0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p2->obj_table, 0,0,1);

    /* objective sliders */
    p2->range_scales = calloc(p->objectives,sizeof(*p2->range_scales));
    p2->range_pulldowns = calloc(p->objectives,sizeof(*p2->range_pulldowns));
    p2->alphadel = calloc(p->objectives,sizeof(*p2->alphadel));
    p2->mappings = calloc(p->objectives,sizeof(*p2->mappings));
    for(i=0;i<p->objectives;i++){
      GtkWidget **sl = calloc(3,sizeof(*sl));
      sv_obj_t *o = p->objective_list[i].o;
      int lo = o->scale->val_list[0];
      int hi = o->scale->val_list[o->scale->vals-1];
      
      /* label */
      GtkWidget *label = gtk_label_new(o->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p2->obj_table),label,0,1,i,i+1,
		       GTK_FILL,0,8,0);
      
      /* mapping pulldown */
      {
	GtkWidget *menu=_gtk_combo_box_new_markup();
	int j;
	for(j=0;j<_sv_mapping_names();j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), _sv_mapping_name(j));
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (_sv_panel2d_mapchange_callback), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p2->obj_table),menu,4,5,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,0,0);
	p2->range_pulldowns[i] = menu;
      }

      /* the range mapping slices/slider */ 
      sl[0] = _sv_slice_new(_sv_panel2d_map_callback,p->objective_list+i);
      sl[1] = _sv_slice_new(_sv_panel2d_map_callback,p->objective_list+i);
      sl[2] = _sv_slice_new(_sv_panel2d_map_callback,p->objective_list+i);
      
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[0],1,2,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[1],2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[2],3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      p2->range_scales[i] = _sv_slider_new((_sv_slice_t **)sl,3,o->scale->label_list,o->scale->val_list,
				       o->scale->vals,_SV_SLIDER_FLAG_INDEPENDENT_MIDDLE);
      gtk_table_set_col_spacing(GTK_TABLE(p2->obj_table),3,5);

      _sv_slice_thumb_set((_sv_slice_t *)sl[0],lo);
      _sv_slice_thumb_set((_sv_slice_t *)sl[1],lo);
      _sv_slice_thumb_set((_sv_slice_t *)sl[2],hi);
      _sv_mapping_setup(&p2->mappings[i],0.,1.,0);
      _sv_slider_set_gradient(p2->range_scales[i], &p2->mappings[i]);
    }
  }

  /* dims */
  {
    p2->dim_table = gtk_table_new(p->dimensions,4,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), p2->dim_table, 0,0,4);
    
    GtkWidget *first_x = NULL;
    GtkWidget *first_y = NULL;
    GtkWidget *pressed_y = NULL;
    p->private->dim_scales = calloc(p->dimensions,sizeof(*p->private->dim_scales));
    p2->dim_xb = calloc(p->dimensions,sizeof(*p2->dim_xb));
    p2->dim_yb = calloc(p->dimensions,sizeof(*p2->dim_yb));
    
    for(i=0;i<p->dimensions;i++){
      sv_dim_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->legend);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p2->dim_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* x/y radio buttons */
      if(!(d->flags & SV_DIM_NO_X)){
	if(first_x)
	  p2->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
	else{
	  first_x = p2->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_xb[i],1,2,i,i+1,
			 GTK_SHRINK,0,3,0);
      }
      
      if(!(d->flags & SV_DIM_NO_Y)){
	if(first_y)
	  p2->dim_yb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_y),"Y");
	else
	  first_y = p2->dim_yb[i] = gtk_radio_button_new_with_label(NULL,"Y");
	if(!pressed_y && p2->dim_xb[i]!=first_x){
	  pressed_y = p2->dim_yb[i];
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_yb[i],2,3,i,i+1,
			 GTK_SHRINK,0,3,0);
      }
      
      p->private->dim_scales[i] = 
	_sv_dim_widget_new(p->dimension_list+i,_sv_panel2d_center_callback,_sv_panel2d_bracket_callback);
      
      gtk_table_attach(GTK_TABLE(p2->dim_table),
		       p->private->dim_scales[i]->t,
		       3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
    }
    for(i=0;i<p->dimensions;i++){
      if(p2->dim_xb[i])
	g_signal_connect (G_OBJECT (p2->dim_xb[i]), "toggled",
			  G_CALLBACK (_sv_panel2d_dimchange_callback), p);
      if(p2->dim_yb[i])
	g_signal_connect (G_OBJECT (p2->dim_yb[i]), "toggled",
			  G_CALLBACK (_sv_panel2d_dimchange_callback), p);
    }
  }

  _sv_panel2d_update_xysel(p);

  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_realize(GTK_WIDGET(p->private->spinner));
  gtk_widget_show_all(p->private->toplevel);
  _sv_panel2d_update_xysel(p); // yes, this was already done; however,
			       // gtk clobbered the event setup on the
			       // insensitive buttons when it realized
			       // them.  This call will restore them.

  _sv_undo_resume();
}

