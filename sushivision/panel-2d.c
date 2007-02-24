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
#include <gdk/gdkkeysyms.h>
#include "internal.h"

/* helper functions for performing progressive computation */

// enter unlocked
static void compute_one_data_line_2d(sushiv_panel_t *p, 
				     int serialno,
				     int dw,
				     int y,
				     int x_d, 
				     double x_min, 
				     double x_max, 
				     double *dim_vals, 
				     _sushiv_bythread_cache_2d *c){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i,j;
  
  /* cache access is unlocked because the cache is private to this
     worker thread */

  for(j=0;j<dw;j++){
    double *fout = c->fout;
    sushiv_function_t **f = p2->used_function_list;
    int *obj_y_off = p2->y_fout_offset;
    int *onum = p2->y_obj_to_panel;
    
    /* by function */
    dim_vals[x_d] = (x_max-x_min) * j / dw + x_min;
    for(i=0;i<p2->used_functions;i++){
      (*f)->callback(dim_vals,fout);
      fout += (*f)->outputs;
      f++;
    }
    
    /* process function output by plane type/objective */
    /* 2d panels currently only care about the Y output value */
    
    /* slider map */
    for(i=0;i<p2->y_obj_num;i++){
      float val = (float)slider_val_to_del(p2->range_scales[*onum++], c->fout[*obj_y_off++]);
      if(isnan(val)){
	c->y_map[i][j] = -1;
      }else{
	if(val<0)val=0;
	if(val>1)val=1;
	c->y_map[i][j] = rint(val * (256.f*256.f*256.f));
      }
    }
  }

  gdk_threads_enter ();
  if(p->private->plot_serialno == serialno){
    for(j=0;j<p2->y_obj_num;j++){
      int *d = p2->y_map[j] + y*dw;
      int *td = c->y_map[j];
      
      memcpy(d,td,dw*sizeof(*d));
      
    }
  }
  gdk_threads_leave ();
}

// call with lock
static void clear_pane(sushiv_panel_t *p){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  int pw = p2->x.pixels;
  int ph = p2->y.pixels;
  int i;
  Plot *plot = PLOT(p->private->graph);
  
  for(i=0;i<p2->y_obj_num;i++){
    // map is freed and nulled to avoid triggering a fast-scale on an empty data pane
    if(p2->y_map[i])
      free(p2->y_map[i]);
    p2->y_map[i]=NULL;

    // free y_planes so that initial remap doesn't waste time on mix
    // op; they are recreated during remap at the point the y_todo
    // vector indicates something to do
    if(p2->y_planes[i])
      free(p2->y_planes[i]);
    p2->y_planes[i] = NULL;

    // work vector is merely cleared
    memset(p2->y_planetodo[i], 0, ph*sizeof(**p2->y_planetodo));
  }

  // clear the background surface 
  if(plot->datarect)
    memset(plot->datarect, 0, ph*pw*sizeof(*plot->datarect));

  // the bg is not marked to be refreshed; computation setup will do
  // that as part of the fast_scale, even if the fast_scale is
  // short-circuited to a noop.
}

typedef struct{
  double x;
  double y;
  double z;
  double e1;
  double e2;
  double p1;
  double p2;
  double m;
} compute_result;

