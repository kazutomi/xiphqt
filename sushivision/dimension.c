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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "internal.h"

int _sv_dimensions=0;
sv_dim_t **_sv_dimension_list=NULL;
pthread_key_t   _sv_dim_key;

void _sv_dim_data_copy(sv_dim_data_t *dd,sv_dim_data_t *copy){



}

void _sv_dim_data_clear(sv_dim_data_t *dd){

}

/* A panel must deal with two different scales for an axis; the first
   is pixel-oriented to the actually displayed panel, the second
   covers the same range but is oriented to integer bin numbers in the
   underlying data vector (same as the display for continuous,
   differnet for discrete) */

_sv_scalespace_t _sv_dim_panelscale(sv_dim_data_t *dd,
				    int panel_w, int flip){
  
  int pneg=1, dimneg=1, spacing = 50;
  double lo=dd->lo, hi=dd->hi;
  _sv_scalespace_t ret={0};
  
  if(lo>hi) pneg = -1;
  if(dd->floor > dd->ceil) dimneg = -1;
  if(flip) pneg = -pneg;

  switch(dd->type){
  case SV_DIM_CONTINUOUS:
    ret=_sv_scalespace_linear(lo, hi, panel_w, spacing);
    break;

  case SV_DIM_DISCRETE:
    
    int lo_i =  rint(lo * dd->denominator / dd->numerator);
    int hi_i =  rint(hi * dd->denominator / dd->numerator);

    ret = _sv_scalespace_linear((lo_i-pneg*.4) * dd->numerator / dd->denominator,
				(hi_i+pneg*.4) * dd->numerator / dd->denominator,
				panel_w, spacing);    
    break;
  case SV_DIM_PICKLIST:
    fprintf(stderr,"ERROR: Cannot iterate over picklist dimension!\n"
	    "\tA picklist dimension may not be a panel axis.\n");
    break;

  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in dimension_datascale.\n");
    break;

  }

  return ret;
}

_sv_scalespace_t _sv_dim_datascale(sv_dim_data_t *dd,
				   _sv_scalespace_t *panel,
				   int data_w,
				   int floor){
  
  /* the panel scales may be reversed (the easiest way to do y)
     and/or the dimension may have an inverted scale. */
  int pneg=1, dimneg=1;
  int panel_w = (panel?panel->pixels:0);
  _sv_scalespace_t ret={0};
  double lo=dd->lo, hi=dd->hi;

  if(lo>hi) pneg = -1;
  if(dd->floor > dd->ceil) dimneg = -1;
  if(flip) pneg = -pneg;
      
  switch(dd->type){
  case SV_DIM_CONTINUOUS:

    ret = _sv_scalespace_linear(lo, hi, data_w, 1);
    
    if(panel  && data_w != panel_w){
	
      /* if possible, the data/iterator scales should cover the entire pane exposed
	 by the panel scale so long as there's room left to extend them without
	 overflowing the lo/hi fenceposts */
      while(1){
	double panel2 = _sv_scalespace_value(panel,panel_w-1)*pneg;
	double data2 = _sv_scalespace_value(data,data_w-1)*pneg;
	
	if(data2>=panel2)break;
	data_w++;
      }
      
      data->pixels = data_w;
    }
    break;
    
  case SV_DIM_DISCRETE:
    
    /* return a scale that when iterated will only hit values
       quantized to the discrete base */
    /* what is the absolute base? */
    int floor_i =  rint(dd->floor * dd->denominator / dd->numerator);
    int ceil_i =  rint(dd->ceil * dd->denominator / dd->numerator);
    
    int lo_i =  rint(lo * dd->denominator / dd->numerator);
    int hi_i =  rint(hi * dd->denominator / dd->numerator);

    if(panel){
      /* if possible, the data scale should cover the entire pane
	 exposed by the panel scale so long as there's room left to
	 extend them without overflowing the lo/hi fenceposts */
      double panel1 = _sv_scalespace_value(panel,0)*pneg;
      double panel2 = _sv_scalespace_value(panel,panel->pixels)*pneg;
      double data1 = (double)(lo_i-.49*pneg) * dd->numerator / dd->denominator*pneg;
      double data2 = (double)(hi_i-.51*pneg) * dd->numerator / dd->denominator*pneg;
	
      while(data1 > panel1 && lo_i*dimneg > floor_i*dimneg){
	lo_i -= pneg;
	data1 = (double)(lo_i-.49*pneg) * dd->numerator / dd->denominator*pneg;
      }
      
      while(data2 < panel2 && hi_i*dimneg <= ceil_i*dimneg){ // inclusive upper
	hi_i += pneg;
	data2 = (double)(hi_i-.51*pneg) * dd->numerator / dd->denominator*pneg;
      }
      
    }else{
      hi_i += pneg; // because upper bound is inclusive, and this val is one-past
    }

    /* cosmetic adjustment complete, generate the scales */
    data_w = abs(hi_i-lo_i);
    
    ret = _sv_scalespace_linear((double)lo_i * dd->numerator / dd->denominator,
				(double)hi_i * dd->numerator / dd->denominator,
				data_w, 1);
    
    break;

  case SV_DIM_PICKLIST:
    fprintf(stderr,"ERROR: Cannot iterate over picklist dimension!\n"
	    "\tA picklist dimension may not be a panel axis.\n");
    break;
  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in dimension_datascale.\n");
    break;
  }
  
  return ret;
}

