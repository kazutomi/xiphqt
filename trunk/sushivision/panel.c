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

extern void _sushiv_wake_workers(void);
static void wrap_exit(sushiv_panel_t *dummy, GtkWidget *dummyw);
static void wrap_bg(sushiv_panel_t *p, GtkWidget *w);
static void wrap_grid(sushiv_panel_t *p, GtkWidget *w);
static void wrap_text(sushiv_panel_t *p, GtkWidget *w);
static void wrap_res(sushiv_panel_t *p, GtkWidget *w);
static void do_load(sushiv_panel_t *p, GtkWidget *dummy);
static void do_save(sushiv_panel_t *p, GtkWidget *dummy);
static void _sushiv_panel_print(sushiv_panel_t *p, GtkWidget *dummy);
static void wrap_undo_up(sushiv_panel_t *p, GtkWidget *dummy);
static void wrap_undo_down(sushiv_panel_t *p, GtkWidget *dummy);
static void wrap_legend(sushiv_panel_t *p, GtkWidget *dummy);
static void wrap_escape(sushiv_panel_t *p, GtkWidget *dummy);
static void wrap_enter(sushiv_panel_t *p, GtkWidget *dummy);

static propmap *bgmap[]={
  &(propmap){"white",SUSHIV_BG_WHITE,   "[<i>b</i>]",NULL,wrap_bg},
  &(propmap){"black",SUSHIV_BG_BLACK,   "[<i>b</i>]",NULL,wrap_bg},
  &(propmap){"checks",SUSHIV_BG_CHECKS, "[<i>b</i>]",NULL,wrap_bg},
  NULL
};

static propmap *gridmap[]={
  &(propmap){"light",PLOT_GRID_LIGHT,   "[<i>g</i>]",NULL,wrap_grid},
  &(propmap){"normal",PLOT_GRID_NORMAL, "[<i>g</i>]",NULL,wrap_grid},
  &(propmap){"dark",PLOT_GRID_DARK,     "[<i>g</i>]",NULL,wrap_grid},
  &(propmap){"tics",PLOT_GRID_TICS,     "[<i>g</i>]",NULL,wrap_grid},
  &(propmap){"none",PLOT_GRID_NONE,     "[<i>g</i>]",NULL,wrap_grid},
  NULL
};

