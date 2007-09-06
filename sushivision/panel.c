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
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <cairo-ft.h>
#include "internal.h"

extern void _sv_wake_workers(void);
static void wrap_exit(sv_panel_t *dummy, GtkWidget *dummyw);
static void wrap_bg(sv_panel_t *p, GtkWidget *w);
static void wrap_grid(sv_panel_t *p, GtkWidget *w);
static void wrap_text(sv_panel_t *p, GtkWidget *w);
static void wrap_res(sv_panel_t *p, GtkWidget *w);
static void wrap_load(sv_panel_t *p, GtkWidget *dummy);
static void wrap_save(sv_panel_t *p, GtkWidget *dummy);
static void _sv_panel_print(sv_panel_t *p, GtkWidget *dummy);
static void wrap_undo_up(sv_panel_t *p, GtkWidget *dummy);
static void wrap_undo_down(sv_panel_t *p, GtkWidget *dummy);
static void wrap_legend(sv_panel_t *p, GtkWidget *dummy);
static void wrap_escape(sv_panel_t *p, GtkWidget *dummy);
static void wrap_enter(sv_panel_t *p, GtkWidget *dummy);

static _sv_propmap_t *bgmap[]={
  &(_sv_propmap_t){"white",SV_BG_WHITE,   "[<i>b</i>]",NULL,wrap_bg},
  &(_sv_propmap_t){"black",SV_BG_BLACK,   "[<i>b</i>]",NULL,wrap_bg},
  &(_sv_propmap_t){"checks",SV_BG_CHECKS, "[<i>b</i>]",NULL,wrap_bg},
  NULL
};

static _sv_propmap_t *gridmap[]={
  &(_sv_propmap_t){"light",_SV_PLOT_GRID_LIGHT,   "[<i>g</i>]",NULL,wrap_grid},
  &(_sv_propmap_t){"normal",_SV_PLOT_GRID_NORMAL, "[<i>g</i>]",NULL,wrap_grid},
  &(_sv_propmap_t){"dark",_SV_PLOT_GRID_DARK,     "[<i>g</i>]",NULL,wrap_grid},
  &(_sv_propmap_t){"tics",_SV_PLOT_GRID_TICS,     "[<i>g</i>]",NULL,wrap_grid},
  &(_sv_propmap_t){"none",_SV_PLOT_GRID_NONE,     "[<i>g</i>]",NULL,wrap_grid},
  NULL
};

static _sv_propmap_t *textmap[]={
  &(_sv_propmap_t){"dark",_SV_PLOT_TEXT_DARK,   "[<i>t</i>]",NULL,wrap_text},
  &(_sv_propmap_t){"light",_SV_PLOT_TEXT_LIGHT, "[<i>t</i>]",NULL,wrap_text},
  NULL
};

#define RES_DEF 0
#define RES_1_32 1
#define RES_1_16 2
#define RES_1_8 3
#define RES_1_4 4
#define RES_1_2 5
#define RES_1_1 6
#define RES_2_1 7
#define RES_4_1 8

