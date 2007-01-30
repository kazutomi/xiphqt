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

/* performs render helper cleanup after finishing a progressive
   render; makes sure old map is completely replaced by new. */
/* Don't check serialno or size; we must force any completion before a
   resize/rerender, so it will always match anyway */
/* call from lock */
static void compute_complete_render(sushiv_panel_t *p, 
				    int true_complete){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  scalespace *panelx = &p2->x;
  scalespace *panely = &p2->y;
  scalespace *datay = &p2->y_v;

  int i,x,y;
  int w = panelx->pixels;
  int h = panely->pixels;

  /* these progressive rendering helpers are specific to resampled y;
     if ph=dh, there's nothing to do */

  if(h != datay->pixels){
    if(p2->render_flag){
      if(true_complete){
	/* fully complete render; swap and zero */

	for(i=0;i<p2->y_obj_num;i++){
	  float *n = p2->y_num_rend[i];
	  float *d = p2->y_den_rend[i];

	  p2->y_num_rend[i] = p2->y_num[i];
	  p2->y_num[i] = n;
	  p2->y_den_rend[i] = p2->y_den[i];
	  p2->y_den[i] = d;
	  
	  memset(p2->y_num_rend[i],0,sizeof(**p2->y_num_rend)*w*h);
	  memset(p2->y_den_rend[i],0,sizeof(**p2->y_den_rend)*w*h);

	}

      }else{
	/* partially complete render; force the completion as a mix of current/pending */

	for(i=0;i<p2->y_obj_num;i++){
	  for(y=0;y<h;y++){
	    float del = p2->y_rend[y];
	    if(del>0){
	      float *na = p2->y_num[i] + y*w;
	      float *nb = p2->y_num_rend[i] + y*w;
	      float *da = p2->y_den[i] + y*w;
	      float *db = p2->y_den_rend[i] + y*w;

	      for(x=0;x<w;x++){
		*na = (*na) + (*na - *nb++)*del;
		na++;
	      }

	      for(x=0;x<w;x++){
		*da = (*da) + (*da - *db++)*del;
		da++;
	      }
	    }
	  }

	  memset(p2->y_num_rend[i],0,sizeof(**p2->y_num_rend)*w*h);
	  memset(p2->y_den_rend[i],0,sizeof(**p2->y_den_rend)*w*h);

	}      
      
      }

      memset(p2->y_rend,0,sizeof(*p2->y_rend)*h);
    }

    p2->render_flag = 0;
  }
}

/* prepares for a render (checks for / sets up resampling) */
// call from lock
static void compute_prepare_render(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  scalespace *panelx = &p2->x;
  scalespace *panely = &p2->y;
  scalespace *datay = &p2->y_v;

  int i;
  int w = panelx->pixels;
  int h = panely->pixels;

  if(p2->render_flag)
    compute_complete_render(p, 0);

  /* progressive rendering helpers are specific to resampled y;
     if ph=dh, there's nothing to do */
  if(h != datay->pixels){
    if(!p2->y_rend){

      p2->y_rend = calloc(h,sizeof(*p2->y_rend));
    
      if(!p2->y_num_rend)
	p2->y_num_rend = calloc(p2->y_obj_num,sizeof(*p2->y_num_rend));
      if(!p2->y_den_rend)
	p2->y_den_rend = calloc(p2->y_obj_num,sizeof(*p2->y_den_rend));
      
      for(i=0;i<p2->y_obj_num;i++){
	p2->y_num_rend[i] = calloc(w*h,sizeof(**p2->y_num_rend));
	p2->y_den_rend[i] = calloc(w*h,sizeof(**p2->y_den_rend));
      }
    }
    p2->render_flag = 1;
  }
}

static void compute_free_render(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;

  if(p2->y_rend){
    free(p2->y_rend);
    p2->y_rend = NULL;
  }
    
  if(p2->y_num_rend){
    for(i=0;i<p2->y_obj_num;i++){
      free(p2->y_num_rend[i]);
      p2->y_num_rend[i]=NULL;
    }
  }

  if(p2->y_den_rend){
    for(i=0;i<p2->y_obj_num;i++){
      free(p2->y_den_rend[i]);
      p2->y_den_rend[i]=NULL;
    }
  }
}