// used by the legend code. this lets us get away with having only a mapped display pane
// call with lock
static void compute_single_point(sushiv_panel_t *p,sushiv_objective_t *o, double x, double y, compute_result *out){
  double dim_vals[p->sushi->dimensions];
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i,j;
  int pflag=0;
  int eflag=0;

  // fill in dimensions
  int x_d = p2->x_d->number;
  int y_d = p2->y_d->number;

  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  gdk_threads_leave ();

  dim_vals[x_d] = x;
  dim_vals[y_d] = y;

  *out = (compute_result){NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN};

  // compute
  for(i=0;i<p->sushi->functions;i++){
    sushiv_function_t *f = p->sushi->function_list[i];
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
  gdk_threads_enter ();

}

/* functions that perform actual graphical rendering */

static float resample_helpers_init(scalespace *to, scalespace *from,
				   unsigned char *delA, unsigned char *delB, 
				   int *posA, int *posB,
				   int xymul){
  int i;
  int dw = from->pixels;
  int pw = to->pixels;

  long scalenum = scalespace_scalenum(to,from);
  long scaleden = scalespace_scaleden(to,from);
  long del = scalespace_scaleoff(to,from);
  int bin = del / scaleden;
  del -= bin * scaleden; 
  int discscale = (scaleden>scalenum?scalenum:scaleden);
  int total = xymul*scalenum/discscale;

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

      delA[i] = (xymul * (scaleden - del)) / discscale;
      posA[i] = bin;
      rem -= delA[i];
      rem -= xymul*(sizeceil-2);

      while(bin+sizeceil>dw){
	sizeceil--;
	del2=0;
      }

      del2 %= scaleden;
      delB[i] = rem; // don't leak 
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

/* x resample helpers are put in the per-thread cache because locking it would
   be relatively expensive. */
// call while locked
static void resample_helpers_manage_x(sushiv_panel_t *p, _sushiv_bythread_cache_2d *c){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  if(p->private->plot_serialno != c->serialno){
    int pw = p2->x.pixels;
    c->serialno = p->private->plot_serialno;

    if(c->xdelA)
      free(c->xdelA);
    if(c->xdelB)
      free(c->xdelB);
    if(c->xnumA)
      free(c->xnumA);
    if(c->xnumB)
      free(c->xnumB);
    
    c->xdelA = calloc(pw,sizeof(*c->xdelA));
    c->xdelB = calloc(pw,sizeof(*c->xdelB));
    c->xnumA = calloc(pw,sizeof(*c->xnumA));
    c->xnumB = calloc(pw,sizeof(*c->xnumB));
    c->xscalemul = resample_helpers_init(&p2->x, &p2->x_v, c->xdelA, c->xdelB, c->xnumA, c->xnumB, 17);
  }
}

/* y resample is in the panel struct as per-row access is already locked */
// call while locked
static void resample_helpers_manage_y(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  if(p->private->plot_serialno != p2->resample_serialno){
    int ph = p2->y.pixels;
    p2->resample_serialno = p->private->plot_serialno;

    if(p2->ydelA)
      free(p2->ydelA);
    if(p2->ydelB)
      free(p2->ydelB);
    if(p2->ynumA)
      free(p2->ynumA);
    if(p2->ynumB)
      free(p2->ynumB);
    
    p2->ydelA = calloc(ph,sizeof(*p2->ydelA));
    p2->ydelB = calloc(ph,sizeof(*p2->ydelB));
    p2->ynumA = calloc(ph,sizeof(*p2->ynumA));
    p2->ynumB = calloc(ph,sizeof(*p2->ynumB));
    p2->yscalemul = resample_helpers_init(&p2->y, &p2->y_v, p2->ydelA, p2->ydelB, p2->ynumA, p2->ynumB, 15);
  }
}

static inline void l_mapping_calc( void (*m)(int,int, lcolor*), 
				   int low,
				   float range,
				   int in, 
				   int alpha, 
				   int mul, 
				   lcolor *outc){
  if(mul && in>=alpha){
    int val = rint((in - low) * range);
    if(val<0)val=0;
    if(val>65536)val=65536;
    m(val,mul,outc);
  }
}

/* the data rectangle is data width/height mapped deltas.  we render
   and subsample at the same time. */
/* return: -1 == abort
            0 == more work to be done in this plane
	    1 == plane fully dispatched (possibly not complete) */

/* enter with lock */
static int resample_render_y_plane_line(sushiv_panel_t *p, _sushiv_bythread_cache_2d *c, 
					int plot_serialno, int map_serialno, int y_no){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  int objnum = p2->y_obj_to_panel[y_no]; 
  int *in_data = p2->y_map[y_no];
  int ph = p2->y.pixels;

  if(!in_data || !c){
    p->private->map_complete_count -= ph;
    return 1;
  }

  unsigned char *todo = p2->y_planetodo[y_no];
  int i = p2->y_next_line;
  int j;

  /* find a row that needs to be updated */
  while(i<ph && !todo[i]){
    p->private->map_complete_count--;
    p2->y_next_line++;
    i++;
  }

  if(i == ph) return 1;

  p2->y_next_line++;

  /* row [i] needs to be updated; marshal */
  mapping *map = p2->mappings+objnum;
  void (*mapfunc)(int,int, lcolor *) = map->mapfunc;
  int ol_alpha = rint(p2->alphadel[y_no] * 16777216.f);
  ucolor *panel = p2->y_planes[y_no];
  int ol_low = rint(map->low * 16777216.f);
  float ol_range = map->i_range * (1.f/256.f);

  int pw = p2->x.pixels;
  int dw = p2->x_v.pixels;
  int dh = p2->y_v.pixels;
  ccolor work[pw];

  if(!panel)
    panel = p2->y_planes[y_no] = calloc(pw*ph, sizeof(**p2->y_planes));

  if(ph!=dh || pw!=dw){
    /* resampled row computation; may involve multiple data rows */

    resample_helpers_manage_y(p);
    resample_helpers_manage_x(p,c);

    float idel = p2->yscalemul * c->xscalemul;

    /* by column */
    int ydelA=p2->ydelA[i];
    int ydelB=p2->ydelB[i];
    int ystart=p2->ynumA[i];
    int yend=p2->ynumB[i];
    int lh = yend - ystart;
    int data[lh*dw];

    memcpy(data,in_data+ystart*dw,sizeof(data));

    gdk_threads_leave();

    unsigned char *xdelA = c->xdelA;
    unsigned char *xdelB = c->xdelB;
    int *xnumA = c->xnumA;
    int *xnumB = c->xnumB;
      
    /* by panel col */
    for(j=0;j<pw;j++){
	
      lcolor out = (lcolor){0,0,0,0}; 
      int xstart = xnumA[j];
      int xend = xnumB[j];
      int dx = xstart;
      int xA = xdelA[j];
      int xB = xdelB[j];
      int y = ystart;

      // first line
      if(y<yend){
	if(dx<xend)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx++], ol_alpha, ydelA*xA, &out);
	
	for(; dx < xend-1; dx++)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx], ol_alpha, ydelA*17, &out);
	
	if(dx<xend)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx], ol_alpha, ydelA*xB, &out);
	y++;
      }

      // mid lines
      for(;y<yend-1;y++){
	dx = xstart += dw;
	xend += dw;
	if(dx<xend)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx++], ol_alpha, 15*xA, &out);
	
	for(; dx < xend-1; dx++)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx], ol_alpha, 255, &out);
	
	if(dx<xend)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx], ol_alpha, 15*xB, &out);
      }
      
      // last line
      if(y<yend){
	dx = xstart += dw;
	xend += dw;
	if(dx<xend)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx++], ol_alpha, ydelB*xA, &out);
	
	for(; dx < xend-1; dx++)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx], ol_alpha, ydelB*17, &out);
	
	if(dx<xend)
	  l_mapping_calc(mapfunc, ol_low, ol_range, data[dx], ol_alpha, ydelB*xB, &out);
      }

      work[j].a = (u_int32_t)(out.a*idel);
      work[j].r = (u_int32_t)(out.r*idel);
      work[j].g = (u_int32_t)(out.g*idel);
      work[j].b = (u_int32_t)(out.b*idel);
      
    }

  }else{
    /* non-resampling render */

    int data[dw];
    memcpy(data,in_data+i*dw,sizeof(data));
    gdk_threads_leave();      

    for(j=0;j<pw;j++){

      lcolor out = (lcolor){0,0,0,0};
      l_mapping_calc(mapfunc, ol_low, ol_range, data[j], ol_alpha, 255, &out);
	
      work[j].a = (u_int32_t)(out.a);
      work[j].r = (u_int32_t)(out.r);
      work[j].g = (u_int32_t)(out.g);
      work[j].b = (u_int32_t)(out.b);
    }
  }

  gdk_threads_enter ();  
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno)
    return -1;
  memcpy(panel+i*pw,work,sizeof(work));
  p2->bg_todo[i] = 1;
  p2->y_planetodo[y_no][i] = 0;

  // must be last; it indicates completion
  p->private->map_complete_count--;
  return 0;
}