// only used for the menus
static _sv_propmap_t *resmap[]={
  &(_sv_propmap_t){"default",RES_DEF,  "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"1:32",RES_1_32,     "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"1:16",RES_1_16,     "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"1:8",RES_1_8,      "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"1:4",RES_1_4,      "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"1:2",RES_1_2,      "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"1",RES_1_1,        "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"2:1",RES_2_1,      "[<i>m</i>]",NULL,wrap_res},
  &(_sv_propmap_t){"4:1",RES_4_1,      "[<i>m</i>]",NULL,wrap_res},
  NULL,
};

static _sv_propmap_t *crossmap[]={
  &(_sv_propmap_t){"no",0     ,NULL,NULL,NULL},
  &(_sv_propmap_t){"yes",1    ,NULL,NULL,NULL},
  NULL
};

static _sv_propmap_t *legendmap[]={
  &(_sv_propmap_t){"none",_SV_PLOT_LEGEND_NONE,       NULL,NULL,NULL},
  &(_sv_propmap_t){"shadowed",_SV_PLOT_LEGEND_SHADOW, NULL,NULL,NULL},
  &(_sv_propmap_t){"boxed",_SV_PLOT_LEGEND_BOX,       NULL,NULL,NULL},
  NULL
};

static _sv_propmap_t *menu[]={
  &(_sv_propmap_t){"Open",0,"[<i>o</i>]",NULL,wrap_load},
  &(_sv_propmap_t){"Save",1,"[<i>s</i>]",NULL,wrap_save},
  &(_sv_propmap_t){"Print/Export",2,"[<i>p</i>]",NULL,_sv_panel_print},

  &(_sv_propmap_t){"",3,NULL,NULL,NULL},

  &(_sv_propmap_t){"Undo",4,"[<i>bksp</i>]",NULL,&wrap_undo_down},
  &(_sv_propmap_t){"Redo",5,"[<i>space</i>]",NULL,&wrap_undo_up},
  &(_sv_propmap_t){"Start zoom box",6,"[<i>enter</i>]",NULL,&wrap_enter},
  &(_sv_propmap_t){"Clear selection",7,"[<i>escape</i>]",NULL,&wrap_escape},
  &(_sv_propmap_t){"Toggle Legend",8,"[<i>l</i>]",NULL,&wrap_legend},

  &(_sv_propmap_t){"",9,NULL,NULL,NULL},

  &(_sv_propmap_t){"Background",10,"...",bgmap,NULL},
  &(_sv_propmap_t){"Text color",11,"...",textmap,NULL},
  &(_sv_propmap_t){"Grid mode",12,"...",gridmap,NULL},
  &(_sv_propmap_t){"Sampling",13,"...",resmap,NULL},

  &(_sv_propmap_t){"",14,NULL,NULL,NULL},

  &(_sv_propmap_t){"Quit",15,"[<i>q</i>]",NULL,&wrap_exit},

  NULL
};

static void decide_text_inv(sv_panel_t *p){
  if(p->private->graph){
    _sv_plot_t *plot = PLOT(p->private->graph);
    if(p->private->bg_type == SV_BG_WHITE)
      _sv_plot_set_bg_invert(plot,_SV_PLOT_TEXT_DARK);
    else
      _sv_plot_set_bg_invert(plot,_SV_PLOT_TEXT_LIGHT);
  }
}

static void recompute_if_running(sv_panel_t *p){
  if(p->private->realized && p->private->graph)
    _sv_panel_recompute(p);
}

static void redraw_if_running(sv_panel_t *p){
  if(p->private->realized && p->private->graph){
    _sv_plot_draw_scales(PLOT(p->private->graph));
    _sv_panel_dirty_map(p);
    _sv_panel_dirty_legend(p);
  }
}

static void refg_if_running(sv_panel_t *p){
  if(p->private->realized && p->private->graph){
    _sv_plot_draw_scales(PLOT(p->private->graph));
    _sv_panel_dirty_legend(p);
  }
}

static void wrap_exit(sv_panel_t *dummy, GtkWidget *dummyw){
  _sv_clean_exit();
}

// precipitated actions perform undo push
static void wrap_enter(sv_panel_t *p, GtkWidget *dummy){
  _sv_plot_do_enter(PLOT(p->private->graph));
}

static void wrap_escape(sv_panel_t *p, GtkWidget *dummy){
  _sv_undo_push();
  _sv_undo_suspend();

  _sv_plot_set_crossactive(PLOT(p->private->graph),0);
  _sv_panel_dirty_legend(p);

  _sv_undo_resume();
}

static void wrap_legend(sv_panel_t *p, GtkWidget *dummy){
  _sv_undo_push();
  _sv_undo_suspend();

  _sv_plot_toggle_legend(PLOT(p->private->graph));
  _sv_panel_dirty_legend(p);

  _sv_undo_resume();
}

static void set_grid(sv_panel_t *p, int mode){
  _sv_undo_push();
  _sv_undo_suspend();

  _sv_plot_set_grid(PLOT(p->private->graph),mode);
  _sv_panel_update_menus(p);
  refg_if_running(p);

  _sv_undo_resume();
}

static void wrap_grid(sv_panel_t *p, GtkWidget *w){
  int pos = _gtk_menu_item_position(w);
  set_grid(p, gridmap[pos]->value);
}

static int set_background(sv_panel_t *p,
			  enum sv_background bg){
  
  sv_panel_internal_t *pi = p->private;
  
  _sv_undo_push();
  _sv_undo_suspend();

  pi->bg_type = bg;
  
  decide_text_inv(p);
  set_grid(p,_SV_PLOT_GRID_NORMAL);
  redraw_if_running(p);
  _sv_panel_update_menus(p);

  _sv_undo_resume();
  return 0;
}

static void wrap_bg(sv_panel_t *p, GtkWidget *w){
  int pos = _gtk_menu_item_position(w);
  set_background(p, bgmap[pos]->value);
}

static void cycle_bg(sv_panel_t *p){
  int menupos = _sv_propmap_pos(bgmap, p->private->bg_type) + 1;
  if(bgmap[menupos] == NULL) menupos = 0;
  set_background(p, bgmap[menupos]->value);
}

static void cycleB_bg(sv_panel_t *p){
  int menupos = _sv_propmap_pos(bgmap, p->private->bg_type) - 1;
  if(menupos<0) menupos = _sv_propmap_last(bgmap);
  set_background(p, bgmap[menupos]->value);
}

static void set_text(sv_panel_t *p, int mode){
  _sv_undo_push();
  _sv_undo_suspend();

  _sv_plot_set_bg_invert(PLOT(p->private->graph),mode);
  _sv_panel_update_menus(p);
  refg_if_running(p);

  _sv_undo_resume();
}

static void wrap_text(sv_panel_t *p, GtkWidget *w){
  int pos = _gtk_menu_item_position(w);
  set_text(p, textmap[pos]->value);
}

static void cycle_text(sv_panel_t *p){
  int menupos = _sv_propmap_pos(textmap, PLOT(p->private->graph)->bg_inv) + 1;
  if(textmap[menupos] == NULL) menupos = 0;
  set_text(p, textmap[menupos]->value);
}

static void cycle_grid(sv_panel_t *p){
  int menupos = _sv_propmap_pos(gridmap, PLOT(p->private->graph)->grid_mode) + 1;
  if(gridmap[menupos] == NULL) menupos = 0;
  set_grid(p, gridmap[menupos]->value);
}
static void cycleB_grid(sv_panel_t *p){
  int menupos = _sv_propmap_pos(gridmap, PLOT(p->private->graph)->grid_mode) - 1;
  if(menupos<0) menupos = _sv_propmap_last(gridmap);
  set_grid(p, gridmap[menupos]->value);
}

static void res_set(sv_panel_t *p, int n, int d){
  if(n != p->private->oversample_n ||
     d != p->private->oversample_d){

    _sv_undo_push();
    _sv_undo_suspend();
    
    p->private->oversample_n = n;
    p->private->oversample_d = d;
    _sv_panel_update_menus(p);
    recompute_if_running(p);

    _sv_undo_resume();
  }
}

// a little different; the menu value is not the internal setting
static void res_set_pos(sv_panel_t *p, int pos){
  p->private->menu_cursamp = pos;
  switch(pos){
  case RES_DEF:
    res_set(p,p->private->def_oversample_n,p->private->def_oversample_d);
    break;
  case RES_1_32:
    res_set(p,1,32);
    break;
  case RES_1_16:
    res_set(p,1,16);
    break;
  case RES_1_8:
    res_set(p,1,8);
    break;
  case RES_1_4:
    res_set(p,1,4);
    break;
  case RES_1_2:
    res_set(p,1,2);
    break;
  case RES_1_1:
    res_set(p,1,1);
    break;
  case RES_2_1:
    res_set(p,2,1);
    break;
  case RES_4_1:
    res_set(p,4,1);
    break;
  }
}

static void wrap_res(sv_panel_t *p, GtkWidget *w){
  int pos = _gtk_menu_item_position(w);
  res_set_pos(p, resmap[pos]->value);
}

static void cycle_res(sv_panel_t *p){
  int menupos = _sv_propmap_pos(resmap, p->private->menu_cursamp) + 1;
  if(resmap[menupos] == NULL) menupos = 0;
  res_set_pos(p, resmap[menupos]->value);
}

static void cycleB_res(sv_panel_t *p){
  int menupos = _sv_propmap_pos(resmap, p->private->menu_cursamp) - 1;
  if(menupos<0) menupos = _sv_propmap_last(resmap);
  res_set_pos(p, resmap[menupos]->value);
}

static GtkPrintSettings *printset=NULL;
static void _begin_print_handler (GtkPrintOperation *op,
				  GtkPrintContext   *context,
				  gpointer           dummy){

  gtk_print_operation_set_n_pages(op,1);

}

static void _print_handler(GtkPrintOperation *operation,
			   GtkPrintContext   *context,
			   gint               page_nr,
			   gpointer           user_data){

  cairo_t *c;
  gdouble w, h;
  sv_panel_t *p = (sv_panel_t *)user_data;

  c = gtk_print_context_get_cairo_context (context);
  w = gtk_print_context_get_width (context);
  h = gtk_print_context_get_height (context);
  
  p->private->print_action(p,c,w,h);
}

static void _sv_panel_print(sv_panel_t *p, GtkWidget *dummy){
  GtkPrintOperation *op = gtk_print_operation_new ();

  if (printset != NULL) 
    gtk_print_operation_set_print_settings (op, printset);
  
  g_signal_connect (op, "begin-print", 
		    G_CALLBACK (_begin_print_handler), p);
  g_signal_connect (op, "draw-page", 
		    G_CALLBACK (_print_handler), p);

  GError *err;
  GtkPrintOperationResult ret = gtk_print_operation_run (op,GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
							 NULL,&err);

  if (ret == GTK_PRINT_OPERATION_RESULT_ERROR) {
    GtkWidget *error_dialog = gtk_message_dialog_new (NULL,0,GTK_MESSAGE_ERROR,
					   GTK_BUTTONS_CLOSE,
					   "Error printing file:\n%s",
					   err->message);
    g_signal_connect (error_dialog, "response", 
		      G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show (error_dialog);
    g_error_free (err);
  }else if (ret == GTK_PRINT_OPERATION_RESULT_APPLY){
    if (printset != NULL)
      g_object_unref (printset);
    printset = g_object_ref (gtk_print_operation_get_print_settings (op));
  }
  g_object_unref (op);
}

static void wrap_undo_down(sv_panel_t *p, GtkWidget *dummy){
  _sv_undo_down();
}
static void wrap_undo_up(sv_panel_t *p, GtkWidget *dummy){
  _sv_undo_up();
}

static void wrap_save(sv_panel_t *p, GtkWidget *dummy){
  GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save",
						   NULL,
						   GTK_FILE_CHOOSER_ACTION_SAVE,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						   NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog), _sv_cwdname, NULL);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), _sv_dirname);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _sv_filebase);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    if(_sv_filebase)free(_sv_filebase);
    if(_sv_filename)free(_sv_filename);
    if(_sv_dirname)free(_sv_dirname);

    _sv_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    _sv_dirname = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    _sv_filebase = g_path_get_basename(_sv_filename);
    _sv_main_save();
  }

  gtk_widget_destroy (dialog);

}