// enter unlocked
static void compute_one_data_line_2d(sushiv_panel_t *p, 
				     scalespace panelx,
				     scalespace datax,
				     int x_d, 
				     double x_min, 
				     double x_max, 
				     double *dim_vals, 
				     _sushiv_compute_cache_2d *c){
  int pw = panelx.pixels;
  int dw = datax.pixels;

  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i,j;

  /* cache access is unlocked because the cache is private to this
     worker thread */

  if(pw != dw){
    /* resampled computation */
    float scaledel = scalespace_scaledel(&datax,&panelx);
    float outdel = scalespace_pixel(&panelx,scalespace_value(&datax,-.5))+.5;
    int outbin = floor(outdel);
    outdel -= outbin; 
    
    /* zero obj line cache */
    for(i=0;i<p->objectives;i++){
      if(c->y_num[i])
	memset(c->y_num[i],0, c->storage_width * sizeof(**c->y_num));
      if(c->y_den[i])
	memset(c->y_den[i],0, c->storage_width * sizeof(**c->y_den));
    }

    /* by x */
    for(j=0;j<dw;j++){
      float outdel2 = outdel + scaledel;

      double *fout = c->fout;
      sushiv_function_t **f = p2->used_function_list;
      int *obj_y_off = p2->y_fout_offset;

      float obj_y[p2->y_obj_num];
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
	obj_y[i] = (float)slider_val_to_del(p2->range_scales[*onum], c->fout[*obj_y_off]);
	obj_y_off++;
	onum++;
      }
      
      /* resample */
      while(outdel2>1.f){
	float addel = (1.f - outdel);
	
	if(outbin >= 0 && outbin < pw){
	  for(i=0;i<p2->y_obj_num;i++){
	    if(!isnan(obj_y[i])){
	       c->y_num[i][outbin] += obj_y[i] * addel;
	       c->y_den[i][outbin] += addel;
	    }
	  }
	}
	
	outdel2--;;
	outbin++;
	outdel = 0.f;
      }
      
      if(outdel2>0.f){
	float addel = (outdel2 - outdel);
	
	if(outbin >= 0 && outbin < pw){
	  for(i=0;i<p2->y_obj_num;i++){
	    if(!isnan(obj_y[i])){
	       c->y_num[i][outbin] += obj_y[i] * addel;
	       c->y_den[i][outbin] += addel;
	    }
	  }
	}
	outdel += addel;
      }
    }

  }else{
    /* simpler non-resampling case */
    /* by x */
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
	float yval = (float)slider_val_to_del(p2->range_scales[*onum], c->fout[*obj_y_off]);
	
	if(!isnan(yval)){
	  c->y_num[i][j] = yval;
	  c->y_den[i][j] = 1.f;
	}else{
	  c->y_num[i][j] = 0.f;
	  c->y_den[i][j] = 0.f;
	}

	obj_y_off++;
	onum++;
      }
    }
  }
}

/* Although render/swizzle is done by data line, we still need to
   display panel lines.  This is a wrapper around data line rendering
   that updates the relevant panel line[s] with the computed
   data. */
static void compute_one_line_2d(sushiv_panel_t *p, 
				int serialno,
				scalespace panelx,
				scalespace datax,
				scalespace panely,
				scalespace datay,
				      
				int y, // data line
				int x_d, 
				double x_min, 
				double x_max, 
				double *dim_vals, 
				_sushiv_compute_cache_2d *c){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i,j;
  int w = panelx.pixels;
  int ph = panely.pixels;
  int dh = datay.pixels;
  
  /* before anything else-- compute the line. */
  compute_one_data_line_2d(p, panelx, datax, x_d, x_min, x_max,
			   dim_vals, c);    

  if(ph != dh){
    /* this is a resampling population */

    float scaledel = scalespace_scaledel(&datay,&panely);
    float outdel = scalespace_pixel(&panely,scalespace_value(&datay,y-.5))+.5;
    int outbin = floor(outdel);
    float outdel2 = (outdel-outbin) + scaledel;
    outdel -= outbin; 

    while(outdel2>1.f){
      float addel = (1.f - outdel);

      if(outbin >= 0 && outbin < ph){
	gdk_threads_enter ();

	if(p2->serialno == serialno){
	  for(j=0;j<p2->y_obj_num;j++){
	    float *n = p2->y_num_rend[j] + outbin*w;
	    float *d = p2->y_den_rend[j] + outbin*w;
	    float *tn = c->y_num[j];
	    float *td = c->y_den[j];

	    for(i=0;i<w;i++){
	      n[i] += tn[i] * addel;
	      d[i] += td[i] * addel;
	    }

	  }
	  p2->y_rend[outbin]+=addel;
	  gdk_threads_leave ();
	}else{
	  gdk_threads_leave ();
	  return;
	}
      }

      outdel2--;
      outbin++;
      outdel = 0.f;
    }

    if(outdel2>0.f){
      float addel = (outdel2 - outdel);
      
      if(outbin >= 0 && outbin < ph){
	gdk_threads_enter ();
	if(p2->serialno == serialno){
	  for(j=0;j<p2->y_obj_num;j++){
	    float *n = p2->y_num_rend[j] + outbin*w;
	    float *d = p2->y_den_rend[j] + outbin*w;
	    float *tn = c->y_num[j];
	    float *td = c->y_den[j];
	    
	    for(i=0;i<w;i++){
	      n[i] += tn[i] * addel;
	      d[i] += td[i] * addel;
	    }

	  }
	  p2->y_rend[outbin]+=addel;
	}
	gdk_threads_leave ();
      }
    }
  }else{

    gdk_threads_enter ();
    
    if(p2->serialno == serialno){
      for(j=0;j<p2->y_obj_num;j++){
	float *n = p2->y_num[j] + y*w;
	float *d = p2->y_den[j] + y*w;
	float *tn = c->y_num[j];
	float *td = c->y_den[j];
	
	memcpy(n,tn,w*sizeof(*n));
	memcpy(d,td,w*sizeof(*n));

      }
    }
    gdk_threads_leave ();
  }
}