int _sv_dim_scales_from_panel(sv_dim_t *d,
			      _sv_scalespace_t panel,
			      int data_w,
			      _sv_scalespace_t *data, 
			      _sv_scalespace_t *iter){
  
  return _sv_dim_scales(d,
			panel.lo,
			panel.hi,
			panel.pixels,
			data_w,
			panel.spacing,
			panel.legend,
			&panel, // dummy
			data, 
			iter);
}

static double quantize_val(sv_dim_t *d, double val){
  if(d->type == SV_DIM_DISCRETE){
    val *= d->private->denominator;
    val /= d->private->numerator;
    
    val = rint(val);
    
    val *= d->private->numerator;
    val /= d->private->denominator;
  }
  return val;
}

static void _sv_dim_center_callback(void *data, int buttonstate){
  gdk_threads_enter();

  _sv_dim_widget_t *dw = (_sv_dim_widget_t *)data;

  sv_dim_t *d = dw->dl->d;
  double val = _sv_slider_get_value(dw->scale,1);
  char buffer[80];
  
  val = quantize_val(d,val);
  
  if(buttonstate == 0){
    _sv_undo_push();
    _sv_undo_suspend();
  }
  
  if(d->val != val){
    int i;
    
    d->val = val;
    
    if(d->private->value_callback) 
      d->private->value_callback(d,d->private->value_callback_data);

    /* dims can be shared amongst multiple widgets; all must be updated */
    for(i=0;i<d->private->widgets;i++){
      _sv_dim_widget_t *w = d->private->widget_list[i];
      if(w->scale) // all shared widgets had better have scales, but bulletproof in case
	_sv_slider_set_value(w->scale,1,val);
    }
    
    /* dims can be shared amongst multiple widgets; all must get callbacks */
    for(i=0;i<d->private->widgets;i++){
      _sv_dim_widget_t *w = d->private->widget_list[i];
      w->center_callback(w->dl);
    }

  }
  
  if(buttonstate == 2)
    _sv_undo_resume();
  
  snprintf(buffer,80,"%.10g",d->bracket[0]);
  gtk_entry_set_text(GTK_ENTRY(dw->entry[0]),buffer);
  snprintf(buffer,80,"%.10g",d->val);
  gtk_entry_set_text(GTK_ENTRY(dw->entry[1]),buffer);
  snprintf(buffer,80,"%.10g",d->bracket[1]);
  gtk_entry_set_text(GTK_ENTRY(dw->entry[2]),buffer);
  
  gdk_threads_leave();
}