static void wrap_load(sv_panel_t *p, GtkWidget *dummy){
  GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open",
						   NULL,
						   GTK_FILE_CHOOSER_ACTION_OPEN,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						   NULL);

  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog), _sv_cwdname, NULL);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), _sv_dirname);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    char *temp_filebase = _sv_filebase;
    char *temp_filename = _sv_filename;
    char *temp_dirname = _sv_dirname;
    _sv_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    _sv_dirname = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    _sv_filebase = g_path_get_basename(_sv_filename);

    if(_sv_main_load()){

      free(_sv_filebase);
      free(_sv_filename);
      free(_sv_dirname);

      _sv_filebase = temp_filebase;
      _sv_filename = temp_filename;
      _sv_dirname = temp_dirname;

    }else{
      free(temp_filebase);
      free(temp_filename);
      free(temp_dirname);
    }
  }

  gtk_widget_destroy (dialog);

}

void _sv_panel_update_menus(sv_panel_t *p){

  // is undo active?
  if(!_sv_undo_stack ||
     !_sv_undo_level){
    gtk_widget_set_sensitive(_gtk_menu_get_item(GTK_MENU(p->private->popmenu),4),FALSE);
  }else{
    gtk_widget_set_sensitive(_gtk_menu_get_item(GTK_MENU(p->private->popmenu),4),TRUE);
  }

  // is redo active?
  if(!_sv_undo_stack ||
     !_sv_undo_stack[_sv_undo_level] ||
     !_sv_undo_stack[_sv_undo_level+1]){
    gtk_widget_set_sensitive(_gtk_menu_get_item(GTK_MENU(p->private->popmenu),5),FALSE);
  }else{
    gtk_widget_set_sensitive(_gtk_menu_get_item(GTK_MENU(p->private->popmenu),5),TRUE);
  }

  // are we starting or enacting a zoom box?
  if(p->private->oldbox_active){ 
    _gtk_menu_alter_item_label(GTK_MENU(p->private->popmenu),6,"Zoom to box");
  }else{
    _gtk_menu_alter_item_label(GTK_MENU(p->private->popmenu),6,"Start zoom box");
  }

  // make sure menu reflects plot configuration
  _gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			     _sv_propmap_label_pos(menu,"Background"),
			     bgmap[_sv_propmap_pos(bgmap,p->private->bg_type)]->left);

  _gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			     _sv_propmap_label_pos(menu,"Text color"),
			     textmap[_sv_propmap_pos(textmap,PLOT(p->private->graph)->bg_inv)]->left);

  _gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			     _sv_propmap_label_pos(menu,"Grid mode"),
			     gridmap[_sv_propmap_pos(gridmap,PLOT(p->private->graph)->grid_mode)]->left);
   {
    char buffer[80];
    snprintf(buffer,60,"%d:%d",p->private->oversample_n,p->private->oversample_d);
    if(p->private->def_oversample_n == p->private->oversample_n &&
       p->private->def_oversample_d == p->private->oversample_d)
      strcat(buffer," (default)");
    _gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			       _sv_propmap_label_pos(menu,"Sampling"),buffer);
  }
}