// enter with lock
static int render_bg_line(sushiv_panel_t *p, int plot_serialno, int map_serialno){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno) return -1;
  
  int ph = p2->y.pixels;
  int pw = p2->x.pixels;
  unsigned char *todo = p2->bg_todo;
  int i = p2->bg_next_line,j;
  ucolor work_bg[pw];
  ucolor work_pl[pw];
  int bgmode = p->private->bg_type;

  /* find a row that needs to be updated */
  while(i<ph && !todo[i]){
    p->private->map_complete_count--;
    p2->bg_next_line++;
    i++;
  }

  if(i == ph)
    goto done;

  if(i < p2->bg_first_line) p2->bg_first_line = i;
  if(i+1 > p2->bg_last_line) p2->bg_last_line = i+1;
  p2->bg_next_line++;

  /* gray background checks */
  gdk_threads_leave();

  switch(bgmode){
  case SUSHIV_BG_WHITE:
    for(j=0;j<pw;j++)
      work_bg[j].u = 0xffffffffU;
    break;
  case SUSHIV_BG_BLACK:
    for(j=0;j<pw;j++)
      work_bg[j].u = 0xff000000U;
    break;
  default:
    render_checks(work_bg,pw,i);
    break;
  }

  /* by objective */
  for(j=0;j<p->objectives;j++){
    int o_ynum = p2->y_obj_from_panel[j];
    
    gdk_threads_enter();
    if(plot_serialno != p->private->plot_serialno ||
       map_serialno != p->private->map_serialno) return -1;

    /**** mix Y plane */
    
    if(p2->y_planes[o_ynum]){
      int x;
      ucolor (*mixfunc)(ucolor,ucolor) = p2->mappings[j].mixfunc;
      ucolor *rect = p2->y_planes[o_ynum] + i*pw;
      memcpy(work_pl,rect,sizeof(work_pl));
      
      gdk_threads_leave();
      for(x=0;x<pw;x++)
	work_bg[x] = mixfunc(work_pl[x],work_bg[x]);
    }else
      gdk_threads_leave();

    /**** mix Z plane */
    
    /**** mix vector plane */

  }

  gdk_threads_enter();
  if(plot_serialno != p->private->plot_serialno ||
     map_serialno != p->private->map_serialno) return -1;

    // rendered a line, get it on the screen */
  
  memcpy(plot->datarect+pw*i, work_bg, sizeof(work_bg));

  p->private->map_complete_count--;

 done:
  if(p->private->map_complete_count)
    return 1; // not done yet

  return 0;
}

static void _dirty_map_full(sushiv_panel_t *p);
// enter with lock; returns zero if thread should sleep / get distracted
static int _sushiv_panel2d_remap(sushiv_panel_t *p, _sushiv_bythread_cache_2d *thread_cache){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);

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
      _dirty_map_full(p);
  }

  /* by plane, by line; each plane renders independently */
  /* Y planes */
  if(p2->y_planetodo){
    if(p2->y_next_plane < p2->y_obj_num){
      int status = resample_render_y_plane_line(p, thread_cache, 
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

      _sushiv_wake_workers();
    }
  }

  /* mix new background, again line by line */
  if(p2->bg_next_line < ph){
    int status = render_bg_line(p, plot_serialno, map_serialno);
    if(status == -1) goto abort;
    if(p->private->map_complete_count)return status;
  }else
    return 0; // nothing left to dispatch

  // entirely finished.

  // remap completed; flush background to screen
  plot_expose_request_partial (plot,0,p2->bg_first_line,
			       pw,p2->bg_last_line - p2->bg_first_line);
  gdk_flush();

  // clean bg todo list
  memset(p2->bg_todo,0,ph*sizeof(*p2->bg_todo));

  // clear 'panel in progress' flag
  p2->partial_remap = 0;
  _sushiv_panel_clean_map(p);
  return 0;

 abort:
  // reset progress to 'start over'
  return 1;
}

// call while locked
static void _dirty_map_one_plane(sushiv_panel_t *p, int onum, int y, int z, int v){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int ph = p2->y.pixels;

  if(y && p2->y_planetodo){
    int y_no = p2->y_obj_from_panel[onum];
    if(y_no>=0 && p2->y_planetodo[y_no])
      memset(p2->y_planetodo[y_no],1,ph * sizeof(**p2->y_planetodo));
  }
  p2->partial_remap = 1;
}