static propmap *textmap[]={
  &(propmap){"dark",PLOT_TEXT_DARK,   "[<i>t</i>]",NULL,wrap_text},
  &(propmap){"light",PLOT_TEXT_LIGHT, "[<i>t</i>]",NULL,wrap_text},
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
static propmap *resmap[]={
  &(propmap){"default",RES_DEF,  "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"1:32",RES_1_32,     "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"1:16",RES_1_16,     "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"1:8",RES_1_8,      "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"1:4",RES_1_4,      "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"1:2",RES_1_2,      "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"1",RES_1_1,        "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"2:1",RES_2_1,      "[<i>m</i>]",NULL,wrap_res},
  &(propmap){"4:1",RES_4_1,      "[<i>m</i>]",NULL,wrap_res},
  NULL,
};

static propmap *crossmap[]={
  &(propmap){"no",0     ,NULL,NULL,NULL},
  &(propmap){"yes",1    ,NULL,NULL,NULL},
  NULL
};

static propmap *legendmap[]={
  &(propmap){"none",PLOT_LEGEND_NONE,       NULL,NULL,NULL},
  &(propmap){"shadowed",PLOT_LEGEND_SHADOW, NULL,NULL,NULL},
  &(propmap){"boxed",PLOT_LEGEND_BOX,       NULL,NULL,NULL},
  NULL
};

static propmap *menu[]={
  &(propmap){"Open",0,"[<i>o</i>]",NULL,do_load},
  &(propmap){"Save",1,"[<i>s</i>]",NULL,do_save},
  &(propmap){"Print/Export",2,"[<i>p</i>]",NULL,_sushiv_panel_print},

  &(propmap){"",3,NULL,NULL,NULL},

  &(propmap){"Undo",4,"[<i>bksp</i>]",NULL,&wrap_undo_down},
  &(propmap){"Redo",5,"[<i>space</i>]",NULL,&wrap_undo_up},
  &(propmap){"Start zoom box",6,"[<i>enter</i>]",NULL,&wrap_enter},
  &(propmap){"Clear selection",7,"[<i>escape</i>]",NULL,&wrap_escape},
  &(propmap){"Toggle Legend",8,"[<i>l</i>]",NULL,&wrap_legend},

  &(propmap){"",9,NULL,NULL,NULL},

  &(propmap){"Background",10,"...",bgmap,NULL},
  &(propmap){"Text color",11,"...",textmap,NULL},
  &(propmap){"Grid mode",12,"...",gridmap,NULL},
  &(propmap){"Sampling",13,"...",resmap,NULL},

  &(propmap){"",14,NULL,NULL,NULL},

  &(propmap){"Quit",15,"[<i>q</i>]",NULL,&wrap_exit},

  NULL
};

static void decide_text_inv(sushiv_panel_t *p){
  if(p->private->graph){
    Plot *plot = PLOT(p->private->graph);
    if(p->private->bg_type == SUSHIV_BG_WHITE)
      plot_set_bg_invert(plot,PLOT_TEXT_DARK);
    else
      plot_set_bg_invert(plot,PLOT_TEXT_LIGHT);
  }
}

static void recompute_if_running(sushiv_panel_t *p){
  if(p->private->realized && p->private->graph)
    _sushiv_panel_recompute(p);
}

static void redraw_if_running(sushiv_panel_t *p){
  if(p->private->realized && p->private->graph){
    plot_draw_scales(PLOT(p->private->graph));
    _sushiv_panel_dirty_map(p);
    _sushiv_panel_dirty_legend(p);
  }
}

static void refg_if_running(sushiv_panel_t *p){
  if(p->private->realized && p->private->graph){
    plot_draw_scales(PLOT(p->private->graph));
    _sushiv_panel_dirty_legend(p);
  }
}

static void wrap_exit(sushiv_panel_t *dummy, GtkWidget *dummyw){
  _sushiv_clean_exit(SIGINT);
}

// precipitated actions perform undo push
static void wrap_enter(sushiv_panel_t *p, GtkWidget *dummy){
  plot_do_enter(PLOT(p->private->graph));
}

static void wrap_escape(sushiv_panel_t *p, GtkWidget *dummy){
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  plot_set_crossactive(PLOT(p->private->graph),0);
  _sushiv_panel_dirty_legend(p);

  _sushiv_undo_resume(p->sushi);
}

static void wrap_legend(sushiv_panel_t *p, GtkWidget *dummy){
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  plot_toggle_legend(PLOT(p->private->graph));
  _sushiv_panel_dirty_legend(p);

  _sushiv_undo_resume(p->sushi);
}

static void set_grid(sushiv_panel_t *p, int mode){
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  plot_set_grid(PLOT(p->private->graph),mode);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);

  _sushiv_undo_resume(p->sushi);
}

static void wrap_grid(sushiv_panel_t *p, GtkWidget *w){
  int pos = gtk_menu_item_position(w);
  set_grid(p, gridmap[pos]->value);
}

static int set_background(sushiv_panel_t *p,
			  enum sushiv_background bg){
  
  sushiv_panel_internal_t *pi = p->private;
  
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  pi->bg_type = bg;
  
  decide_text_inv(p);
  set_grid(p,PLOT_GRID_NORMAL);
  redraw_if_running(p);
  _sushiv_panel_update_menus(p);

  _sushiv_undo_resume(p->sushi);
  return 0;
}

static void wrap_bg(sushiv_panel_t *p, GtkWidget *w){
  int pos = gtk_menu_item_position(w);
  set_background(p, bgmap[pos]->value);
}

static void cycle_bg(sushiv_panel_t *p){
  int menupos = propmap_pos(bgmap, p->private->bg_type) + 1;
  if(bgmap[menupos] == NULL) menupos = 0;
  set_background(p, bgmap[menupos]->value);
}

static void cycleB_bg(sushiv_panel_t *p){
  int menupos = propmap_pos(bgmap, p->private->bg_type) - 1;
  if(menupos<0) menupos = propmap_last(bgmap);
  set_background(p, bgmap[menupos]->value);
}

static void set_text(sushiv_panel_t *p, int mode){
  _sushiv_undo_push(p->sushi);
  _sushiv_undo_suspend(p->sushi);

  plot_set_bg_invert(PLOT(p->private->graph),mode);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);

  _sushiv_undo_resume(p->sushi);
}