static gboolean panel_keypress(GtkWidget *widget,
				 GdkEventKey *event,
				 gpointer in){
  sv_panel_t *p = (sv_panel_t *)in;
  //  sv_panel2d_t *p2 = (sv_panel2d_t *)p->internal;
  
  // check if the widget with focus is an Entry
  GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(widget));
  int entryp = (focused?GTK_IS_ENTRY(focused):0);

  // don't swallow modified keypresses
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK)return FALSE;

  switch(event->keyval){
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
  case GDK_Insert:case GDK_KP_Insert:
    return FALSE;
  }
  
  if(entryp){
    // we still filter, but differently
    switch(event->keyval){
    case GDK_BackSpace:
    case GDK_e:case GDK_E:
    case GDK_Return:case GDK_ISO_Enter:
      return FALSE;
    }
  }
        
  /* non-control keypresses */
  switch(event->keyval){
  case GDK_b:
    cycle_bg(p);
    return TRUE;
  case GDK_B:
    cycleB_bg(p);
    return TRUE;
  case GDK_t:case GDK_T:
    cycle_text(p);
    return TRUE;
  case GDK_g:
    cycle_grid(p);
    return TRUE;
  case GDK_G:
    cycleB_grid(p);
    return TRUE;
  case GDK_m:
    cycle_res(p);
    return TRUE;
  case GDK_M:
    cycleB_res(p);
    return TRUE;

  case GDK_s:
    wrap_save(p,NULL);
    return TRUE;
  case GDK_o:
    wrap_load(p,NULL);
    return TRUE;
    
   case GDK_Escape:
     wrap_escape(p,NULL);
    return TRUE;

  case GDK_Return:case GDK_ISO_Enter:
    wrap_enter(p,NULL);
    return TRUE;
   
  case GDK_Q:
  case GDK_q:
    // quit
    _sv_clean_exit();
    return TRUE;
    
  case GDK_BackSpace:
    // undo 
    _sv_undo_down();
    return TRUE;
    
  case GDK_r:
  case GDK_space:
    // redo/forward
    _sv_undo_up();
    return TRUE;

  case GDK_p:
    _sv_panel_print(p,NULL);
    return TRUE;

  case GDK_l:
    wrap_legend(p,NULL);
    return TRUE;
  } 

  return FALSE;
}