// call while locked 
static void _dirty_map_one_data_line_y(sushiv_panel_t *p, int line){
  // determine all panel lines this y data line affects
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int ph = p2->y.pixels;
  int pw = p2->x.pixels;
  int dw = p2->x_v.pixels;
  int dh = p2->y_v.pixels;
  int i,j;

  p2->partial_remap = 1;

  if(ph!=dh || pw!=dw){
    /* resampled row computation; may involve multiple data rows */
    if(p2->y_planetodo){
      resample_helpers_manage_y(p);
      
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

// call while locked
static void _dirty_map_full(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int ph = p2->y.pixels;
  int i,j;

  p2->partial_remap = 1;

  if(p2->y_planetodo){
    for(j=0;j<p2->y_obj_num;j++){
      if(p2->y_planetodo[j]){
	for(i=0;i<ph;i++){
	  p2->y_planetodo[j][i]=1;
	}
      }
    }
  }
}

// enter with lock
static void update_legend(sushiv_panel_t *p){  
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);

  if(plot){
    int i;
    char buffer[320];
    int depth = 0;
    plot_legend_clear(plot);

    // add each dimension to the legend
    // display decimal precision relative to display scales
    if(3-p2->x.decimal_exponent > depth) depth = 3-p2->x.decimal_exponent;
    if(3-p2->y.decimal_exponent > depth) depth = 3-p2->y.decimal_exponent;
    for(i=0;i<p->dimensions;i++){
      snprintf(buffer,320,"%s = %+.*f",
	       p->dimension_list[i].d->name,
	       depth,
	       p->dimension_list[i].d->val);
      plot_legend_add(plot,buffer);
    }
    
    // one space 
    plot_legend_add(plot,NULL);

    // add each active objective plane to the legend
    // choose the value under the crosshairs 
    for(i=0;i<p->objectives;i++){

      if(!mapping_inactive_p(p2->mappings+i)){
	compute_result vals;
	compute_single_point(p,p->objective_list[i].o, plot->selx, plot->sely, &vals);

	if(!isnan(vals.y)){
	  
	  snprintf(buffer,320,"%s = %f",
		   p->objective_list[i].o->name,
		   vals.y);
	  plot_legend_add(plot,buffer);
	}
      }
    }
  }
}

static void mapchange_callback_2d(GtkWidget *w,gpointer in){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  //sushiv_objective_t *o = optr->o;
  sushiv_panel_t *p = optr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int onum = optr - p->objective_list;

  _sushiv_panel_undo_push(p);
  _sushiv_panel_undo_suspend(p);

  mapping_set_func(&p2->mappings[onum],gtk_combo_box_get_active(GTK_COMBO_BOX(w)));
  
  //redraw the map slider
  slider_draw_background(p2->range_scales[onum]);
  slider_draw(p2->range_scales[onum]);
  slider_expose(p2->range_scales[onum]);
    
  _sushiv_panel_dirty_legend(p);

  //redraw the plot
  _dirty_map_one_plane(p,onum,1,0,0);
  _sushiv_panel_dirty_map(p);
  _sushiv_panel_undo_resume(p);
}

static void map_callback_2d(void *in,int buttonstate){
  sushiv_objective_list_t *optr = (sushiv_objective_list_t *)in;
  //sushiv_objective_t *o = optr->o;
  sushiv_panel_t *p = optr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int onum = optr - p->objective_list;

  if(buttonstate == 0){
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
  }

  // recache alpha del */
  p2->alphadel[onum] = 
    slider_val_to_del(p2->range_scales[onum],
		      slider_get_value(p2->range_scales[onum],1));

  // redraw the plot on motion
  if(buttonstate == 1){
    _dirty_map_one_plane(p,onum,1,0,0);
    _sushiv_panel_dirty_map(p);
  }
  if(buttonstate == 2)
    _sushiv_panel_undo_resume(p);
}

static void update_xy_availability(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;
  // update which x/y buttons are pressable */
  // enable/disable dimension slider thumbs

  for(i=0;i<p->dimensions;i++){
    if(p2->dim_xb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))){
      // make the y insensitive
      if(p2->dim_yb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_yb[i],FALSE);

      // set the x dim flag
      p2->x_d = p->dimension_list[i].d;
      p2->x_scale = p->private->dim_scales[i];
      p2->x_dnum = i;
    }else{
      // if there is a y, make it sensitive 
      if(p2->dim_yb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_yb[i],TRUE);
    }
    if(p2->dim_yb[i] &&
       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i]))){
      // make the x insensitive
      if(p2->dim_xb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_xb[i],FALSE);

      // set the y dim
      p2->y_d = p->dimension_list[i].d;
      p2->y_scale = p->private->dim_scales[i];
      p2->y_dnum = i;
    }else{
      // if there is a x, make it sensitive 
      if(p2->dim_xb[i])
	gtk_widget_set_sensitive_fixup(p2->dim_xb[i],TRUE);
    }
    if((p2->dim_xb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]))) ||
       (p2->dim_yb[i] &&
	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p2->dim_yb[i])))){
      // make all thumbs visible 
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],0,1);
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],2,1);
    }else{
      // make bracket thumbs invisible */
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],0,0);
      _sushiv_dim_widget_set_thumb_active(p->private->dim_scales[i],2,0);
    }
  } 
}

static int v_swizzle(int y, int height){
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

// assumes data is locked
static void fast_scale_x(Spinner *sp,
			 int *data, 
			 int w,
			 int h,
			 scalespace new,
			 scalespace old){
  int x,y;
  int work[w];
  int mapbase[w];
  int mapdel[w];
  double old_w = old.pixels;
  double new_w = new.pixels;

  double old_lo = scalespace_value(&old,0);
  double old_hi = scalespace_value(&old,old_w);
  double new_lo = scalespace_value(&new,0);
  double new_hi = scalespace_value(&new,new_w);
  double newscale = (new_hi-new_lo)/new_w;
  double oldscale = old_w/(old_hi-old_lo);
  for(x=0;x<w;x++){
    double xval = (x)*newscale+new_lo;
    double map = ((xval-old_lo)*oldscale);
    int base = (int)floor(map);
    int del = rint((map - floor(map))*64.f);
    /* hack to overwhelm roundoff error; this is inside a purely
       temporary cosmetic approximation anyway*/
    if(base>0 && del==0){
      mapbase[x]=base-1;
      mapdel[x]=64;
    }else{
      mapbase[x]=base;
      mapdel[x]=del;
    }
  }

  for(y=0;y<h;y++){
    int *data_line = data+y*w;
    spinner_set_busy(sp);
    for(x=0;x<w;x++){
      if(mapbase[x]<0 || mapbase[x]>=(w-1)){
	work[x]=-1;
      }else{
	int base = mapbase[x];
	int A = data_line[base];
	int B = data_line[base+1];
	if(A<0 || B<0)
	  work[x]=-1;
	else
	  work[x]= A + (((B - A)*mapdel[x])>>6);
	
      }
    }
    memcpy(data_line,work,w*(sizeof(*work)));
  }   
}

static void fast_scale_y(Spinner *sp,
			 int *olddata, 
			 int *newdata, 
			 int oldw,
			 int neww,
			 scalespace new,
			 scalespace old){
  int x,y;
  int w = (oldw<neww?oldw:neww);

  int old_h = old.pixels;
  int new_h = new.pixels;

  int mapbase[new_h];
  int mapdel[new_h];

  double old_lo = scalespace_value(&old,0);
  double old_hi = scalespace_value(&old,(double)old_h);
  double new_lo = scalespace_value(&new,0);
  double new_hi = scalespace_value(&new,(double)new_h);
  double newscale = (new_hi-new_lo)/new_h;
  double oldscale = old_h/(old_hi-old_lo);
  
  for(y=0;y<new_h;y++){
    double yval = (y)*newscale+new_lo;
    double map = ((yval-old_lo)*oldscale);
    int base = (int)floor(map);
    int del = rint((map - floor(map))*64.);
    /* hack to overwhelm roundoff error; this is inside a purely
       temporary cosmetic approximation anyway */
    if(base>0 && del==0){
      mapbase[y]=base-1;
      mapdel[y]=64;
    }else{
      mapbase[y]=base;
      mapdel[y]=del;
    }
  }

  
  for(y=0;y<new_h;y++){
    int base = mapbase[y];
    int *new_column = &newdata[y*neww];
    spinner_set_busy(sp);

    if(base<0 || base>=(old_h-1)){
      for(x=0;x<w;x++)
	new_column[x] = -1;
    }else{
      int del = mapdel[y];
      int *old_column = &olddata[base*oldw];

      for(x=0;x<w;x++){
	int A = old_column[x];
	int B = old_column[x+oldw];
	if(A<0 || B<0)
	  new_column[x]=-1;
	else
	  new_column[x]= A + (((B-A)*del)>>6);
      }
    }
  }
}

static void fast_scale(Spinner *sp, 
		       int *newdata, 
		       scalespace xnew,
		       scalespace ynew,
		       int *olddata,
		       scalespace xold,
		       scalespace yold){
  
  int new_w = xnew.pixels;
  int new_h = ynew.pixels;
  int old_w = xold.pixels;
  int old_h = yold.pixels;

  if(new_w > old_w){
    fast_scale_y(sp,olddata,newdata,old_w,new_w,ynew,yold);
    fast_scale_x(sp,newdata,new_w,new_h,xnew,xold);
  }else{
    fast_scale_x(sp,olddata,old_w,old_h,xnew,xold);
    fast_scale_y(sp,olddata,newdata,old_w,new_w,ynew,yold);
  }
}

// call only from main gtk thread
static void _mark_recompute_2d(sushiv_panel_t *p){
  if(!p->private->realized) return;
  Plot *plot = PLOT(p->private->graph);

  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){
    _sushiv_panel1d_mark_recompute_linked(p);   
    _sushiv_panel_dirty_plot(p);
  }
}