static void wrap_text(sushiv_panel_t *p, GtkWidget *w){
  int pos = gtk_menu_item_position(w);
  set_text(p, textmap[pos]->value);
}

static void cycle_text(sushiv_panel_t *p){
  int menupos = propmap_pos(textmap, PLOT(p->private->graph)->bg_inv) + 1;
  if(textmap[menupos] == NULL) menupos = 0;
  set_text(p, textmap[menupos]->value);
}

static void cycle_grid(sushiv_panel_t *p){
  int menupos = propmap_pos(gridmap, PLOT(p->private->graph)->grid_mode) + 1;
  if(gridmap[menupos] == NULL) menupos = 0;
  set_grid(p, gridmap[menupos]->value);
}
static void cycleB_grid(sushiv_panel_t *p){
  int menupos = propmap_pos(gridmap, PLOT(p->private->graph)->grid_mode) - 1;
  if(menupos<0) menupos = propmap_last(gridmap);
  set_grid(p, gridmap[menupos]->value);
}

static void res_set(sushiv_panel_t *p, int n, int d){
  if(n != p->private->oversample_n ||
     d != p->private->oversample_d){

    _sushiv_undo_push(p->sushi);
    _sushiv_undo_suspend(p->sushi);
    
    p->private->oversample_n = n;
    p->private->oversample_d = d;
    _sushiv_panel_update_menus(p);
    recompute_if_running(p);

    _sushiv_undo_resume(p->sushi);
  }
}

// a little different; the menu value is not the internal setting
static void res_set_pos(sushiv_panel_t *p, int pos){
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

static void wrap_res(sushiv_panel_t *p, GtkWidget *w){
  int pos = gtk_menu_item_position(w);
  res_set_pos(p, resmap[pos]->value);
}

static void cycle_res(sushiv_panel_t *p){
  int menupos = propmap_pos(resmap, p->private->menu_cursamp) + 1;
  if(resmap[menupos] == NULL) menupos = 0;
  res_set_pos(p, resmap[menupos]->value);
}

static void cycleB_res(sushiv_panel_t *p){
  int menupos = propmap_pos(resmap, p->private->menu_cursamp) - 1;
  if(menupos<0) menupos = propmap_last(resmap);
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
  sushiv_panel_t *p = (sushiv_panel_t *)user_data;

  c = gtk_print_context_get_cairo_context (context);
  w = gtk_print_context_get_width (context);
  h = gtk_print_context_get_height (context);
  
  p->private->print_action(p,c,w,h);
}

static void _sushiv_panel_print(sushiv_panel_t *p, GtkWidget *dummy){
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

static void wrap_undo_down(sushiv_panel_t *p, GtkWidget *dummy){
  _sushiv_undo_down(p->sushi);
}
static void wrap_undo_up(sushiv_panel_t *p, GtkWidget *dummy){
  _sushiv_undo_up(p->sushi);
}

static void do_save(sushiv_panel_t *p, GtkWidget *dummy){
  GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save",
						   NULL,
						   GTK_FILE_CHOOSER_ACTION_SAVE,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						   NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog), cwdname, NULL);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), filebase);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    if(filebase)free(filebase);
    if(filename)free(filename);
    if(dirname)free(dirname);

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    dirname = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    filebase = g_path_get_basename(filename);
    save_main();
  }

  gtk_widget_destroy (dialog);

}

static void do_load(sushiv_panel_t *p, GtkWidget *dummy){
  GtkWidget *dialog = gtk_file_chooser_dialog_new ("Open",
						   NULL,
						   GTK_FILE_CHOOSER_ACTION_OPEN,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						   NULL);

  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog), cwdname, NULL);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
    char *temp_filebase = filebase;
    char *temp_filename = filename;
    char *temp_dirname = dirname;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    dirname = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
    filebase = g_path_get_basename(filename);

    if(load_main()){

      free(filebase);
      free(filename);
      free(dirname);

      filebase = temp_filebase;
      filename = temp_filename;
      dirname = temp_dirname;

    }else{
      free(temp_filebase);
      free(temp_filename);
      free(temp_dirname);
    }
  }

  gtk_widget_destroy (dialog);

}