static void _sv_dim_bracket_callback(void *data, int buttonstate){
  gdk_threads_enter();

  _sv_dim_widget_t *dw = (_sv_dim_widget_t *)data;

  sv_dim_t *d = dw->dl->d;
  double lo = _sv_slider_get_value(dw->scale,0);
  double hi = _sv_slider_get_value(dw->scale,2);
  char buffer[80];
  
  hi = quantize_val(d,hi);
  lo = quantize_val(d,lo);
  
  if(buttonstate == 0){
    _sv_undo_push();
    _sv_undo_suspend();
  }
  
  if(d->bracket[0] != lo || d->bracket[1] != hi){
    int i;
    
    d->bracket[0] = lo;
    d->bracket[1] = hi;
    
    /* dims can be shared amongst multiple widgets; all must be updated */
    for(i=0;i<d->private->widgets;i++){
      _sv_dim_widget_t *w = d->private->widget_list[i];
      if(w->scale){ // all shared widgets had better have scales, but bulletproof in case
	_sv_slider_set_value(w->scale,0,lo);
	_sv_slider_set_value(w->scale,2,hi);
      }
    }
    
    /* dims can be shared amongst multiple widgets; all must get callbacks */
    for(i=0;i<d->private->widgets;i++){
      _sv_dim_widget_t *w = d->private->widget_list[i];
      w->bracket_callback(w->dl);
    }
  }
  
  if(buttonstate == 2)
    _sv_undo_resume();
  
  snprintf(buffer,80,"%.10g",d->bracket[0]);
  gtk_entry_set_text(GTK_ENTRY(dw->entry[0]),buffer);
  snprintf(buffer,80,"%.10g",d->val);
  gtk_entry_set_text(GTK_ENTRY(dw->entry[1]),buffer);
  snprintf(buffer,80,"%.10g",d->bracket[1]);
  gtk_entry_set_text(GTK_ENTRY(dw->entry[2]),buffer);
  
  gdk_threads_leave();
}

static void _sv_dim_dropdown_callback(GtkWidget *dummy, void *data){
  gdk_threads_enter();

  _sv_dim_widget_t *dw = (_sv_dim_widget_t *)data;

  sv_dim_t *d = dw->dl->d;
  int bin = gtk_combo_box_get_active(GTK_COMBO_BOX(dw->menu));
  double val = d->scale->val_list[bin];
    
  _sv_undo_push();
  _sv_undo_suspend();
    
  if(d->val != val){
    int i;
    
    d->val = val;
    d->bracket[0] = val;
    d->bracket[1] = val;
    
    if(d->private->value_callback) 
      d->private->value_callback(d,d->private->value_callback_data);

    /* dims can be shared amongst multiple widgets; all must be updated */
    for(i=0;i<d->private->widgets;i++){
      _sv_dim_widget_t *w = d->private->widget_list[i];
      if(w->menu) // all shared widgets had better have scales, but bulletproof in case
	gtk_combo_box_set_active(GTK_COMBO_BOX(w->menu),bin);
    }
    
    /* dims can be shared amongst multiple widgets; all must get callbacks */
    for(i=0;i<d->private->widgets;i++){
      _sv_dim_widget_t *w = d->private->widget_list[i];
      w->center_callback(w->dl);
    }

  }
  _sv_undo_resume();
  
  gdk_threads_leave();
}

/* undo/redo have to frob widgets; this is indirected here too */
int _sv_dim_widget_set_thumb(_sv_dim_widget_t *dw, int thumb, double val){
  sv_dim_t *d = dw->dl->d;

  switch(d->type){
  case SV_DIM_CONTINUOUS:
    _sv_slider_set_value(dw->scale,thumb,val);
    break;
  case SV_DIM_DISCRETE:
    val *= d->private->denominator;
    val /= d->private->numerator;

    val = rint(val);

    val *= d->private->numerator;
    val /= d->private->denominator;

    _sv_slider_set_value(dw->scale,thumb,val);
    break;
  case SV_DIM_PICKLIST:
    /* find the picklist val closest to matching requested val */
    if(thumb == 1){
      int best=-1;
      double besterr=0;
      int i;
      
      for(i=0;i<d->scale->vals;i++){
	double err = fabs(val - d->scale->val_list[i]);
	if( best == -1 || err < besterr){
	  best = i;
	  besterr = err;
	}
      }
      
      if(best > -1)
	gtk_combo_box_set_active(GTK_COMBO_BOX(dw->menu),best);
    }
    break;
  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in dimension_set_value.\n");
    errno = -EINVAL;
    return -EINVAL;
  }
  return 0;
}

int _sv_dim_set_thumb(sv_dim_t *d, int thumb, double val){
  if(!d->private->widgets){
    switch(thumb){
    case 0:
      d->bracket[0] = quantize_val(d,val);
      break;
    case 1:
      d->val = quantize_val(d,val);
      break;
    case 2:
      d->bracket[1] = quantize_val(d,val);
      break;
    default:
      errno = -EINVAL;
      return -EINVAL;
    }

    if(d->private->value_callback) 
      d->private->value_callback(d,d->private->value_callback_data);
    
  }else
    return _sv_dim_widget_set_thumb(d->private->widget_list[0],thumb,val);
  return 0;
}