static void update_crosshairs(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  double x=0,y=0;
  int i;
  
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    if(d == p2->x_d)
      x = d->val;
    if(d == p2->y_d)
      y = d->val;
    
  }
  
  plot_set_crosshairs(plot,x,y);
  _sushiv_panel1d_update_linked_crosshairs(p,0,0); 
  _sushiv_panel_dirty_legend(p);
}

static void center_callback_2d(sushiv_dimension_list_t *dptr){
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  //Plot *plot = PLOT(p->private->graph);
  int axisp = (d == p2->x_d || d == p2->y_d);

  if(!axisp){
    // mid slider of a non-axis dimension changed, rerender
    _mark_recompute_2d(p);
  }else{
    // mid slider of an axis dimension changed, move crosshairs
    update_crosshairs(p);
    _sushiv_panel1d_update_linked_crosshairs(p,d==p2->x_d,d==p2->y_d); 
  }
}

static void bracket_callback_2d(sushiv_dimension_list_t *dptr){
  sushiv_dimension_t *d = dptr->d;
  sushiv_panel_t *p = dptr->p;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int axisp = (d == p2->x_d || d == p2->y_d);

  if(axisp)
    _mark_recompute_2d(p);
    
}

static void dimchange_callback_2d(GtkWidget *button,gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))){

    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    plot_unset_box(PLOT(p->private->graph));
    update_xy_availability(p);

    clear_pane(p);
    _mark_recompute_2d(p);
    update_crosshairs(p);

    _sushiv_panel_undo_resume(p);
  }
}

static void _sushiv_panel2d_crosshairs_callback(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  double x=PLOT(p->private->graph)->selx;
  double y=PLOT(p->private->graph)->sely;
  int i;
  
  _sushiv_panel_undo_push(p);
  _sushiv_panel_undo_suspend(p);

  //plot_snap_crosshairs(PLOT(p->private->graph));

  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    if(d == p2->x_d){
      _sushiv_dimension_set_value(p->private->dim_scales[i],1,x);
    }

    if(d == p2->y_d){
      _sushiv_dimension_set_value(p->private->dim_scales[i],1,y);
    }
    
    p->private->oldbox_active = 0;
  }

  // dimension setting might have enforced granularity restrictions;
  // have the display reflect that
  x = p2->x_d->val;
  y = p2->y_d->val;

  plot_set_crosshairs(PLOT(p->private->graph),x,y);

  _sushiv_panel_dirty_legend(p);
  _sushiv_panel_undo_resume(p);
}

static void box_callback(void *in, int state){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  
  switch(state){
  case 0: // box set
    _sushiv_panel_undo_push(p);
    plot_box_vals(plot,p2->oldbox);
    p->private->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    _sushiv_panel2d_crosshairs_callback(p);

    _sushiv_dimension_set_value(p2->x_scale,0,p2->oldbox[0]);
    _sushiv_dimension_set_value(p2->x_scale,2,p2->oldbox[1]);
    _sushiv_dimension_set_value(p2->y_scale,0,p2->oldbox[2]);
    _sushiv_dimension_set_value(p2->y_scale,2,p2->oldbox[3]);
    p->private->oldbox_active = 0;
    _sushiv_panel_undo_resume(p);
    break;
  }
  _sushiv_panel_update_menus(p);
}