void _sushiv_panel_update_menus(sushiv_panel_t *p){

  // is undo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_level){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),4),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),4),TRUE);
  }

  // is redo active?
  if(!p->sushi->private->undo_stack ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level] ||
     !p->sushi->private->undo_stack[p->sushi->private->undo_level+1]){
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),5),FALSE);
  }else{
    gtk_widget_set_sensitive(gtk_menu_get_item(GTK_MENU(p->private->popmenu),5),TRUE);
  }

  // are we starting or enacting a zoom box?
  if(p->private->oldbox_active){ 
    gtk_menu_alter_item_label(GTK_MENU(p->private->popmenu),6,"Zoom to box");
  }else{
    gtk_menu_alter_item_label(GTK_MENU(p->private->popmenu),6,"Start zoom box");
  }

  // make sure menu reflects plot configuration
  gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			    propmap_label_pos(menu,"Background"),
			    bgmap[propmap_pos(bgmap,p->private->bg_type)]->left);

  gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			    propmap_label_pos(menu,"Text color"),
			    textmap[propmap_pos(textmap,PLOT(p->private->graph)->bg_inv)]->left);

  gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			    propmap_label_pos(menu,"Grid mode"),
			    gridmap[propmap_pos(gridmap,PLOT(p->private->graph)->grid_mode)]->left);
   {
    char buffer[80];
    snprintf(buffer,60,"%d:%d",p->private->oversample_n,p->private->oversample_d);
    if(p->private->def_oversample_n == p->private->oversample_n &&
       p->private->def_oversample_d == p->private->oversample_d)
      strcat(buffer," (default)");
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),
			      propmap_label_pos(menu,"Sampling"),buffer);
  }
}

static gboolean panel_keypress(GtkWidget *widget,
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
    do_save(p,NULL);
    return TRUE;
  case GDK_o:
    do_load(p,NULL);
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
    _sushiv_clean_exit(SIGINT);
    return TRUE;
    
  case GDK_BackSpace:
    // undo 
    _sushiv_undo_down(p->sushi);
    return TRUE;
    
  case GDK_r:
  case GDK_space:
    // redo/forward
    _sushiv_undo_up(p->sushi);
    return TRUE;

  case GDK_p:
    _sushiv_panel_print(p,NULL);
    return TRUE;

  case GDK_l:
    wrap_legend(p,NULL);
    return TRUE;
  } 

  return FALSE;
}

void _sushiv_realize_panel(sushiv_panel_t *p){
  if(p && !p->private->realized){
    p->private->realize(p);

    g_signal_connect (G_OBJECT (p->private->toplevel), "key-press-event",
		      G_CALLBACK (panel_keypress), p);
    gtk_window_set_title (GTK_WINDOW (p->private->toplevel), p->name);

    p->private->realized=1;

    // generic things that happen in all panel realizations...

    // text black or white in the plot?
    decide_text_inv(p);
    p->private->popmenu = gtk_menu_new_twocol(p->private->toplevel, menu, p);
    _sushiv_panel_update_menus(p);
    
  }
}

void set_map_throttle_time(sushiv_panel_t *p){
  struct timeval now;
  gettimeofday(&now,NULL);

  p->private->last_map_throttle = now.tv_sec*1000 + now.tv_usec/1000;
}

static int test_throttle_time(sushiv_panel_t *p){
  struct timeval now;
  long test;
  gettimeofday(&now,NULL);
  
  test = now.tv_sec*1000 + now.tv_usec/1000;
  if(p->private->last_map_throttle + 500 < test) 
    return 1;

  return 0;  
}

/* request a recomputation with full setup (eg, linking, scales,
   etc) */
void _sushiv_panel_recompute(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->request_compute(p);
  gdk_threads_leave ();
}

/* use these to request a render/compute action after everything has
   already been set up to perform the op; the above full recompute
   request will eventually trigger a call here to kick off the actual
   computation.  Do panel subtype-specific setup, then wake workers
   with one of the below */
void _sushiv_panel_dirty_plot(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->plot_active = 1;
  p->private->plot_serialno++;
  p->private->plot_progress_count=0;
  p->private->plot_complete_count=0;
  gdk_threads_leave ();
  _sushiv_wake_workers();
}

void _sushiv_panel_dirty_map(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->map_active = 1;
  p->private->map_serialno++;
  p->private->map_progress_count=0;
  p->private->map_complete_count=0;
  gdk_threads_leave ();
  _sushiv_wake_workers();
}