// call with lock
static void clear_pane(sushiv_panel_t *p){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  scalespace *panelx = &p2->x;
  scalespace *panely = &p2->y;
  int i;
  int w = panelx->pixels;
  int h = panely->pixels;

  for(i=0;i<p2->y_obj_num;i++){
    memset(p2->y_num[i],0,sizeof(**p2->y_num)*w*h);
    memset(p2->y_den[i],0,sizeof(**p2->y_den)*w*h);
    if(p2->y_num_rend && p2->y_num_rend[i])
      memset(p2->y_num_rend[i],0,sizeof(**p2->y_num_rend)*w*h);
    if(p2->y_den_rend && p2->y_den_rend[i])
      memset(p2->y_den_rend[i],0,sizeof(**p2->y_den_rend)*w*h);
  } 
  if(p2->y_rend)
    memset(p2->y_rend,0,sizeof(*p2->y_rend)*h);
  p2->render_flag = 0;
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

  gdk_threads_leave();

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
  gdk_threads_enter();

}

/* functions that perform actual graphical rendering */

static void render_checks(int w, int y, u_int32_t *render){
  int x,j;
  /* default checked background */
  /* 16x16 'mid-checks' */ 
  int phase = (y>>4)&1;
  for(x=0;x<w;){
    u_int32_t phaseval = 0x505050;
    if(phase) phaseval = 0x808080;
    for(j=0;j<16 && x<w;j++,x++)
      render[x] = phaseval;
    phase=!phase;
  }
}

static void render_y_plane(sushiv_panel_t *p, int y, int objnum, u_int32_t *render){
  sushiv_panel2d_t *p2 = p->subtype->p2;

  int w,h,x;
  int cond_onum = p2->y_obj_from_panel[objnum];
  double alpha = p2->alphadel[objnum];
  w = p2->x.pixels;
  h = p2->y.pixels;

  // is this a resampled render in-progress?
  if(p2->render_flag){
    // resampled render in-progress; we must merge the panel and render buffers
    float del = p2->y_rend[y];
    float *numA = p2->y_num[cond_onum] + w*y;
    float *denA = p2->y_den[cond_onum] + w*y;
    float *numB = p2->y_num_rend[cond_onum] + w*y;
    float *denB = p2->y_den_rend[cond_onum] + w*y;

    for(x=0;x<w;x++){
      float num = numA[x] + (numB[x] - numA[x])*del;
      float den = denA[x] + (denB[x] - denA[x])*del;
      if(den>0.f && !isnan(num)){
	num /= den;
	/* map/render result */
	if(num>=alpha)
	  render[x] = mapping_calc_a(p2->mappings+objnum,num,den,render[x]);
      }
    }
    
  }else{
    // normal render or fully complete resampled render 
    
    float *num = p2->y_num[cond_onum] + w*y;
    float *den = p2->y_den[cond_onum] + w*y;

    for(x=0;x<w;x++){
      if(den[x]>0.f && !isnan(num[x])){
	float val = num[x] / den[x];
	/* map/render result */
	if(val>=alpha)
	  render[x] = mapping_calc_a(p2->mappings+objnum,val,den[x],render[x]);
      }
    }
  }
}

