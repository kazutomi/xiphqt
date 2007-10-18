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

static void wrap_exit(sv_panel_t *dummy, GtkWidget *dummyw);
static void wrap_bg(sv_panel_t *p, GtkWidget *w);
static void wrap_grid(sv_panel_t *p, GtkWidget *w);
static void wrap_text(sv_panel_t *p, GtkWidget *w);
static void wrap_res(sv_panel_t *p, GtkWidget *w);
static void wrap_load(sv_panel_t *p, GtkWidget *dummy);
static void wrap_save(sv_panel_t *p, GtkWidget *dummy);
static void wrap_print(sv_panel_t *p, GtkWidget *dummy);
static void wrap_undo_up(sv_panel_t *p, GtkWidget *dummy);
static void wrap_undo_down(sv_panel_t *p, GtkWidget *dummy);
static void wrap_legend(sv_panel_t *p, GtkWidget *dummy);
static void wrap_escape(sv_panel_t *p, GtkWidget *dummy);
static void wrap_enter(sv_panel_t *p, GtkWidget *dummy);

static sv_propmap_t *bgmap[]={
  &(sv_propmap_t){"white","#ffffff",   "[<i>b</i>]",NULL,wrap_bg},
  &(sv_propmap_t){"gray","#a0a0a0",   "[<i>b</i>]",NULL,wrap_bg},
  &(sv_propmap_t){"blue","#000060",   "[<i>b</i>]",NULL,wrap_bg},
  &(sv_propmap_t){"black","#000000",   "[<i>b</i>]",NULL,wrap_bg},
  &(sv_propmap_t){"checks","checks", "[<i>b</i>]",NULL,wrap_bg},
  NULL
};

static sv_propmap_t *gridmap[]={
  &(sv_propmap_t){"light","#e6e6e6",   "[<i>g</i>]",NULL,wrap_grid},
  &(sv_propmap_t){"normal","#b4b4b4", "[<i>g</i>]",NULL,wrap_grid},
  &(sv_propmap_t){"dark","#181818",     "[<i>g</i>]",NULL,wrap_grid},
  &(sv_propmap_t){"tics","tics",     "[<i>g</i>]",NULL,wrap_grid},
  &(sv_propmap_t){"none","none",     "[<i>g</i>]",NULL,wrap_grid},
  NULL
};

static _sv_propmap_t *textmap[]={
  &(sv_propmap_t){"dark","#000000",   "[<i>t</i>]",NULL,wrap_text},
  &(sv_propmap_t){"light","#ffffff", "[<i>t</i>]",NULL,wrap_text},
  NULL
};

static _sv_propmap_t *legendmap[]={
  &(_sv_propmap_t){"none","none",       NULL,NULL,NULL},
  &(_sv_propmap_t){"shadowed","shadowed", NULL,NULL,NULL},
  &(_sv_propmap_t){"boxed","boxed",       NULL,NULL,NULL},
  NULL
};

static _sv_propmap_t *menu[]={
  &(_sv_propmap_t){"Open",0,"[<i>o</i>]",NULL,wrap_load},
  &(_sv_propmap_t){"Save",0,"[<i>s</i>]",NULL,wrap_save},
  &(_sv_propmap_t){"Print/Export",0,"[<i>p</i>]",NULL,wrap_print},

  &(_sv_propmap_t){"",0,NULL,NULL,NULL},

  &(_sv_propmap_t){"Undo",0,"[<i>bksp</i>]",NULL,wrap_undo_down},
  &(_sv_propmap_t){"Redo",0,"[<i>space</i>]",NULL,wrap_undo_up},
  &(_sv_propmap_t){"Start zoom box",0,"[<i>enter</i>]",NULL,wrap_enter},
  &(_sv_propmap_t){"Clear selection",0,"[<i>escape</i>]",NULL,wrap_escape},
  &(_sv_propmap_t){"Toggle Legend",0,"[<i>l</i>]",NULL,wrap_legend},

  &(_sv_propmap_t){"",9,NULL,NULL,NULL},

  &(_sv_propmap_t){"Background",0,"...",bgmap,NULL},
  &(_sv_propmap_t){"Text color",0,"...",textmap,NULL},
  &(_sv_propmap_t){"Grid mode",0,"...",gridmap,NULL},
  &(_sv_propmap_t){"Sampling",0,"...",resmap,NULL},

  &(_sv_propmap_t){"",0,NULL,NULL,NULL},

  &(_sv_propmap_t){"Quit",0,"[<i>q</i>]",NULL,wrap_exit},

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
      // error
      GtkWidget *dialog;
      if(errno == -EINVAL){
	dialog = gtk_message_dialog_new (NULL,0,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "Error parsing file '%s'",
					 _sv_filename);
      }else{
	dialog = gtk_message_dialog_new (NULL,0,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "Error opening file '%s': %s",
					 _sv_filename, strerror (errno));
      }
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

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