void _maintain_compute_cache_2d(sushiv_panel_t *p, _sushiv_bythread_cache_2d *c, int w){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  
  /* toplevel initialization */
  if(c->fout == 0){
    int i,count=0;
    
    /* allocate output temporary buffer */
    for(i=0;i<p2->used_functions;i++){
      int fnum = p2->used_function_list[i]->number;
      sushiv_function_t *f = p->sushi->function_list[fnum];
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
static int _sushiv_panel2d_map_redraw(sushiv_panel_t *p, _sushiv_bythread_cache *c){
  return _sushiv_panel2d_remap(p,&c->p2);
}

// subtype entry point for legend redraws; lock held
static int _sushiv_panel2d_legend_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  if(p->private->legend_progress_count)return 0;
  p->private->legend_progress_count++;
  update_legend(p);
  _sushiv_panel_clean_legend(p);
  plot_draw_scales(plot);
  plot_expose_request(plot);
  return 1;
}

// subtype entry point for recomputation; lock held
static int _sushiv_panel2d_compute(sushiv_panel_t *p,
				   _sushiv_bythread_cache *c){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot;
  
  int pw,ph,dw,dh,i,d;
  int serialno;
  double x_min, x_max;
  double y_min, y_max;
  int x_d=-1, y_d=-1;
  scalespace sx,sx_v,sx_i;
  scalespace sy,sy_v,sy_i;

  plot = PLOT(p->private->graph);
  pw = plot->x.pixels;
  ph = plot->y.pixels;
  
  x_d = p2->x_d->number;
  y_d = p2->y_d->number;

  // beginning of computation init
  if(p->private->plot_progress_count==0){
    int remapflag = 0;

    scalespace old_x = p2->x;
    scalespace old_y = p2->y;
    scalespace old_xv = p2->x_v;
    scalespace old_yv = p2->y_v;

    // generate new scales
    _sushiv_dimension_scales(p2->x_d, 
			     p2->x_d->bracket[0],
			     p2->x_d->bracket[1],
			     pw,pw * p->private->oversample_n / p->private->oversample_d,
			     plot->scalespacing,
			     p2->x_d->name,
			     &sx,
			     &sx_v,
			     &sx_i);
    _sushiv_dimension_scales(p2->y_d, 
			     p2->y_d->bracket[1],
			     p2->y_d->bracket[0],
			     ph,ph * p->private->oversample_n / p->private->oversample_d,
			     plot->scalespacing,
			     p2->y_d->name,
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
	  fast_scale(p->private->spinner,newmap, sx_v, sy_v,
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
      _dirty_map_full(p);

      _sushiv_panel_dirty_map(p);

      gdk_threads_leave ();      
      plot_draw_scales(plot); // this should happen outside lock
      gdk_threads_enter ();      
    }
    set_map_throttle_time(p); // swallow the first 'throttled' remap which would only be a single line;

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

  _maintain_compute_cache_2d(p,&c->p2,dw);
  
  d = p->dimensions;

  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[p->sushi->dimensions];
  int y = v_swizzle(p->private->plot_progress_count-1,dh);
  p->private->plot_progress_count++;

  x_min = scalespace_value(&p2->x_i,0);
  x_max = scalespace_value(&p2->x_i,dw);

  y_min = scalespace_value(&p2->y_i,0);
  y_max = scalespace_value(&p2->y_i,dh);

  // Initialize local dimension value array
  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  /* unlock for computation */
  gdk_threads_leave ();
    
  dim_vals[y_d]= (y_max - y_min) / dh * y + y_min;
  compute_one_data_line_2d(p, serialno, dw, y, x_d, x_min, x_max, dim_vals, &c->p2);

  gdk_threads_enter ();

  if(p->private->plot_serialno == serialno){
    p->private->plot_complete_count++;
    _dirty_map_one_data_line_y(p,y);
    if(p->private->plot_complete_count>=dh){ 
      _sushiv_panel_dirty_map(p);
      _sushiv_panel_dirty_legend(p);
      _sushiv_panel_clean_plot(p);
    }else
      _sushiv_panel_dirty_map_throttled(p); 
  }

  return 1;
}

// only called for resize events
static void recompute_callback_2d(void *ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  int i;

  gdk_threads_enter ();
  _mark_recompute_2d(p);
  _sushiv_panel2d_compute(p,NULL); // initial scale setup

  // temporary: blank background to checks
  Plot *plot = PLOT(p->private->graph);
  int pw = plot->x.pixels;
  int ph = plot->y.pixels;
  for(i=0;i<ph;i++)
    render_checks((ucolor *)plot->datarect+pw*i, pw, i);
  
  gdk_threads_leave();
}

static void panel2d_undo_log(sushiv_panel_undo_t *u, sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;

  // alloc fields as necessary
  
  if(!u->mappings)
    u->mappings =  calloc(p->objectives,sizeof(*u->mappings));
  if(!u->scale_vals[0])
    u->scale_vals[0] =  calloc(p->objectives,sizeof(**u->scale_vals));
  if(!u->scale_vals[1])
    u->scale_vals[1] =  calloc(p->objectives,sizeof(**u->scale_vals));
  if(!u->scale_vals[2])
    u->scale_vals[2] =  calloc(p->objectives,sizeof(**u->scale_vals));
  if(!u->dim_vals[0])
    u->dim_vals[0] =  calloc(p->dimensions,sizeof(**u->dim_vals));
  if(!u->dim_vals[1])
    u->dim_vals[1] =  calloc(p->dimensions,sizeof(**u->dim_vals));
  if(!u->dim_vals[2])
    u->dim_vals[2] =  calloc(p->dimensions,sizeof(**u->dim_vals));

  // populate undo
  for(i=0;i<p->objectives;i++){
    u->mappings[i] = p2->mappings[i].mapnum;
    u->scale_vals[0][i] = slider_get_value(p2->range_scales[i],0);
    u->scale_vals[1][i] = slider_get_value(p2->range_scales[i],1);
    u->scale_vals[2][i] = slider_get_value(p2->range_scales[i],2);
  }

  for(i=0;i<p->dimensions;i++){
    u->dim_vals[0][i] = p->dimension_list[i].d->bracket[0];
    u->dim_vals[1][i] = p->dimension_list[i].d->val;
    u->dim_vals[2][i] = p->dimension_list[i].d->bracket[1];
  }
  
  u->x_d = p2->x_dnum;
  u->y_d = p2->y_dnum;
  u->box[0] = p2->oldbox[0];
  u->box[1] = p2->oldbox[1];
  u->box[2] = p2->oldbox[2];
  u->box[3] = p2->oldbox[3];
  u->box_active = p->private->oldbox_active;
}

static void panel2d_undo_restore(sushiv_panel_undo_t *u, sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  int i;
  
  // go in through widgets
  for(i=0;i<p->objectives;i++){
    gtk_combo_box_set_active(GTK_COMBO_BOX(p2->range_pulldowns[i]),u->mappings[i]);
    slider_set_value(p2->range_scales[i],0,u->scale_vals[0][i]);
    slider_set_value(p2->range_scales[i],1,u->scale_vals[1][i]);
    slider_set_value(p2->range_scales[i],2,u->scale_vals[2][i]);
  }

  for(i=0;i<p->dimensions;i++){
    _sushiv_dimension_set_value(p->private->dim_scales[i],0,u->dim_vals[0][i]);
    _sushiv_dimension_set_value(p->private->dim_scales[i],1,u->dim_vals[1][i]);
    _sushiv_dimension_set_value(p->private->dim_scales[i],2,u->dim_vals[2][i]);
  }

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[u->x_d]),TRUE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_yb[u->y_d]),TRUE);

  update_xy_availability(p);

  if(u->box_active){
    plot_box_set(plot,u->box);
    p->private->oldbox_active = 1;
  }else{
    plot_unset_box(plot);
    p->private->oldbox_active = 0;
  }
}

static gboolean panel2d_keypress(GtkWidget *widget,
				 GdkEventKey *event,
				 gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  //  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  
  // check if the widget with focus is an Entry
  GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(widget));
  int entryp = (focused?GTK_IS_ENTRY(focused):0);

  // don't swallow modified keypresses
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK)return FALSE;

  if(entryp){
    // we still filter, but differently
    switch(event->keyval){
    case GDK_BackSpace:
    case GDK_Home:case GDK_KP_Begin:
    case GDK_End:case GDK_KP_End:
    case GDK_Up:case GDK_KP_Up:
    case GDK_Down:case GDK_KP_Down:
    case GDK_Left:case GDK_KP_Left:
    case GDK_Right:case GDK_KP_Right:
    case GDK_minus:case GDK_KP_Subtract:
    case GDK_plus:case GDK_KP_Add:
    case GDK_period:case GDK_KP_Decimal:
    case GDK_0:case GDK_KP_0:
    case GDK_1:case GDK_KP_1:
    case GDK_2:case GDK_KP_2:
    case GDK_3:case GDK_KP_3:
    case GDK_4:case GDK_KP_4:
    case GDK_5:case GDK_KP_5:
    case GDK_6:case GDK_KP_6:
    case GDK_7:case GDK_KP_7:
    case GDK_8:case GDK_KP_8:
    case GDK_9:case GDK_KP_9:
    case GDK_Tab:case GDK_KP_Tab:
    case GDK_ISO_Left_Tab:
    case GDK_Delete:case GDK_KP_Delete:
    case GDK_Return:case GDK_ISO_Enter:
    case GDK_Insert:case GDK_KP_Insert:
    case GDK_e:case GDK_E:
      return FALSE;
    }
    return TRUE;

  }else{
        
    /* non-control keypresses */
    switch(event->keyval){
      
    case GDK_Q:
    case GDK_q:
      // quit
    _sushiv_clean_exit(SIGINT);
    return TRUE;
    
    case GDK_BackSpace:
      // undo 
      _sushiv_panel_undo_down(p);
      return TRUE;
      
    case GDK_r:
    case GDK_space:
      // redo/forward
      _sushiv_panel_undo_up(p);
      return TRUE;
      
    } 
    return FALSE;
  }
}