static void _sushiv_panel2d_remap(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);

  int w,h,y,i;
  w = p2->x.pixels;
  h = p2->y.pixels;

  if(plot){
    for(y = 0; y<h; y++){
      u_int32_t *render = plot->datarect + y*w;
      
      /* background checks */
      render_checks(w,y,render);
      
      /* by objective */
      for(i=0;i<p->objectives;i++){
	
	/**** render Y plane */
	render_y_plane(p, y, i, render);
	
	/**** render Z plane */

	/**** render vector plane */

      }
    }
  }
}

static void update_legend(sushiv_panel_t *p){  
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter ();

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
    gdk_threads_leave ();
  }
}

static void _sushiv_panel2d_map_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter (); // misuse me as a global mutex
  
  _sushiv_panel2d_remap(p);
  if(plot)
    plot_expose_request(plot);
 
  gdk_threads_leave (); // misuse me as a global mutex
}

static void _sushiv_panel2d_legend_redraw(sushiv_panel_t *p){
  Plot *plot = PLOT(p->private->graph);

  gdk_threads_enter (); // misuse me as a global mutex
    update_legend(p);
  if(plot)
    plot_draw_scales(plot);
  gdk_threads_leave (); // misuse me as a global mutex
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

  //redraw the plot
  _sushiv_panel_dirty_map(p);
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
static void fast_scale_x(float *data, 
			 int w,
			 int h,
			 scalespace new,
			 scalespace old){
  int x,y;
  float work[w];
  int mapbase[w];
  float mapdel[w];

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
    mapbase[x]=(int)floor(map);
    mapdel[x]=map-floor(map);
  }

  for(y=0;y<h;y++){
    float *data_line = data+y*w;
    for(x=0;x<w;x++){
      if(mapbase[x]<0 || mapbase[x]>=(w-1)){
	work[x]=0.f;
      }else{
	int base = mapbase[x];
	float del = mapdel[x];
	float A = data_line[base];
	float B = data_line[base+1];
	  work[x]= A + (B - A)*del;
	
      }
    }
    memcpy(data_line,work,w*(sizeof(*work)));
  }   
}

static void fast_scale_y(float *data, 
			 int w,
			 int h,
			 scalespace new,
			 scalespace old){
  int x,y;
  float work[h];
  int mapbase[h];
  float mapdel[h];

  double old_h = old.pixels;
  double new_h = new.pixels;
  double old_lo = scalespace_value(&old,0);
  double old_hi = scalespace_value(&old,old_h);
  double new_lo = scalespace_value(&new,0);
  double new_hi = scalespace_value(&new,new_h);
  double newscale = (new_hi-new_lo)/new_h;
  double oldscale = old_h/(old_hi-old_lo);
  
  for(y=0;y<h;y++){
    double yval = (y)*newscale+new_lo;
    double map = ((yval-old_lo)*oldscale);
    mapbase[y]=(int)floor(map);
    mapdel[y]=map-floor(map);
  }
  
  for(x=0;x<w;x++){
    float *data_column = data+x;
    int stride = w;
    for(y=0;y<h;y++){
      if(mapbase[y]<0 || mapbase[y]>=(h-1)){
	work[y]=0.f;
      }else{
	int base = mapbase[y]*stride;
	float del = mapdel[y];
	float A = data_column[base];
	float B = data_column[base+stride];
	
	work[y]= A + (B-A)*del;
	
      }
    }
    for(y=0;y<h;y++){
      *data_column = work[y];
      data_column+=stride;
    }
  }   
}

static void fast_scale(float *newdata, 
		       scalespace xnew,
		       scalespace ynew,
		       float *olddata,
		       scalespace xold,
		       scalespace yold){
  int y;
  
  int new_w = xnew.pixels;
  int new_h = ynew.pixels;
  int old_w = xold.pixels;
  int old_h = yold.pixels;

  if(new_w > old_w){
    if(new_h > old_h){
      // copy image to new, scale there
      for(y=0;y<old_h;y++){
	float *new_line = newdata+y*new_w;
	float *old_line = olddata+y*old_w;
	memcpy(new_line,old_line,old_w*(sizeof*new_line));
      }
      fast_scale_x(newdata,new_w,new_h,xnew,xold);
      fast_scale_y(newdata,new_w,new_h,ynew,yold);
    }else{
      // scale y in old pane, copy to new, scale x 
      fast_scale_y(olddata,old_w,old_h,ynew,yold);
      for(y=0;y<new_h;y++){
	float *new_line = newdata+y*new_w;
	float *old_line = olddata+y*old_w;
	memcpy(new_line,old_line,old_w*(sizeof*new_line));
      }
      fast_scale_x(newdata,new_w,new_h,xnew,xold);
    }
  }else{
    if(new_h > old_h){
      // scale x in old pane, o=copy to new, scale y
      fast_scale_x(olddata,old_w,old_h,xnew,xold);
      for(y=0;y<old_h;y++){
	float *new_line = newdata+y*new_w;
	float *old_line = olddata+y*old_w;
	memcpy(new_line,old_line,new_w*(sizeof*new_line));
      }
      fast_scale_y(newdata,new_w,new_h,ynew,yold);
    }else{
      // scale in old pane, copy to new 
      // also the case where newdata == olddata and the size is unchanged
      fast_scale_x(olddata,old_w,old_h,xnew,xold);
      fast_scale_y(olddata,old_w,old_h,ynew,yold);
      if(olddata != newdata){
	for(y=0;y<new_h;y++){
	  float *new_line = newdata+y*new_w;
	  float *old_line = olddata+y*old_w;
	  memcpy(new_line,old_line,new_w*(sizeof*new_line));
	}
      }
    }
  }
}