void _sv_panel_realize(sv_panel_t *p){
  if(p && !p->private->realized){
    p->private->realize(p);

    g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
		      G_CALLBACK (panel_keypress), p);
    gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

    p->private->realized=1;

    // generic things that happen in all panel realizations...

    // text black or white in the plot?
    decide_text_inv(p);
    p->private->popmenu = _gtk_menu_new_twocol(p->private->toplevel, menu, p);
    _sv_panel_update_menus(p);
    
  }
}

void _sv_map_set_throttle_time(sv_panel_t *p){
  struct timeval now;
  gettimeofday(&now,NULL);

  p->private->last_map_throttle = now.tv_sec*1000 + now.tv_usec/1000;
}

static int test_throttle_time(sv_panel_t *p){
  struct timeval now;
  long test;
  gettimeofday(&now,NULL);
  
  test = now.tv_sec*1000 + now.tv_usec/1000;
  if(p->private->last_map_throttle + 500 < test) 
    return 1;

  return 0;  
}

/* request a recomputation with full setup (eg, scales,
   etc) */
void _sv_panel_recompute(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->request_compute(p);
  gdk_threads_leave ();
}

/* use these to request a render/compute action after everything has
   already been set up to perform the op; the above full recompute
   request will eventually trigger a call here to kick off the actual
   computation.  Do panel subtype-specific setup, then wake workers
   with one of the below */