void _sushiv_panel_dirty_map_throttled(sushiv_panel_t *p){
  gdk_threads_enter ();
  if(!p->private->map_active && test_throttle_time(p)){
     _sushiv_panel_dirty_map(p);
  }
  gdk_threads_leave ();
}

void _sushiv_panel_dirty_legend(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->legend_active = 1;
  p->private->legend_serialno++;
  p->private->legend_progress_count=0;
  p->private->legend_complete_count=0;
  gdk_threads_leave ();
  _sushiv_wake_workers();
}

/* use these to signal a computation is completed */
void _sushiv_panel_clean_plot(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->plot_active = 0;
  gdk_threads_leave ();
}

void _sushiv_panel_clean_map(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->map_active = 0;
  gdk_threads_leave ();
}

void _sushiv_panel_clean_legend(sushiv_panel_t *p){
  gdk_threads_enter ();
  p->private->legend_active = 0;
  gdk_threads_leave ();
}

int sushiv_panel_oversample(sushiv_instance_t *s,
			    int number,
			    int numer,
			    int denom){
  
  if(number<0){
    fprintf(stderr,"sushiv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>s->panels || !s->panel_list[number]){
    fprintf(stderr,"sushiv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sushiv_panel_t *p = s->panel_list[number];
  sushiv_panel_internal_t *pi = p->private;

  if(denom == 0){
    fprintf(stderr,"sushiv_panel_oversample: A denominator of zero is invalid\n");
    return -EINVAL;
  }

  pi->def_oversample_n = pi->oversample_n = numer;
  pi->def_oversample_d = pi->oversample_d = denom;
  recompute_if_running(p);
  return 0;
}

int sushiv_panel_background(sushiv_instance_t *s,
			    int number,
			    enum sushiv_background bg){

  if(number<0){
    fprintf(stderr,"sushiv_panel_background: Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number>s->panels || !s->panel_list[number]){
    fprintf(stderr,"sushiv_panel_background: Panel number %d does not exist\n",number);
    return -EINVAL;
  }
  
  sushiv_panel_t *p = s->panel_list[number];
  return set_background(p,bg);
}

int _sushiv_new_panel(sushiv_instance_t *s,
		      int number,
		      const char *name, 
		      int *objectives,
		      int *dimensions,
		      unsigned flags){
  
  sushiv_panel_t *p;
  int i;

  if(number<0){
    fprintf(stderr,"Panel number must be >= 0\n");
    return -EINVAL;
  }

  if(number<s->panels){
    if(s->panel_list[number]!=NULL){
      fprintf(stderr,"Panel number %d already exists\n",number);
      return -EINVAL;
    }
  }else{
    if(s->panels == 0){
      s->panel_list = calloc (number+1,sizeof(*s->panel_list));
    }else{
      s->panel_list = realloc (s->panel_list,(number+1) * sizeof(*s->panel_list));
      memset(s->panel_list + s->panels, 0, sizeof(*s->panel_list)*(number+1 - s->panels));
    }
    s->panels = number+1; 
  }

  p = s->panel_list[number] = calloc(1, sizeof(**s->panel_list));

  p->number = number;
  p->name = strdup(name);
  p->flags = flags;
  p->sushi = s;
  p->private = calloc(1, sizeof(*p->private));
  p->private->spinner = spinner_new();
  p->private->def_oversample_n = p->private->oversample_n = 1;
  p->private->def_oversample_d = p->private->oversample_d = 1;

  i=0;
  while(objectives && objectives[i]>=0)i++;
  p->objectives = i;
  p->objective_list = malloc(i*sizeof(*p->objective_list));
  for(i=0;i<p->objectives;i++){
    if(objectives[i]<0 || objectives[i]>=s->objectives ||
       s->objective_list[objectives[i]] == NULL){
      fprintf(stderr,"Panel %d: Objective number %d does not exist\n",number, objectives[i]);
      return -EINVAL;
    }

    sushiv_objective_t *o = s->objective_list[objectives[i]];
    p->objective_list[i].o = o;
    p->objective_list[i].p = p;
  }

  i=0;
  while(dimensions && dimensions[i]>=0)i++;
  p->dimensions = i;
  p->dimension_list = malloc(i*sizeof(*p->dimension_list));
  for(i=0;i<p->dimensions;i++){
    if(dimensions[i]<0 || dimensions[i]>=s->dimensions ||
       s->dimension_list[dimensions[i]] == NULL){
      fprintf(stderr,"Panel %d: Objective number %d does not exist\n",number, objectives[i]);
      return -EINVAL;
    }

    sushiv_dimension_t *d = s->dimension_list[dimensions[i]];
    p->dimension_list[i].d = d;
    p->dimension_list[i].p = p;
  }

  return number;
}

void _sushiv_panel_undo_log(sushiv_panel_t *p, sushiv_panel_undo_t *u){
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

void _sushiv_panel_undo_restore(sushiv_panel_t *p, sushiv_panel_undo_t *u){
  // go in through setting routines
  plot_set_crossactive(PLOT(p->private->graph),u->cross_mode);
  plot_set_legendactive(PLOT(p->private->graph),u->legend_mode);
  set_background(p, u->bg_mode); // must be first; it can frob grid and test
  set_text(p, u->text_mode);
  set_grid(p, u->grid_mode);
  p->private->menu_cursamp = u->menu_cursamp;
  res_set(p, u->oversample_n, u->oversample_d);

  // panel-subtype-specific restore
  p->private->undo_restore(u,p);

  _sushiv_panel_dirty_legend(p); 
}

int _save_panel(sushiv_panel_t *p, xmlNodePtr instance){  
  if(!p) return 0;
  char buffer[80];
  int ret=0;

  xmlNodePtr pn = xmlNewChild(instance, NULL, (xmlChar *) "panel", NULL);
  xmlNodePtr n;

  xmlNewPropI(pn, "number", p->number);
  xmlNewPropS(pn, "name", p->name);

  // let the panel subtype handler fill in type
  // we're only saving settings independent of subtype

  // background
  n = xmlNewChild(pn, NULL, (xmlChar *) "background", NULL);
  xmlNewMapProp(n, "color", bgmap, p->private->bg_type);

  // grid
  n = xmlNewChild(pn, NULL, (xmlChar *) "grid", NULL);
  xmlNewMapProp(n, "mode", gridmap, PLOT(p->private->graph)->grid_mode);

  // crosshairs
  n = xmlNewChild(pn, NULL, (xmlChar *) "crosshairs", NULL);
  xmlNewMapProp(n, "active", crossmap, PLOT(p->private->graph)->cross_active);

  // legend
  n = xmlNewChild(pn, NULL, (xmlChar *) "legend", NULL);
  xmlNewMapProp(n,"mode", legendmap, PLOT(p->private->graph)->legend_active);

  // text
  n = xmlNewChild(pn, NULL, (xmlChar *) "text", NULL);
  xmlNewMapProp(n,"color", textmap, PLOT(p->private->graph)->bg_inv);

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

int _load_panel(sushiv_panel_t *p,
		sushiv_panel_undo_t *u,
		xmlNodePtr pn,
		int warn){

  // check name 
  xmlCheckPropS(pn,"name",p->name,"Panel %d name mismatch in save file.",p->number,&warn);

  // background
  xmlGetChildMap(pn, "background", "color", bgmap, &u->bg_mode,
		 "Panel %d unknown background setting", p->number, &warn);
  // grid
  xmlGetChildMap(pn, "grid", "mode", gridmap, &u->grid_mode,
		 "Panel %d unknown grid mode setting", p->number, &warn);
  // crosshairs
  xmlGetChildMap(pn, "crosshairs", "active", crossmap, &u->cross_mode,
		 "Panel %d unknown crosshair setting", p->number, &warn);
  // legend
  xmlGetChildMap(pn, "legend", "mode", legendmap, &u->legend_mode,
		 "Panel %d unknown legend setting", p->number, &warn);
  // text
  xmlGetChildMap(pn, "text", "color", textmap, &u->text_mode,
		 "Panel %d unknown text color setting", p->number, &warn);
  // resample
  char *prop = NULL;
  xmlGetChildPropS(pn, "sampling", "ratio", &prop);
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
      first_load_warning(&warn);
      fprintf(stderr,"Unknown option (%s) set for panel %d.\n",n->name,p->number);
    }
    n = n->next; 
  }

  return warn;
}