// call only from main gtk thread!
static void _mark_recompute_2d(sushiv_panel_t *p){
  if(!p->private->realized) return;
  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot = PLOT(p->private->graph);
  int w = plot->w.allocation.width;
  int h = plot->w.allocation.height;
  int remapflag = 0;

  if(plot && GTK_WIDGET_REALIZED(GTK_WIDGET(plot))){
    
    if( p2->serialno && // we've been through once and alloced
	(p2->x.pixels != w ||
	 p2->y.pixels != h)){
		
      // if a render was in progress, force completion 
      compute_complete_render(p, 0);
      compute_free_render(p);

      // make new rects, do a fast/dirty scaling job from old to new
      int i;

      /* Y planes */
      for(i=0;i<p2->y_obj_num;i++){
	float *new_n = calloc(w*h,sizeof(*new_n));
	float *new_d = calloc(w*h,sizeof(*new_d));
	
	fast_scale(new_n,plot->x,plot->y,p2->y_num[i],p2->x,p2->y);
	fast_scale(new_d,plot->x,plot->y,p2->y_den[i],p2->x,p2->y);
	
	free(p2->y_num[i]);
	free(p2->y_den[i]);
	p2->y_num[i] = new_n;
	p2->y_den[i] = new_d;
      }

      /* match data scales to new panel size/scale */
      p2->x = plot->x;
      _sushiv_dimension_scales_from_panel(p2->x_d,
					  p2->x,
					  p2->x.pixels, // over/undersample will go here
					  &p2->x_v,
					  &p2->x_i);
      p2->y = plot->y;
      _sushiv_dimension_scales_from_panel(p2->y_d,
					  p2->y,
					  p2->y.pixels, // over/undersample will go here
					  &p2->y_v,
					  &p2->y_i);
      _sushiv_panel2d_map_redraw(p);
    }else{

      _sushiv_dimension_scales(p2->x_d, 
			       p2->x_d->bracket[0],
			       p2->x_d->bracket[1],
			       w,w,// over/undersample will go here
			       plot->scalespacing,
			       p2->x_d->name,
			       &p2->x,
			       &p2->x_v,
			       &p2->x_i);
      _sushiv_dimension_scales(p2->y_d, 
			       p2->y_d->bracket[1],
			       p2->y_d->bracket[0],
			       h,h,// over/undersample will go here
			       plot->scalespacing,
			       p2->y_d->name,
			       &p2->y,
			       &p2->y_v,
			       &p2->y_i);
    }
    
    if(!p2->y_num){
      int i;
      // allocate it
      
      p2->y_num = calloc(p2->y_obj_num,sizeof(*p2->y_num));
      for(i=0;i<p2->y_obj_num;i++)
	p2->y_num[i] = calloc(w*h, sizeof(**p2->y_num));

      remapflag = 1;
    }

    if(!p2->y_den){
      int i;
      // allocate it
      
      p2->y_den = calloc(p2->y_obj_num,sizeof(*p2->y_den));
      for(i=0;i<p2->y_obj_num;i++)
	p2->y_den[i] = calloc(w*h, sizeof(**p2->y_den));

      remapflag = 1;
    }

    if(remapflag)
      _sushiv_panel2d_map_redraw(p);
    
    p2->serialno++;
    p2->last_line = 0;
    p2->completed_lines = 0;
    
    _sushiv_panel1d_mark_recompute_linked(p);   

    _sushiv_wake_workers();
  }
}