/* external version with externalish API */
int sv_dim_set_value(double val){
  sv_dim_t *d = sv_dim(0);
  return _sv_dim_set_thumb(d,1,val);
}

int sv_dim_set_bracket(double lo, double hi){
  sv_dim_t *d = sv_dim(0);
  int ret = _sv_dim_widget_set_thumb(d->private->widget_list[0],0,lo);
  if(!ret) ret = _sv_dim_widget_set_thumb(d->private->widget_list[0],2,hi);
  return ret;
}

void _sv_dim_widget_set_thumb_active(_sv_dim_widget_t *dw, int thumb, int active){
  if(dw->scale)
    _sv_slider_set_thumb_active(dw->scale,thumb,active);
}

static int expander_level = 0; // avoid races due to calling main_loop internally
static int expander_loop = 0;

static void _sv_dim_expander_callback (GtkExpander *expander, _sv_dim_widget_t *dw){
  expander_level++;
  if(expander_level==1){

    // do not allow the plot to resize 
    _sv_plot_resizable(PLOT(dw->dl->p->private->graph),0);

    do{
      expander_loop = 0;

      // Prevent the resizing algorithm from frobbing the plot's box
      _gtk_box_freeze_child (GTK_BOX(dw->dl->p->private->topbox),
			     dw->dl->p->private->plotbox);

      // allow the toplevel to resize automatically
      gtk_window_set_policy (GTK_WINDOW (dw->dl->p->private->toplevel), FALSE, FALSE, TRUE);
      while(gtk_events_pending()){
	gtk_main_iteration();
	gdk_flush();
      }
      
      if (gtk_expander_get_expanded (expander)){
	gtk_widget_show(dw->entry[0]);
	gtk_widget_show(dw->entry[1]);
	gtk_widget_show(dw->entry[2]);	
      }else{
	gtk_widget_hide(dw->entry[0]);
	gtk_widget_hide(dw->entry[1]);
	gtk_widget_hide(dw->entry[2]);
      }

      // process this change
      while(gtk_events_pending()){
	gtk_main_iteration();
	gdk_flush();
      }
      
      // revert toplevel to user-resizing
      gtk_window_set_policy (GTK_WINDOW (dw->dl->p->private->toplevel), FALSE, TRUE, FALSE);
      while(gtk_events_pending()){
	gtk_main_iteration(); 
	gdk_flush();
      }

      // revert plot box to autofilling if user alters window size
      _gtk_box_unfreeze_child(GTK_BOX(dw->dl->p->private->topbox),
			      dw->dl->p->private->plotbox);
      while(gtk_events_pending()){
	gtk_main_iteration(); 
	gdk_flush();
      }

    } while(expander_loop);

    // lastly, allow plot to resize again
    _sv_plot_resizable(PLOT(dw->dl->p->private->graph),1);
    
  }else
    expander_loop=1;
  
  expander_level--; 
}

static void _sv_dim_entry_callback (GtkEntry *entry, _sv_slice_t *s){
  _sv_slice_thumb_set(s, atof(gtk_entry_get_text(entry)));
}

static gboolean _sv_dim_entry_refresh_callback (GtkEntry *entry, GdkEventFocus *event, double *v){
  char buffer[80];
  snprintf(buffer,80,"%.10g",*v);
  gtk_entry_set_text(entry,buffer);
  return FALSE;
}

static void _sv_dim_entry_active_callback(void *e, int active){
  gtk_widget_set_sensitive(GTK_WIDGET(e),active);
}