static void _sushiv_realize_panel2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;

  _sushiv_panel_undo_suspend(p);

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sushiv_clean_exit), (void *)SIGINT);
 
  // add border to sides with hbox/padding
  GtkWidget *borderbox =  gtk_hbox_new(0,0);
  gtk_container_add (GTK_CONTAINER (p->private->toplevel), borderbox);

  // main layout vbox
  p->private->topbox = gtk_vbox_new(0,0);
  gtk_box_pack_start(GTK_BOX(borderbox), p->private->topbox, 0,0,4);
  gtk_container_set_border_width (GTK_CONTAINER (p->private->toplevel), 1);

  /* spinner, top bar */
  {
    GtkWidget *hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(p->private->topbox), hbox, 0,0,0);
    gtk_box_pack_end(GTK_BOX(hbox),GTK_WIDGET(p->private->spinner),0,0,0);
  }

  /* plotbox, graph */
  {
    p->private->graph = GTK_WIDGET(plot_new(recompute_callback_2d,p,
					    (void *)(void *)_sushiv_panel2d_crosshairs_callback,p,
					    box_callback,p,0)); 
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
      sushiv_objective_t *o = p->objective_list[i].o;
      int lo = o->scale->val_list[0];
      int hi = o->scale->val_list[o->scale->vals-1];
      
      /* label */
      GtkWidget *label = gtk_label_new(o->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p2->obj_table),label,0,1,i,i+1,
		       GTK_FILL,0,10,0);
      
      /* mapping pulldown */
      {
	GtkWidget *menu=gtk_combo_box_new_markup();
	int j;
	for(j=0;j<num_mappings();j++)
	  gtk_combo_box_append_text (GTK_COMBO_BOX (menu), mapping_name(j));
	gtk_combo_box_set_active(GTK_COMBO_BOX(menu),0);
	g_signal_connect (G_OBJECT (menu), "changed",
			  G_CALLBACK (mapchange_callback_2d), p->objective_list+i);
	gtk_table_attach(GTK_TABLE(p2->obj_table),menu,4,5,i,i+1,
			 GTK_SHRINK,GTK_SHRINK,0,0);
	p2->range_pulldowns[i] = menu;
      }

    /* the range mapping slices/slider */ 
      sl[0] = slice_new(map_callback_2d,p->objective_list+i);
      sl[1] = slice_new(map_callback_2d,p->objective_list+i);
      sl[2] = slice_new(map_callback_2d,p->objective_list+i);
      
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[0],1,2,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[1],2,3,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(GTK_TABLE(p2->obj_table),sl[2],3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      p2->range_scales[i] = slider_new((Slice **)sl,3,o->scale->label_list,o->scale->val_list,
				       o->scale->vals,SLIDER_FLAG_INDEPENDENT_MIDDLE);

      slice_thumb_set((Slice *)sl[0],lo);
      slice_thumb_set((Slice *)sl[1],lo);
      slice_thumb_set((Slice *)sl[2],hi);
      mapping_setup(&p2->mappings[i],0.,1.,0);
      slider_set_gradient(p2->range_scales[i], &p2->mappings[i]);
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
      sushiv_dimension_t *d = p->dimension_list[i].d;
      
      /* label */
      GtkWidget *label = gtk_label_new(d->name);
      gtk_misc_set_alignment(GTK_MISC(label),1.,.5);
      gtk_table_attach(GTK_TABLE(p2->dim_table),label,0,1,i,i+1,
		       GTK_FILL,0,5,0);
      
      /* x/y radio buttons */
      if(!(d->flags & SUSHIV_DIM_NO_X)){
	if(first_x)
	  p2->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
	else{
	  first_x = p2->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]),TRUE);
	}
	gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_xb[i],1,2,i,i+1,
			 GTK_SHRINK,0,3,0);
      }
      
      if(!(d->flags & SUSHIV_DIM_NO_Y)){
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
	_sushiv_new_dimension_widget(p->dimension_list+i,center_callback_2d,bracket_callback_2d);
      
      gtk_table_attach(GTK_TABLE(p2->dim_table),
		       p->private->dim_scales[i]->t,
		       3,4,i,i+1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      
    }
    for(i=0;i<p->dimensions;i++){
      if(p2->dim_xb[i])
	g_signal_connect (G_OBJECT (p2->dim_xb[i]), "toggled",
			  G_CALLBACK (dimchange_callback_2d), p);
      if(p2->dim_yb[i])
	g_signal_connect (G_OBJECT (p2->dim_yb[i]), "toggled",
			  G_CALLBACK (dimchange_callback_2d), p);
    }
  }

  update_xy_availability(p);

  g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
                    G_CALLBACK (panel2d_keypress), p);
  gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
  gtk_widget_realize(GTK_WIDGET(p->private->spinner));
  gtk_widget_show_all(p->private->toplevel);
  update_xy_availability(p); // yes, this was already done; however,
			     // gtk clobbered the event setup on the
			     // insensitive buttons when it realized
			     // them.  This call will restore them.

  _sushiv_panel_undo_resume(p);
}