static void recompute_callback_2d(void *ptr){
  sushiv_panel_t *p = (sushiv_panel_t *)ptr;
  _mark_recompute_2d(p);
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
    _sushiv_panel2d_map_redraw(p);
    
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

  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = p->dimension_list[i].d;
    if(d == p2->x_d){
      if(p2->x_d->val != x)
	_sushiv_dimension_set_value(p->private->dim_scales[i],1,x);
    }

    if(d == p2->y_d){
      if(p2->y_d->val != y)
	_sushiv_dimension_set_value(p->private->dim_scales[i],1,y);
    }
    
    p2->oldbox_active = 0;
  }

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
    p2->oldbox_active = plot->box_active;
    break;
  case 1: // box activate
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);

    _sushiv_panel2d_crosshairs_callback(p);

    _sushiv_dimension_set_value(p2->x_scale,0,p2->oldbox[0]);
    _sushiv_dimension_set_value(p2->x_scale,2,p2->oldbox[1]);
    _sushiv_dimension_set_value(p2->y_scale,0,p2->oldbox[2]);
    _sushiv_dimension_set_value(p2->y_scale,2,p2->oldbox[3]);
    p2->oldbox_active = 0;
    _sushiv_panel_undo_resume(p);
    break;
  }
  p->private->update_menus(p);
}

void _maintain_cache_2d(sushiv_panel_t *p, _sushiv_compute_cache_2d *c, int w){
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
    c->y_num = calloc(p2->y_obj_num,sizeof(*c->y_num));
    c->y_den = calloc(p2->y_obj_num,sizeof(*c->y_den));
    for(i=0;i<p2->y_obj_num;i++){
      c->y_num[i] = calloc(w,sizeof(**c->y_num));
      c->y_den[i] = calloc(w,sizeof(**c->y_den));
    }
    c->storage_width = w;
  }

  /* anytime the data width changes */
  if(c->storage_width != w){
    int i;
    c->storage_width = w;

    for(i=0;i<p2->y_obj_num;i++){
      c->y_num[i] = realloc(c->y_num[i],w*sizeof(**c->y_num));
      c->y_den[i] = realloc(c->y_den[i],w*sizeof(**c->y_den));
    }
  }
}


// called from one/all of the worker threads; the idea is that several
// of the threads will all call this and they collectively interleave
// ongoing computation of the pane
static int _sushiv_panel_cooperative_compute_2d(sushiv_panel_t *p,
						_sushiv_compute_cache *c){

  sushiv_panel2d_t *p2 = p->subtype->p2;
  Plot *plot;
  
  int pw,ph,dw,dh,i,d;
  int serialno;
  double x_min, x_max;
  double y_min, y_max;
  int x_d=-1, y_d=-1;
  int render_scale_flag = 0;
  scalespace sx,sx_v;
  scalespace sy,sy_v;

  // lock during setup
  gdk_threads_enter ();
  sx = p2->x;
  sx_v = p2->x_i;
  sy = p2->y;
  sy_v = p2->y_i;

  pw = sx.pixels;
  ph = sy.pixels;
  dw = sx_v.pixels;
  dh = sy_v.pixels;

  if(p2->last_line>=dh){
    gdk_threads_leave ();
    return 0;
  }

  plot = PLOT(p->private->graph);
  serialno = p2->serialno;
  d = p->dimensions;

  /* render using local dimension array; several threads will be
     computing objectives */
  double dim_vals[p->sushi->dimensions];

  x_min = scalespace_value(&p2->x_i,0);
  x_max = scalespace_value(&p2->x_i,dw);
  x_d = p2->x_d->number;

  y_min = scalespace_value(&p2->y_i,0);
  y_max = scalespace_value(&p2->y_i,dh);
  y_d = p2->y_d->number;

  // preparation and init before first line render attempt 
  _maintain_cache_2d(p,&c->p2,pw);
  if(p2->last_line==0){
    render_scale_flag = 1;
    compute_prepare_render(p);

    // if the scale bound has changed, fast scale our background data to fill
    // the pane while new, more precise data renders.
    if(memcmp(&sx,&plot->x,sizeof(sx))){
      for(i=0;i<p2->y_obj_num;i++){
	fast_scale_x(p2->y_num[i],pw,ph,
		     sx,plot->x);
	fast_scale_x(p2->y_den[i],pw,ph,
		     sx,plot->x);
      }
      plot->x = sx;
      _sushiv_panel2d_remap(p);
    }
    if(memcmp(&sy,&plot->y,sizeof(sy))){
      for(i=0;i<p2->y_obj_num;i++){
	fast_scale_y(p2->y_num[i],pw,ph,
		     sy,plot->y);
	fast_scale_y(p2->y_den[i],pw,ph,
		     sy,plot->y);
      }
      plot->y = sy;
      _sushiv_panel2d_remap(p);
    }
  }

  // Initialize local dimension value array
  for(i=0;i<p->sushi->dimensions;i++){
    sushiv_dimension_t *dim = p->sushi->dimension_list[i];
    dim_vals[i]=dim->val;
  }

  /* iterate */
  /* by line */
  if(p2->last_line<dh &&
     serialno == p2->serialno){
    int y = v_swizzle(p2->last_line,dh);

    p2->last_line++;
    
    /* unlock for computation */
    gdk_threads_leave ();
    
    if(render_scale_flag){
      plot_draw_scales(plot);
      render_scale_flag = 0;
    }
    
    dim_vals[y_d]= (y_max - y_min) / dh * y + y_min;
    
    /* compute line */
    compute_one_line_2d(p, serialno, sx, sx_v, sy, sy_v,  y, x_d, x_min, x_max, dim_vals, &c->p2);

    gdk_threads_enter ();

    if(p2->serialno == serialno){
      p2->completed_lines++;
      if(p2->completed_lines==dh){ 
	compute_complete_render(p, 1);
	_sushiv_panel_dirty_map(p);
	_sushiv_panel_dirty_legend(p);
	plot_expose_request(plot);
      }else{
	_sushiv_panel_dirty_map_throttled(p);
      }
    }
  }
  
  gdk_threads_leave ();
  return 1;
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
  u->box_active = p2->oldbox_active;
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
    p2->oldbox_active = 1;
  }else{
    plot_unset_box(plot);
    p2->oldbox_active = 0;
  }
}