_sv_dim_widget_t *_sv_dim_widget_new(sv_dim_list_t *dl,   
				     void (*center_callback)(sv_dim_list_t *),
				     void (*bracket_callback)(sv_dim_list_t *)){
  
  _sv_dim_widget_t *dw = calloc(1, sizeof(*dw));
  sv_dim_t *d = dl->d;

  dw->dl = dl;
  dw->center_callback = center_callback;
  dw->bracket_callback = bracket_callback;

  switch(d->type){
  case SV_DIM_CONTINUOUS:
  case SV_DIM_DISCRETE:
    /* Continuous and discrete dimensions get sliders */
    {
      double v[3];
      GtkWidget **sl = calloc(3,sizeof(*sl));
      GtkWidget *exp = gtk_expander_new(NULL);
      GtkTable *st = GTK_TABLE(gtk_table_new(2,4,0));

      v[0]=d->bracket[0];
      v[1]=d->val;
      v[2]=d->bracket[1];

      sl[0] = _sv_slice_new(_sv_dim_bracket_callback,dw);
      sl[1] = _sv_slice_new(_sv_dim_center_callback,dw);
      sl[2] = _sv_slice_new(_sv_dim_bracket_callback,dw);
      dw->entry[0] = gtk_entry_new();
      dw->entry[1] = gtk_entry_new();
      dw->entry[2] = gtk_entry_new();

      gtk_entry_set_width_chars(GTK_ENTRY(dw->entry[0]),0);
      gtk_entry_set_width_chars(GTK_ENTRY(dw->entry[1]),0);
      gtk_entry_set_width_chars(GTK_ENTRY(dw->entry[2]),0);
     
      gtk_table_attach(st,exp,0,1,0,1,
		       GTK_SHRINK,0,0,0);

      gtk_table_attach(st,sl[0],1,2,0,1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(st,sl[1],2,3,0,1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(st,sl[2],3,4,0,1,
		       GTK_EXPAND|GTK_FILL,0,0,0);

      gtk_widget_set_no_show_all(dw->entry[0], TRUE);
      gtk_widget_set_no_show_all(dw->entry[1], TRUE);
      gtk_widget_set_no_show_all(dw->entry[2], TRUE);
      gtk_table_attach(st,dw->entry[0],1,2,1,2,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(st,dw->entry[1],2,3,1,2,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(st,dw->entry[2],3,4,1,2,
		       GTK_EXPAND|GTK_FILL,0,0,0);

      dw->scale = _sv_slider_new((_sv_slice_t **)sl,3,d->scale->label_list,d->scale->val_list,
				 d->scale->vals,0);
      if(d->type == SV_DIM_DISCRETE)
	_sv_slider_set_quant(dw->scale,d->private->numerator,d->private->denominator);

      _sv_slice_thumb_set((_sv_slice_t *)sl[0],v[0]);
      _sv_slice_thumb_set((_sv_slice_t *)sl[1],v[1]);
      _sv_slice_thumb_set((_sv_slice_t *)sl[2],v[2]);

      g_signal_connect_after (G_OBJECT (exp), "activate",
			      G_CALLBACK (_sv_dim_expander_callback), dw);

      g_signal_connect (G_OBJECT (dw->entry[0]), "activate",
			G_CALLBACK (_sv_dim_entry_callback), sl[0]);
      g_signal_connect (G_OBJECT (dw->entry[1]), "activate",
			G_CALLBACK (_sv_dim_entry_callback), sl[1]);
      g_signal_connect (G_OBJECT (dw->entry[2]), "activate",
			G_CALLBACK (_sv_dim_entry_callback), sl[2]);

      g_signal_connect (G_OBJECT (dw->entry[0]), "focus-out-event",
			G_CALLBACK (_sv_dim_entry_refresh_callback), &d->bracket[0]);
      g_signal_connect (G_OBJECT (dw->entry[1]), "focus-out-event",
			G_CALLBACK (_sv_dim_entry_refresh_callback), &d->val);
      g_signal_connect (G_OBJECT (dw->entry[2]), "focus-out-event",
			G_CALLBACK (_sv_dim_entry_refresh_callback), &d->bracket[1]);

      _sv_slice_set_active_callback((_sv_slice_t *)sl[0], _sv_dim_entry_active_callback, dw->entry[0]);
      _sv_slice_set_active_callback((_sv_slice_t *)sl[1], _sv_dim_entry_active_callback, dw->entry[1]);
      _sv_slice_set_active_callback((_sv_slice_t *)sl[2], _sv_dim_entry_active_callback, dw->entry[2]);

      dw->t = GTK_WIDGET(st);
    }
    break;
  case SV_DIM_PICKLIST:
    /* picklist dimensions get a wide dropdown */
    dw->t = gtk_table_new(1,1,0);

    {
      int j;
      dw->menu = _gtk_combo_box_new_markup();
      for(j=0;j<d->scale->vals;j++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (dw->menu), d->scale->label_list[j]);

      g_signal_connect (G_OBJECT (dw->menu), "changed",
			G_CALLBACK (_sv_dim_dropdown_callback), dw);
      
      gtk_table_attach(GTK_TABLE(dw->t),dw->menu,0,1,0,1,
		       GTK_EXPAND|GTK_FILL,GTK_SHRINK,0,2);
      _sv_dim_widget_set_thumb(dw,1,d->val);
      //gtk_combo_box_set_active(GTK_COMBO_BOX(dw->menu),0);
    }
    break;
  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in new_dimension_widget.\n");
    break;
  }

  /* add widget to dimension */
  if(!d->private->widget_list){
    d->private->widget_list = calloc (1, sizeof(*d->private->widget_list));
  }else{
    d->private->widget_list = realloc (d->private->widget_list,
				       (d->private->widgets+1) * sizeof(*d->private->widget_list));
  }
  d->private->widget_list[d->private->widgets] = dw;
  d->private->widgets++;

  return dw;
};

sv_dim_t *sv_dim_new(char *name){
  int number;
  sv_dim_t *d;
  int i;

  _sv_token *decl = _sv_tokenize_declparam(name);
  
  if(!decl){
    fprintf(stderr,"sushivision: Unable to parse dimension declaration \"%s\".\n",name);
    errno = -EINVAL;
    return NULL;
  }

  if(_sv_dimensions == 0){
    number=0;
    _sv_dimension_list = calloc (number+1,sizeof(*_sv_dimension_list));
    _sv_dimensions=1;
  }else{
    for(number=0;number<_sv_dimensions;number++)
      if(!_sv_dimension_list[number])break;
    if(number==_sv_dimensions){
      _sv_dimensions=number+1;
      _sv_dimension_list = realloc (_sv_dimension_list,_sv_dimensions * sizeof(*_sv_dimension_list));
    }
  }
  
  d = _sv_dimension_list[number] = calloc(1, sizeof(**_sv_dimension_list));
  d->number = number;
  d->name = strdup(decl->name);
  d->legend = strdup(decl->label);
  d->type = SV_DIM_CONTINUOUS;
  d->private = calloc(1, sizeof(*d->private));
  d->private->numerator = 1;
  d->private->denominator = 1;

  // parse decllist
  for(i=0;i<decl->n;i++){
    char *f=decl->values[i]->s;
    double v=decl->values[i]->v;
 
    if(!strcmp(f,"continuous")){
      d->type = SV_DIM_CONTINUOUS;

    }else if(!strcmp(f,"picklist")){
      d->type = SV_DIM_PICKLIST;

    }else if(!strcmp(f,"discrete")){
      d->type = SV_DIM_DISCRETE;

    }else if(!strcmp(f,"picklist")){
      d->type = SV_DIM_PICKLIST;
      d->flags |= SV_DIM_NO_X | SV_DIM_NO_Y;

    }else if(!strcmp(f,"numerator")){
      if(isnan(v)){
	fprintf(stderr,"sushivision: Missing numerator value in \"%s\"\n.",name);
      }else{
	d->type = SV_DIM_PICKLIST;
	d->private->numerator = v;
      }

    }else if(!strcmp(f,"denominator")){
      if(isnan(v)){
	fprintf(stderr,"sushivision: Missing denominator value in \"%s\"\n.",name);
      }else if(v==0){
	fprintf(stderr,"sushivision: denominator value may not be zero\n.");
      }else{
	d->type = SV_DIM_PICKLIST;
	d->private->denominator = v;
      }

    }else{
      fprintf(stderr,"sushivision: Unknown parameter \"%s\" for dimension \"%s\".\n",
	      f,d->name);
    }
  }

  pthread_setspecific(_sv_dim_key, (void *)d);

  _sv_token_free(decl);
  return d;
}

// XXXX need to recompute after
// XXXX need to add scale cloning to compute to make this safe in callbacks
int sv_dim_set_scale(sv_scale_t *scale){
  sv_dim_t *d = sv_dim(0);
  if(!d) return -EINVAL;

  if(d->scale)
    sv_scale_free(d->scale); // always a deep copy we own
  
  d->scale = sv_scale_copy(scale);

  // in the runtime version, don't just blindly reset values!
  d->bracket[0]=scale->val_list[0];
  d->bracket[1]=scale->val_list[d->scale->vals-1];

  if(d->bracket[0] < d->bracket[1]){
    if(d->val<d->bracket[0])d->val=d->bracket[0];
    if(d->val>d->bracket[1])d->val=d->bracket[1];
  }else{
    if(d->val>d->bracket[0])d->val=d->bracket[0];
    if(d->val<d->bracket[1])d->val=d->bracket[1];
  }
  // redraw the slider

  return 0;
}

// XXXX need to recompute after
// XXXX need to add scale cloning to compute to make this safe in callbacks
int sv_dim_make_scale(char *format){
  sv_dim_t *d = sv_dim(0);
  sv_scale_t *scale;
  int ret;
  char *name=_sv_tokenize_escape(d->name);
  char *label=_sv_tokenize_escape(d->legend);
  char *arg=calloc(strlen(name)+strlen(label)+2,sizeof(*arg));

  strcat(arg,name);
  strcat(arg,":");
  strcat(arg,label);
  free(name);
  free(label);

  if(!d){
    free(arg);
    return -EINVAL;
  }
  scale = sv_scale_new(arg,format);
  free(arg);
  if(!scale)return errno;
  
  d->scale = scale;
  d->bracket[0]=scale->val_list[0];
  d->bracket[1]=scale->val_list[d->scale->vals-1];

  if(d->bracket[0] < d->bracket[1]){
    if(d->val<d->bracket[0])d->val=d->bracket[0];
    if(d->val>d->bracket[1])d->val=d->bracket[1];
  }else{
    if(d->val>d->bracket[0])d->val=d->bracket[0];
    if(d->val<d->bracket[1])d->val=d->bracket[1];
  }
  return ret;
}

static _sv_propmap_t *typemap[]={
  &(_sv_propmap_t){"continuous",SV_DIM_CONTINUOUS, NULL,NULL,NULL},
  &(_sv_propmap_t){"discrete",SV_DIM_DISCRETE,     NULL,NULL,NULL},
  &(_sv_propmap_t){"picklist",SV_DIM_PICKLIST,     NULL,NULL,NULL},
  NULL
};

int _sv_dim_load(sv_dim_t *d,
		 _sv_undo_t *u,
		 xmlNodePtr dn,
		 int warn){

  // check name 
  _xmlCheckPropS(dn,"name",d->name,"Dimension %d name mismatch in save file.",d->number,&warn);
  
  // check type
  _xmlCheckMap(dn,"type",typemap, d->type, "Dimension %d type mismatch in save file.",d->number,&warn);
  
  // load vals
  _xmlGetPropF(dn,"low-bracket", &u->dim_vals[0][d->number]);
  _xmlGetPropF(dn,"value", &u->dim_vals[1][d->number]);
  _xmlGetPropF(dn,"high-bracket", &u->dim_vals[2][d->number]);

  return warn;
}

int _sv_dim_save(sv_dim_t *d, xmlNodePtr instance){  
  if(!d) return 0;
  int ret=0;

  xmlNodePtr dn = xmlNewChild(instance, NULL, (xmlChar *) "dimension", NULL);

  _xmlNewPropI(dn, "number", d->number);
  _xmlNewPropS(dn, "name", d->name);
  _xmlNewMapProp(dn, "type", typemap, d->type);

  switch(d->type){
  case SV_DIM_CONTINUOUS:
  case SV_DIM_DISCRETE:
    _xmlNewPropF(dn, "low-bracket", d->bracket[0]);
    _xmlNewPropF(dn, "high-bracket", d->bracket[1]);
    
  case SV_DIM_PICKLIST:
    _xmlNewPropF(dn, "value", d->val);
    break;
  }

  return ret;
}

int sv_dim_callback_value (int (*callback)(sv_dim_t *, void *), void *data){
  sv_dim_t *d = sv_dim(0);
  if(!d) return -EINVAL;
  
  d->private->value_callback = callback;
  d->private->value_callback_data = data;
  return 0;
}

sv_dim_t *sv_dim(char *name){
  int i;
  
  if(name == NULL || name == 0 || !strcmp(name,"")){
    return (sv_dim_t *)pthread_getspecific(_sv_dim_key);
    
  }
  for(i=0;i<_sv_dimensions;i++){
    sv_dim_t *d=_sv_dimension_list[i];
    if(d && d->name && !strcmp(name,d->name)){
      pthread_setspecific(_sv_dim_key, (void *)d);
      return d;
    }
  }
  return NULL;
}