void _sv_panel_dirty_plot(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->plot_active = 1;
  p->private->plot_serialno++;
  p->private->plot_progress_count=0;
  p->private->plot_complete_count=0;
  gdk_threads_leave ();
  _sv_wake_workers();
}

void _sv_panel_dirty_map(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->map_active = 1;
  p->private->map_serialno++;
  p->private->map_progress_count=0;
  p->private->map_complete_count=0;
  gdk_threads_leave ();
  _sv_wake_workers();
}

void _sv_panel_dirty_map_throttled(sv_panel_t *p){
  gdk_threads_enter ();
  if(!p->private->map_active && test_throttle_time(p)){
     _sv_panel_dirty_map(p);
  }
  gdk_threads_leave ();
}

void _sv_panel_dirty_legend(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->legend_active = 1;
  p->private->legend_serialno++;
  p->private->legend_progress_count=0;
  p->private->legend_complete_count=0;
  gdk_threads_leave ();
  _sv_wake_workers();
}

/* use these to signal a computation is completed */
void _sv_panel_clean_plot(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->plot_active = 0;
  gdk_threads_leave ();
}

void _sv_panel_clean_map(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->map_active = 0;
  gdk_threads_leave ();
}

void _sv_panel_clean_legend(sv_panel_t *p){
  gdk_threads_enter ();
  p->private->legend_active = 0;
  gdk_threads_leave ();
}