static gboolean panel2d_keypress(GtkWidget *widget,
				 GdkEventKey *event,
				 gpointer in){
  sushiv_panel_t *p = (sushiv_panel_t *)in;
  //  sushiv_panel2d_t *p2 = (sushiv_panel2d_t *)p->internal;
  
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK)return FALSE;
  
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

static void update_context_menus(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;

  // is undo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_level){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),0),FALSE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),0),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),0),TRUE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),0),TRUE);
  }

  // is redo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level] ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level+1]){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),1),FALSE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),1),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->popmenu),1),TRUE);
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p2->graphmenu),1),TRUE);
  }

  // are we starting or enacting a zoom box?
  if(p2->oldbox_active){ 
    gtk_menu_alter_item_label(GTK_MENU(p2->popmenu),3,"Zoom to box");
  }else{
    gtk_menu_alter_item_label(GTK_MENU(p2->popmenu),3,"Start zoom box");
  }

}

static void wrap_exit(sushiv_panel_t *dummy){
  _sushiv_clean_exit(SIGINT);
}

static char *panel_menulist[]={
  "Undo",
  "Redo",
  "",
  "Quit",
  NULL
};

static char *panel_shortlist[]={
  "Backspace",
  "Space",
  NULL,
  "q",
  NULL
};

static void (*panel_calllist[])(sushiv_panel_t *)={
  &_sushiv_panel_undo_down,
  &_sushiv_panel_undo_up,
  NULL,
  &wrap_exit,
  NULL,
};

static void wrap_enter(sushiv_panel_t *p){
  plot_do_enter(PLOT(p->private->graph));
}

static void wrap_escape(sushiv_panel_t *p){
  plot_do_escape(PLOT(p->private->graph));
}

static char *graph_menulist[]={
  "Undo",
  "Redo",
  "",
  "Start zoom box",
  "Clear readouts",
  "",
  "Quit",
  NULL
};

static char *graph_shortlist[]={
  "Backspace",
  "Space",
  NULL,
  "Enter",
  "Escape",
  NULL,
  "q",
  NULL
};

static void (*graph_calllist[])(sushiv_panel_t *)={
  &_sushiv_panel_undo_down,
  &_sushiv_panel_undo_up,
  NULL,

  &wrap_enter,
  &wrap_escape,
  NULL,
  &wrap_exit,
  NULL,
};


