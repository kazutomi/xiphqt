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
		       int dw, int dh,
		       int iw, int ih,
		       void (*mapfunc)(int,int, _sv_lcolor_t *), 
		       int i){
  
  sv_ucolor_t *image = pl->image;
  sv_ccolor_t *cwork = work;

  if(ih!=dh || iw!=dw){
    /* resampled row computation; may involve multiple data rows */

    float idel = pl->resample_yscalemul * pl->resample_xscalemul;
    
    /* by column */
    /* XXXXX by row should be more efficient... */
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

    // although the slider map is only locked against heap changes,
    // the calculation is well formed such that data inconsistencies
    // are guaranteed to be cosmetic only and short lived.
    for(j=0;j<lh*dw;j++)
      data[j] = _sv_slidermap_to_mapdel(pl->scale, in_data[j])*65535.f;
          
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

int _sv_plane_recompute_setup_common(sv_plane_t *pl){
  sv_plane_common_t *c = &pl->c;
  sv_panel_t *p = c->panel;
  int w = p->bg->image_x->pixels;
  int h = p->bg->image_y->pixels;
  sv_scalespace_t pending_scales[pl->c.axes];
  double pending_input[pl->o->inputs];
  int flag=0;
  
  // generate new pending scales
  for(i=0;i<pl->c.axes;i++){
    int axisnum = c->axis_list[i];
    int dimnum = p->axis_dims[axisnum];

    // check that dims are actually set 
    // should only come up if dimensions are declared to provide an
    // axis, but there are too few dims to actually populate all
    // declared axes.
    if(dimnum<0){
      fprintf(stderr,"sushivision: Panel \"%s\" has not set the %s axis\n"
	      "\tfor objective \"%s\".\n",
	      p->name,p->axis_names[axisnum],o->name);
      pending_scales[i] = {0};
    }else{

      switch(axisnum){
      case 0: // X
	pending_scales[i] = 
	  _sv_dim_datascale(p->dim_data+dimnum, p->bg->image_x, 
			    w * p->oversample_n / p->oversample_d, 0);
	break;
	
      case 1: // Y
	pending_scales[i] = 
	  _sv_dim_datascale(p->dim_data+dimnum, p->bg->image_y, 
			    h * p->oversample_n / p->oversample_d, 1);
	break;
	
      case 2: // Z
	fprintf(stderr,"Z axis unimplemented!\n");
	pending_scales[i] = {0};
	break;
	
      default: // all auxiliary scales 
	fprintf(stderr,"auxiliary axes unimplemented!\n");
	pending_scales[i] = {0};
	break;
      }
    }
  }

  // precompute non-iteration values in function input vector
  for(i=0;i<pl->o->inputs;i++){
    int dim = pl->o->input_dims[i]; 
    for(j=0;j<pl->c.axes;j++)
      if(dim == p->axis_dims[pl->c.axis_list[j]])
	pending_input[i]=NAN;
      else
	pending_input[i]=p->dim_data[dim].val;
  }
	  
  // Do we really need to recompute?  Check dims and axes for changes.
  // Any changes, we must recompute.  

  // check that the axes dimensions have not changed
  //for(i=0;i<pl->c.axes;i++)
  //if(pl->c.axis_dims[i] != p->axis_dims[pl->c.axis_list[i]]){
  //  flag=1;
  //  break;
  //}

  // check that the axes scales have not changed
  if(!flag)
    for(i=0;i<pl->c.axes;i++)
      if(_sv_scalecmp(c->data_scales+i,pending_scales+i)){
	flag = 1;
	break;
      }
  
  // check whether dimension values have changed.
  // the fact that axis dim values are NAN allows this to double as a
  // check that the axes dims have not changed.
  if(!flag)
    for(i=0;i<pl->o->inputs;i++)
      if(pending_input[i] != c->dim_input[i]){
	flag=1;
	break;
      }

  if(flag){
    memcpy(c->dim_input,pending_input,sizeof(pending_input));
    memcpy(c->pending_data_scales,pending_scales,sizeof(pending_scales));
  }
	
  return flag;
}

int _sv_plane_resample_setup_common(sv_plane_t *in){
  sv_plane_common_t *c = &pl->c;
  sv_panel_t *p = c->panel;

  if(_sv_scalecmp(&c->image_x,&p->bg->image_x) ||
     _sv_scalecmp(&c->image_y,&p->bg->image_y)) return 1;
  return 0;
}

// called from worker thread 
static void recompute_setup(sv_plane_t *in){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  sv_panel_t *p = pl->panel;

  int flag=_sv_plane_recompute_presetup(in);

  // Do we really need to recompute? 
  if(flag){
    pl->data_task=0;
    pl->data_outstanding=0;
    pl->data_next=0;
  }

  // Regardless of recomputation, we check for remap (eg, resizing the
  // plane of a fixed size data set requires a rerender, but not a
  // recompute)

  flag|=_sv_plane_resample_setup_common(in);

  if(flag){
    pl->image_serialno++;
    pl->image_outstanding=0;
    pl->image_task=0;
  }
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
    //pthread_mutex_lock(pl->status_m); don't need two exclusive locks!

    if(p->comp_serialno == serialno){
      
      swapp(&pl->pending_image, &image);
      swapp(&pl->map, &map);
      pl->image_task = 1;
      pl->image_next=0;
      
      if(new_w > old_w){
	// y then x
	pl->image_outstanding=0;
      }else{
	// x then y
	pl->image_outstanding=0;
      }      
      image = NULL;
      map = NULL;
    }

    //pthread_mutex_unlock(pl->status_m);
    pthread_rwlock_unlock(pl->panel_m);
      
    if(map)free(map);
    if(image)free(image);
      
    pthread_rwlock_rdlock(pl->panel_m);
    pthread_mutex_lock(pl->status_m);

    return STATUS_WORKING;
  }

  if(pl->image_task==1){ // scale first dim
    if(new_w > old_w){
      // y then x

      int next = pl->image_next;
      if(next >= oldx){
	if(pl->image_outstanding) return STATUS_BUSY;

	pl->image_task=-1;
	pthread_mutex_unlock(pl->status_m);
	fast_scale_map(map,new_w,newx,oldx);
	pthread_mutex_lock(pl->status_m);
	
	if(p->comp_serialno == serialno){
	  pl->image_task=2;
	  pl->image_next=0;
	}
      }else{

	pl->image_next++;
	pl->image_outstanding++;
	
	pthread_mutex_unlock(pl->status_m);
	fast_scale_imagey(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
	pthread_mutex_lock(pl->status_m);
	
	if(p->comp_serialno == serialno)
	  pl->image_outstanding--;
      }
    }else{
      // x then y

      int next = pl->image_next;
      if(next >= oldy){
	if(pl->image_outstanding) return STATUS_BUSY;
	  
	pl->image_task=-1;
	pthread_mutex_unlock(pl->status_m);
	fast_scale_map(map,new_w,newy,oldy);
	pthread_mutex_lock(pl->status_m);
	
	if(p->comp_serialno == serialno){
	  pl->image_task=2;
	  pl->image_next=0;
	}
      }else{

	pl->image_next++;
	pl->image_outstanding++;

	pthread_mutex_unlock(pl->status_m);
	fast_scale_imagex(olddata+next*old_w,newx,oldx,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)
	  pl->image_outstanding--;
      }
    }
    return STATUS_WORKING;    
  }

  if(pl->image_task==2){ // scale second dim
    if(new_w > old_w){
      // now x

      int next = pl->image_next;
      if(next >= newy){
	if(pl->image_outstanding) return STATUS_BUSY;
	if(p->comp_serialno == serialno)
	  pl->image_task=3;
      }else{
	pthread_mutex_unlock(pl->status_m);
	fast_scale_imagex(newdata+next*new_w,newx,oldx,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)
	  pl->image_outstanding--;
      }

    }else{
      // now y

      int next = pl->image_next;
      if(next >= newx){
	if(pl->image_outstanding) return STATUS_BUSY;
	if(p->comp_serialno == serialno)
	  pl->image_task=3;
      }else{
	pthread_mutex_unlock(pl->status_m);
	fast_scale_imagey(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)
	  pl->image_outstanding--;
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
    //pthread_mutex_lock(pl->status_m);

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
      bg_rerender_full(p);
      pl->image_task = 4;
      pl->image_outstanding=0;
    }
      
    //pthread_mutex_unlock(pl->status_m);
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
    //pthread_mutex_lock(pl->status_m);

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

    //pthread_mutex_unlock(pl->status_m);
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
    if(new_w > old_w){
      // y then x

      int next = pl->data_next;
      if(next >= oldx){
	if(pl->data_outstanding)return STATUS_BUSY;

	pl->data_task=-1;
	pthread_mutex_unlock(pl->status_m);
	fast_scale_map(map,new_w,newx,oldx);
	pthread_mutex_lock(pl->status_m);
	
	if(p->comp_serialno == serialno){
	  pl->data_task=2;
	  pl->data_next=0;
	}
      }else{

	pl->data_next++;
	pl->data_outstanding++;

	pthread_mutex_unlock(pl->status_m);
	fast_scale_datay(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)
	  pl->data_outstanding--;
      }
    }else{
      // x then y

      int next = pl->data_next;
      if(next >= oldy){
	if(pl->data_outstanding)return STATUS_BUSY;

	pl->data_task=-1;
	pthread_mutex_unlock(pl->status_m);
	fast_scale_map(map,new_w,newy,oldy);
	pthread_mutex_lock(pl->status_m);
	
	if(p->comp_serialno == serialno){
	  pl->data_task=2;
	  pl->data_next=0;
	}
      }else{

	pl->data_next++;
	pl->data_outstanding++;

	pthread_mutex_unlock(pl->status_m);
	fast_scale_datax(olddata+next*old_w,newx,oldx,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)

	if(--pl->data_incomplete==0)
	  pl->data_outstanding--;
      }
    }
    return STATUS_WORKING;    
  }

  if(pl->data_task==2){ // scale second dim
    if(new_w > old_w){
      // now x

      int next = pl->data_next;
      if(next >= newy){
	if(pl->data_outstanding)return STATUS_BUSY;
	if(p->comp_serialno == serialno)
	  pl->data_task=3;
      }else{
	pl->data_next++;
	pl->data_outstanding++;
	pthread_mutex_unlock(pl->status_m);
	fast_scale_datax(newdata+next*new_w,newx,oldx,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)
	  pl->data_outstanding--;
      }
    }else{
      // now y

      int next = pl->data_next;
      if(next >= newx){
	if(pl->data_outstanding)return STATUS_BUSY;
	if(p->comp_serialno == serialno)
	  pl->data_task=3;
      }else{
	pl->data_next++;
	pl->data_outstanding++;
	pthread_mutex_unlock(pl->status_m);
	fast_scale_datay(olddata+next,newdata+next,new_w,old_w,newy,oldy,pl->map);
	pthread_mutex_lock(pl->status_m);
	if(p->comp_serialno == serialno)
	  pl->data_outstanding--;
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
    //pthread_mutex_lock(pl->status_m);

    if(p->comp_serialno == serialno){
      pl->data_x = pl->pending_data_x;
      pl->data_y = pl->pending_data_y;
      pl->data_task = 4;
      pl->data_outstanding=0;
      map = pl->map;
      pl->map = NULL;
    }
      
    //pthread_mutex_unlock(pl->status_m);
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
  int mapno = pl->image_serialno;
  int i;
  sv_ucolor_t work[w];
  
  if(pl->image_task == 5 && pl->image_outstanding) return STATUS_BUSY;
  if(pl->image_task != 4) return STATUS_IDLE;
  
  do{
    i = pl->image_next;
    pl->image_next++;
    if(pl->image_next>=h)pl->image_next=0;
    
    if(pl->image_flags[i]){
      sv_scalespace_t dx = pl->data_x;
      sv_scalespace_t dy = pl->data_y;
      sv_scalespace_t ix = pl->image_x;
      sv_scalespace_t iy = pl->image_y;
      void (*mapping)(int, int, _sv_lcolor_t *)=mapfunc[pl->image_mapnum];
      pl->image_flags[i]=0;
      pl->image_outstanding++;

      pthread_mutex_unlock(pl->status_m);
      slow_scale(dx,dy,ix,iy,mapping,i);
      pthread_mutex_lock(pl->status_m);
      
      if(pl->image_serialno == mapno){
	pl->image_outstanding--;
	bg_rerender_line(p,i);
      }
      return STATUS_WORKING;
    }
  }while(i!=last);
  
  pl->image_task = 5;
  if(pl->image_outstanding) return STATUS_BUSY;
  return STATUS_IDLE;
}

static int vswizzle(int y, int height){
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

static void data_demultiplex_2d(sv_plane_t *in,sv_panel_t *p,double *output,
				int w, int xoff, int yoff, int n){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  int i,on=pl->o->outputs;
  float *data_line = pl->data+w*yoff+xoff;
  output += pl->data_z_output;

  for(i=0;i<n;i++){
    *data_line++ = *output;
    output+=on;
  }
}

// called from worker thread
static int data_work(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  int serialno = p->comp_serialno;
  int next = pl->data_next;
  int i,j; 

  // each plane is associated with a single objective, however
  // multiple objectives may be associated with a given computation.
  // This is an optimization for dealing with multiple display
  // ojectives drawing from different output values of the exact same
  // input computation.  The plane types sharing a computation may be
  // different, but the input dimension value vector and input axes
  // will be identical.

  // if this is a 'slave' plane in the computation chain, return idle;
  // some other plane is doing the calculation for us.

  if(pl->c.share_prev)return STATUS_IDLE; 
  if(pl->data_task == 4)
    if(next >= pl->data_y.pixels) 
      pl->data_task = 5;
  if(pl->data_task == 5 && pl->data_outstanding) return STATUS_BUSY;
  if(pl->data_task != 4) return STATUS_IDLE;
  pl->data_next++;

  // marshal iterators, dimension value vectors
  int outputs = pl->o->outputs;
  double input[pl->o->inputs];
  int xpos = -1;
  int yline = vswizzle(next);
  sv_scalespace_t dx = pl->data_x;
  int dw = dx.pixels;
  int dh = pl->data_y.pixels;
  int iw = pl->image_x.pixels;
  int ih = pl->image_y.pixels;

  for(i=0;i<pl->o->inputs;i++){
    int dim = pl->o->input_dims[i]; // dim setup in an objective is immutable
    if(dim == p->ydim){
      input[i] = _sv_scalespace_value(&pl->data_y,yline);   
    }else 
      input[i] = p->dim_data[dim].val;
    
    if(dim == p->xdim) xpos = i;
  }

  // drop status lock and compute.  Writes to the data plane are not
  // locked because we still hold the panel concurrent lock; changes
  // to the computational parameters (and heap) can't happen.  Reads
  // from the data array may get inconsistent information, but a
  // completed line flushes which causes a new read to replace the
  // inconsistent one.

  pl->data_outstanding++;
  pthread_mutex_unlock(pl->status_m);
  
  int sofar = 0;
  int step = (1024+outputs-1)/outputs; // at least one
  sv_plane_t *slave;
  while(sofar < w){
    int this_step = (step>w-sofar?w-sofar:step);
    double output[outputs*step];
    double *outptr=output;
    slave = pl->c.share.next;

    // compute
    for(i=0;i<this_step;i++){
      // set x val
      input[xpos] = _sv_scalespace_value(&dx,i+sofar);   
      pl->o->function(input,outptr); // func setup in an objective is immutable
      outptr+=outputs;
    }

    // demultiplex
    data_demultiplex_2d(pl,p,output,yline,sofar,this_step);
    while(slave){
      slave->data_demultiplex_2d(slave,p,output,yline,sofar,this_step);
      slave = slave->c.share_next;
    }
    sofar+= this_step;
  }

  pthread_mutex_lock(pl->status_m);
  if(p->comp_serialno != serialno)return STATUS_WORKING;

  pl->data_outstanding--;
  
  // determine all image lines this y data line affects
  slave = pl;
  while(slave){
    if(ih!=dh || iw!=dw){
      /* resampled row computation; may involve multiple data rows */
      for(i=0;i<ih;i++)
	if(pl->resample_ynumA[i]<=yline && pl->resample_ynumB[i]>yline)
	  pl->image_flags[i]=1;
    }else
      pl->image_flags[yline]=1;  
    slave = slave->c.share_next;
  }

  pl->image_task = 4;

  return STATUS_WORKING;
  
}

// called from GTK/API for map scale changes
static void plane_remap(sv_plane_t *in, sv_panel_t *p){
  sv_plane_2d_t *pl = (sv_plane_2d_t *)in;
  int i,flag=1;

  // check to see if scale vals have changed or just scale settings
  // scalemap is strictly read-only in the worker threads, so there's
  // no need to lock this comparison beyond the GDK lock.
  if(pl->scale.n == pl->scale_widget->labels){
    flag=0;
    for(i=0;i<pl->scale.n;i++)
      if(pl->scale.vals[i] != pl->scale_widget->label_vals[i]){
	flag=1;
	break;
      }
  }
  if(flag){
    // the actual scale changed so updating the cached information
    // requires complete write locking for heap work
    pthread_rwlock_wrlock(pl->panel_m);
 
    _sv_slidermap_clear(&pl->slider);
    _sv_slidermap_init(&pl->slider,&pl->slider_widget);

    wake_workers();
  }else{
    // no scale change
    pthread_mutex_lock(pl->status_m);
    _sv_slidermap_partial_update(&pl->slider,&pl->slider_widget);
  }

  p->map_render=1;
  for(i=0;i<pl->image_y.pixels;i++)
    pl->image_flags[i]=1;
  pl->image_mapnum = gtk_combo_box_get_active(GTK_COMBO_BOX(pl->range_rulldown));
  p->image_serialno++;
  p->image_outstanding=0;

  if(flag){
    pthread_rwlock_unlock(pl->panel_m);
  }else{
    pthread_mutex_unlock(pl->status_m);
  }

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
 
    _sv_slidermap_clear(&pl->slider);
 
    free(pl);
  }
}

GtkWidget *_sv_plane_2d_label_widget(sv_plane2d_t *pl){
  GtkWidget *label = gtk_label_new(o->name);
  gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
  return label;
}


GtkWidget *_sv_plane_2d_obj_widget(sv_plane2d_t *pl){
  GtkWidget *table = gtk_table_new


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

  ret->plane_label = _sv_plane_2d_label_widget;
  ret->plane_obj = _sv_plane_2d_obj_widget;

  return (sv_plane_t *)ret;
}


static void _sv_panel2d_realize(sv_panel_t *p){
  _sv_panel2d_t *p2 = p->subtype->p2;
  int i;



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