int sushiv_new_panel_2d(sushiv_instance_t *s,
			int number,
			const char *name, 
			int *objectives,
			int *dimensions,
			unsigned flags){
  
  int i,j;
  int ret = _sushiv_new_panel(s,number,name,objectives,dimensions,flags);
  sushiv_panel_t *p;
  sushiv_panel2d_t *p2;
  int fout_offsets[s->functions];

  if(ret<0)return ret;
  p = s->panel_list[number];
  p2 = calloc(1, sizeof(*p2));
  p->subtype = 
    calloc(1, sizeof(*p->subtype)); /* the union is alloced not
				       embedded as its internal
				       structure must be hidden */
  
  p->subtype->p2 = p2;
  p->type = SUSHIV_PANEL_2D;
  p->private->bg_type = SUSHIV_BG_CHECKS;

  // verify all the objectives have scales
  for(i=0;i<p->objectives;i++){
    if(!p->objective_list[i].o->scale){
      fprintf(stderr,"All objectives in a 2d panel must have a scale\n");
      return -EINVAL;
    }
  }

  p->private->realize = _sushiv_realize_panel2d;
  p->private->map_action = _sushiv_panel2d_map_redraw;
  p->private->legend_action = _sushiv_panel2d_legend_redraw;
  p->private->compute_action = _sushiv_panel2d_compute;
  p->private->request_compute = _mark_recompute_2d;
  p->private->crosshair_action = _sushiv_panel2d_crosshairs_callback;

  p->private->undo_log = panel2d_undo_log;
  p->private->undo_restore = panel2d_undo_restore;

  /* set up helper data structures for rendering */

  /* determine which functions are actually needed; if it's referenced
     by an objective, it's used.  Precache them in dense form. */
  {
    int fn = p->sushi->functions;
    int used[fn],count=0,offcount=0;
    memset(used,0,sizeof(used));
    memset(fout_offsets,-1,sizeof(fout_offsets));
    
    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      for(j=0;j<o->outputs;j++)
	used[o->function_map[j]]=1;
    }

    for(i=0;i<fn;i++)
      if(used[i]){
	sushiv_function_t *f = p->sushi->function_list[i];
	fout_offsets[i] = offcount;
	offcount += f->outputs;
	count++;
      }

    p2->used_functions = count;
    p2->used_function_list = calloc(count, sizeof(*p2->used_function_list));

    for(count=0,i=0;i<fn;i++)
     if(used[i]){
        p2->used_function_list[count]=p->sushi->function_list[i];
	count++;
      }
  }

  /* set up computation/render helpers for Y planes */

  /* set up Y object mapping index */
  {
    int yobj_count = 0;

    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      if(o->private->y_func) yobj_count++;
    }

    p2->y_obj_num = yobj_count;
    p2->y_obj_list = calloc(yobj_count, sizeof(*p2->y_obj_list));
    p2->y_obj_to_panel = calloc(yobj_count, sizeof(*p2->y_obj_to_panel));
    p2->y_obj_from_panel = calloc(p->objectives, sizeof(*p2->y_obj_from_panel));
    
    yobj_count=0;
    for(i=0;i<p->objectives;i++){
      sushiv_objective_t *o = p->objective_list[i].o;
      if(o->private->y_func){
	p2->y_obj_list[yobj_count] = o;
	p2->y_obj_to_panel[yobj_count] = i;
	p2->y_obj_from_panel[i] = yobj_count;
	yobj_count++;
      }else
	p2->y_obj_from_panel[i] = -1;
      
    }
  }
  
  /* set up function Y output value demultiplex helper */
  {
    p2->y_fout_offset = calloc(p2->y_obj_num, sizeof(*p2->y_fout_offset));
    for(i=0;i<p2->y_obj_num;i++){
      sushiv_objective_t *o = p2->y_obj_list[i];
      int funcnum = o->private->y_func->number;
      p2->y_fout_offset[i] = fout_offsets[funcnum] + o->private->y_fout;
    }
  }

  p2->y_map = calloc(p2->y_obj_num,sizeof(*p2->y_map));
  p2->y_planetodo = calloc(p2->y_obj_num,sizeof(*p2->y_planetodo));
  p2->y_planes = calloc(p2->y_obj_num,sizeof(*p2->y_planes));

  return 0;
}