int sv_panel_oversample(int number,
			int numer,
			int denom){
  
  if(number<0){
    fprintf(stderr,"sv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>_sv_panels || !_sv_panel_list[number]){
    fprintf(stderr,"sv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sv_panel_t *p = _sv_panel_list[number];
  sv_panel_internal_t *pi = p->private;

  if(denom == 0){
    fprintf(stderr,"sv_panel_oversample: A denominator of zero is invalid\n");
    return -EINVAL;
  }

  pi->def_oversample_n = pi->oversample_n = numer;
  pi->def_oversample_d = pi->oversample_d = denom;
  recompute_if_running(p);
  return 0;
}

int sv_panel_background(int number,
			enum sv_background bg){

  if(number<0){
    fprintf(stderr,"sv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>_sv_panels || !_sv_panel_list[number]){
    fprintf(stderr,"sv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sv_panel_t *p = _sv_panel_list[number];
  return set_background(p,bg);
}

sv_panel_t * _sv_panel_new(int number,
			   char *name, 
			   sv_obj_t **objectives,
			   char *dimensionlist,
			   unsigned flags){

  sv_panel_t *p;
  _sv_tokenlist *dim_tokens;
  int i;

  if(number<0){
    fprintf(stderr,"Panel number must be >= 0\n");
    errno = -EINVAL;
    return NULL;
  }

  if(number<_sv_panels){
    if(_sv_panel_list[number]!=NULL){
      fprintf(stderr,"Panel number %d already exists\n",number);
      errno = -EINVAL;
      return NULL;
    }
  }else{
    if(_sv_panels == 0){
      _sv_panel_list = calloc (number+1,sizeof(*_sv_panel_list));
    }else{
      _sv_panel_list = realloc (_sv_panel_list,(number+1) * sizeof(*_sv_panel_list));
      memset(_sv_panel_list + _sv_panels, 0, sizeof(*_sv_panel_list)*(number+1 - _sv_panels));
    }
    _sv_panels = number+1; 
  }

  p = _sv_panel_list[number] = calloc(1, sizeof(**_sv_panel_list));

  p->number = number;
  p->name = strdup(name);
  p->flags = flags;
  p->private = calloc(1, sizeof(*p->private));
  p->private->spinner = _sv_spinner_new();
  p->private->def_oversample_n = p->private->oversample_n = 1;
  p->private->def_oversample_d = p->private->oversample_d = 1;

  i=0;
  while(objectives && objectives[i])i++;
  p->objectives = i;
  p->objective_list = malloc(i*sizeof(*p->objective_list));
  for(i=0;i<p->objectives;i++){
    p->objective_list[i].o = (sv_obj_t *)objectives[i];
    p->objective_list[i].p = p;
  }

  i=0;
  dim_tokens = _sv_tokenize_namelist(dimensionlist);
  p->dimensions = dim_tokens->n;
  p->dimension_list = malloc(p->dimensions*sizeof(*p->dimension_list));
  for(i=0;i<p->dimensions;i++){
    char *name = dim_tokens->list[i]->name;
    sv_dim_t *d = sv_dim(name);

    if(!d){
      fprintf(stderr,"Panel %d (\"%s\"): Dimension \"%s\" does not exist\n",
	      number,p->name,name);
      errno = -EINVAL;
      //XXX leak
      return NULL;
    }

    if(!d->scale){
      fprintf(stderr,"Panel %d (\"%s\"): Dimension \"%s\" has a NULL scale\n",
	      number,p->name,name);
      errno = -EINVAL;
      //XXX leak
      return NULL;
    }

    p->dimension_list[i].d = d;
    p->dimension_list[i].p = p;
  }

  return p;
}

void _sv_panel_undo_log(sv_panel_t *p, _sv_panel_undo_t *u){
  u->cross_mode = PLOT(p->private->graph)->cross_active;
  u->legend_mode = PLOT(p->private->graph)->legend_active;
  u->grid_mode = PLOT(p->private->graph)->grid_mode;
  u->text_mode = PLOT(p->private->graph)->bg_inv;
  u->bg_mode = p->private->bg_type;
  u->menu_cursamp = p->private->menu_cursamp;
  u->oversample_n = p->private->oversample_n;
  u->oversample_d = p->private->oversample_d;

  // panel-subtype-specific population
  p->private->undo_log(u,p);
}

void _sv_panel_undo_restore(sv_panel_t *p, _sv_panel_undo_t *u){
  // go in through setting routines
  _sv_plot_set_crossactive(PLOT(p->private->graph),u->cross_mode);
  _sv_plot_set_legendactive(PLOT(p->private->graph),u->legend_mode);
  set_background(p, u->bg_mode); // must be first; it can frob grid and test
  set_text(p, u->text_mode);
  set_grid(p, u->grid_mode);
  p->private->menu_cursamp = u->menu_cursamp;
  res_set(p, u->oversample_n, u->oversample_d);

  // panel-subtype-specific restore
  p->private->undo_restore(u,p);

  _sv_panel_dirty_legend(p); 
}

int _sv_panel_save(sv_panel_t *p, xmlNodePtr instance){  
  if(!p) return 0;
  char buffer[80];
  int ret=0;

  xmlNodePtr pn = xmlNewChild(instance, NULL, (xmlChar *) "panel", NULL);
  xmlNodePtr n;

  _xmlNewPropI(pn, "number", p->number);
  _xmlNewPropS(pn, "name", p->name);

  // let the panel subtype handler fill in type
  // we're only saving settings independent of subtype

  // background
  n = xmlNewChild(pn, NULL, (xmlChar *) "background", NULL);
  _xmlNewMapProp(n, "color", bgmap, p->private->bg_type);

  // grid
  n = xmlNewChild(pn, NULL, (xmlChar *) "grid", NULL);
  _xmlNewMapProp(n, "mode", gridmap, PLOT(p->private->graph)->grid_mode);

  // crosshairs
  n = xmlNewChild(pn, NULL, (xmlChar *) "crosshairs", NULL);
  _xmlNewMapProp(n, "active", crossmap, PLOT(p->private->graph)->cross_active);

  // legend
  n = xmlNewChild(pn, NULL, (xmlChar *) "legend", NULL);
  _xmlNewMapProp(n,"mode", legendmap, PLOT(p->private->graph)->legend_active);

  // text
  n = xmlNewChild(pn, NULL, (xmlChar *) "text", NULL);
  _xmlNewMapProp(n,"color", textmap, PLOT(p->private->graph)->bg_inv);

  // resample
  n = xmlNewChild(pn, NULL, (xmlChar *) "sampling", NULL);
  snprintf(buffer,sizeof(buffer),"%d:%d",
	   p->private->oversample_n, p->private->oversample_d);
  xmlNewProp(n, (xmlChar *)"ratio", (xmlChar *)buffer);

  // subtype 
  if(p->private->save_action)
    ret |= p->private->save_action(p, pn);

  return ret;
}

int _sv_panel_load(sv_panel_t *p,
		   _sv_panel_undo_t *u,
		   xmlNodePtr pn,
		   int warn){

  // check name 
  _xmlCheckPropS(pn,"name",p->name,"Panel %d name mismatch in save file.",p->number,&warn);

  // background
  _xmlGetChildMap(pn, "background", "color", bgmap, &u->bg_mode,
		  "Panel %d unknown background setting", p->number, &warn);
  // grid
  _xmlGetChildMap(pn, "grid", "mode", gridmap, &u->grid_mode,
		  "Panel %d unknown grid mode setting", p->number, &warn);
  // crosshairs
  _xmlGetChildMap(pn, "crosshairs", "active", crossmap, &u->cross_mode,
		  "Panel %d unknown crosshair setting", p->number, &warn);
  // legend
  _xmlGetChildMap(pn, "legend", "mode", legendmap, &u->legend_mode,
		  "Panel %d unknown legend setting", p->number, &warn);
  // text
  _xmlGetChildMap(pn, "text", "color", textmap, &u->text_mode,
		  "Panel %d unknown text color setting", p->number, &warn);
  // resample
  char *prop = NULL;
  _xmlGetChildPropS(pn, "sampling", "ratio", &prop);
  if(prop){
    int res = sscanf(prop,"%d:%d", &u->oversample_n, &u->oversample_d);
    if(res<2){
      fprintf(stderr,"Unable to parse sample setting (%s) for panel %d.\n",prop,p->number);
      u->oversample_n = p->private->def_oversample_n;
      u->oversample_d = p->private->def_oversample_d;
    }
    if(u->oversample_d == 0) u->oversample_d = 1;
    xmlFree(prop);
  }
  
  // subtype 
  if(p->private->load_action)
    warn = p->private->load_action(p, u, pn, warn);
  
  // any unparsed elements? 
  xmlNodePtr n = pn->xmlChildrenNode;
  
  while(n){
    if (n->type == XML_ELEMENT_NODE) {
      _sv_first_load_warning(&warn);
      fprintf(stderr,"Unknown option (%s) set for panel %d.\n",n->name,p->number);
    }
    n = n->next; 
  }

  return warn;
}

int sv_panel_callback_recompute (sv_panel_t *p,
				 int (*callback)(sv_panel_t *p,void *data),
				 void *data){

  p->private->callback_precompute = callback;
  p->private->callback_precompute_data = data;
  return 0;
}

sv_dim_t *sv_panel_get_axis(sv_panel_t *p, char axis){
  switch(axis){

  case 'x': case 'X':
    return p->private->x_d;

  case 'y': case 'Y':
    return p->private->y_d;
  }
  
  return NULL;
}