static void _sushiv_realize_panel2d(sushiv_panel_t *p){
  sushiv_panel2d_t *p2 = p->subtype->p2;
  int i;

  _sushiv_panel_undo_suspend(p);

  p->private->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect_swapped (G_OBJECT (p->private->toplevel), "delete-event",
			    G_CALLBACK (_sushiv_clean_exit), (void *)SIGINT);
 
  p2->top_table = gtk_table_new(2 + p->objectives,5,0);
  gtk_container_add (GTK_CONTAINER (p->private->toplevel), p2->top_table);
  gtk_container_set_border_width (GTK_CONTAINER (p->private->toplevel), 5);
  
  p2->dim_table = gtk_table_new(p->dimensions,4,0);
  gtk_table_attach(GTK_TABLE(p2->top_table),p2->dim_table,0,5,1+p->objectives,2+p->objectives,
		   GTK_EXPAND|GTK_FILL,0,0,5);
  
  /* graph */
  p->private->graph = GTK_WIDGET(plot_new(recompute_callback_2d,p,
				  (void *)(void *)_sushiv_panel2d_crosshairs_callback,p,
				  box_callback,p,0)); 
  gtk_table_attach(GTK_TABLE(p2->top_table),p->private->graph,0,5,0,1,
		   GTK_EXPAND|GTK_FILL,GTK_EXPAND|GTK_FILL,0,5);

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
    gtk_table_attach(GTK_TABLE(p2->top_table),label,0,1,i+1,i+2,
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
      gtk_table_attach(GTK_TABLE(p2->top_table),menu,4,5,i+1,i+2,
		       GTK_SHRINK,GTK_SHRINK,5,0);
      p2->range_pulldowns[i] = menu;
    }

    /* the range mapping slices/slider */ 
    sl[0] = slice_new(map_callback_2d,p->objective_list+i);
    sl[1] = slice_new(map_callback_2d,p->objective_list+i);
    sl[2] = slice_new(map_callback_2d,p->objective_list+i);

    gtk_table_attach(GTK_TABLE(p2->top_table),sl[0],1,2,i+1,i+2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p2->top_table),sl[1],2,3,i+1,i+2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(p2->top_table),sl[2],3,4,i+1,i+2,
		     GTK_EXPAND|GTK_FILL,0,0,0);
    p2->range_scales[i] = slider_new((Slice **)sl,3,o->scale->label_list,o->scale->val_list,
				    o->scale->vals,SLIDER_FLAG_INDEPENDENT_MIDDLE);

    slice_thumb_set((Slice *)sl[0],lo);
    slice_thumb_set((Slice *)sl[1],lo);
    slice_thumb_set((Slice *)sl[2],hi);
    mapping_setup(&p2->mappings[i],0.,1.,0);
    slider_set_gradient(p2->range_scales[i], &p2->mappings[i]);
  }

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
		     GTK_FILL,0,10,0);
    
    /* x/y radio buttons */
    if(!(d->flags & SUSHIV_DIM_NO_X)){
      if(first_x)
	p2->dim_xb[i] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(first_x),"X");
      else{
	first_x = p2->dim_xb[i] = gtk_radio_button_new_with_label(NULL,"X");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p2->dim_xb[i]),TRUE);
      }
      gtk_table_attach(GTK_TABLE(p2->dim_table),p2->dim_xb[i],1,2,i,i+1,
		       0,0,10,0);
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
		       0,0,10,0);
    }

    p->private->dim_scales[i] = 
      _sushiv_new_dimension_widget(p->dimension_list+i,center_callback_2d,bracket_callback_2d);
    
    gtk_table_attach(GTK_TABLE(p2->dim_table),
		     GTK_WIDGET(p->private->dim_scales[i]->t),
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
  update_xy_availability(p);

  p2->popmenu = gtk_menu_new_twocol(p->private->toplevel,
				    panel_menulist,
				    panel_shortlist,
				    (void *)(void *)panel_calllist,
				    p);
  p2->graphmenu = gtk_menu_new_twocol(p->private->graph,
				      graph_menulist,
				      graph_shortlist,
				      (void *)(void *)graph_calllist,
				      p);

  update_context_menus(p);

  g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
                    G_CALLBACK (panel2d_keypress), p);
  gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

  gtk_widget_realize(p->private->toplevel);
  gtk_widget_realize(p->private->graph);
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

  // verify all the objectives have scales
  for(i=0;i<p->objectives;i++){
    if(!p->objective_list[i].o->scale){
      fprintf(stderr,"All objectives in a 2d panel must have a scale\n");
      return -EINVAL;
    }
  }

  p->private->realize = _sushiv_realize_panel2d;
  p->private->map_redraw = _sushiv_panel2d_map_redraw;
  p->private->legend_redraw = _sushiv_panel2d_legend_redraw;
  p->private->compute_action = _sushiv_panel_cooperative_compute_2d;
  p->private->request_compute = _mark_recompute_2d;
  p->private->crosshair_action = _sushiv_panel2d_crosshairs_callback;

  p->private->undo_log = panel2d_undo_log;
  p->private->undo_restore = panel2d_undo_restore;
  p->private->update_menus = update_context_menus;

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
  
  return 0;
}

