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

static void decide_text_inv(sushiv_panel_t *p){
  if(p->private->graph){
    Plot *plot = PLOT(p->private->graph);
    if(p->private->bg_type == SUSHIV_BG_WHITE)
      plot_set_bg_invert(plot,0);
    else
      plot_set_bg_invert(plot,1);
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

static void wrap_exit(sushiv_panel_t *dummy){
  _sushiv_clean_exit(SIGINT);
}

static void wrap_enter(sushiv_panel_t *p){
  plot_do_enter(PLOT(p->private->graph));
}

static void wrap_escape(sushiv_panel_t *p){
  plot_do_escape(PLOT(p->private->graph));
  _sushiv_panel_dirty_legend(p);
}

static void wrap_legend(sushiv_panel_t *p){
  plot_toggle_legend(PLOT(p->private->graph));
  _sushiv_panel_dirty_legend(p);
}

static void light_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),PLOT_GRID_LIGHT);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void mid_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),PLOT_GRID_NORMAL);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void dark_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),PLOT_GRID_DARK);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void tic_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),PLOT_GRID_TICS);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void no_scale(sushiv_panel_t *p){
  plot_set_grid(PLOT(p->private->graph),0);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}

static int _sushiv_panel_background_i(sushiv_panel_t *p,
				      enum sushiv_background bg){
  
  sushiv_panel_internal_t *pi = p->private;
  
  pi->bg_type = bg;
  
  decide_text_inv(p);
  mid_scale(p);
  redraw_if_running(p);
  _sushiv_panel_update_menus(p);
  return 0;
}

static void white_bg(sushiv_panel_t *p){
  _sushiv_panel_background_i(p,SUSHIV_BG_WHITE);
}
static void black_bg(sushiv_panel_t *p){
  _sushiv_panel_background_i(p,SUSHIV_BG_BLACK);
}
static void checked_bg(sushiv_panel_t *p){
  _sushiv_panel_background_i(p,SUSHIV_BG_CHECKS);
}
static void cycle_bg(sushiv_panel_t *p){
  switch(p->private->bg_type){
  case 0:
    black_bg(p);
    break;
  case 1:
    checked_bg(p);
    break;
  default:
    white_bg(p);
    break;
  }
}
static void cycleB_bg(sushiv_panel_t *p){
  switch(p->private->bg_type){
  case 0:
    checked_bg(p);
    break;
  case 1:
    white_bg(p);
    break;
  default:
    black_bg(p);
    break;
  }
}

static void black_text(sushiv_panel_t *p){
  plot_set_bg_invert(PLOT(p->private->graph),0);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void white_text(sushiv_panel_t *p){
  plot_set_bg_invert(PLOT(p->private->graph),1);
  _sushiv_panel_update_menus(p);
  refg_if_running(p);
}
static void cycle_text(sushiv_panel_t *p){
  switch(PLOT(p->private->graph)->bg_inv){
  case 0:
    white_text(p);
    break;
  default:
    black_text(p);
    break;
  }
}

static void cycle_grid(sushiv_panel_t *p){
  switch(PLOT(p->private->graph)->grid_mode){
  case PLOT_GRID_LIGHT:
    mid_scale(p);
    break;
  case PLOT_GRID_NORMAL:
    dark_scale(p);
    break;
  case PLOT_GRID_DARK:
    tic_scale(p);
    break;
  case PLOT_GRID_TICS:
    no_scale(p);
    break;
  default:
    light_scale(p);
    break;
  }
}
static void cycleB_grid(sushiv_panel_t *p){
  switch(PLOT(p->private->graph)->grid_mode){
  case PLOT_GRID_LIGHT:
    no_scale(p);
    break;
  case PLOT_GRID_NORMAL:
    light_scale(p);
    break;
  case PLOT_GRID_DARK:
    mid_scale(p);
    break;
  case PLOT_GRID_TICS:
    dark_scale(p);
    break;
  default:
    tic_scale(p);
    break;
  }
}

static void res_set(sushiv_panel_t *p, int n, int d){
  if(n != p->private->oversample_n ||
     d != p->private->oversample_d){
    p->private->oversample_n = n;
    p->private->oversample_d = d;
    _sushiv_panel_update_menus(p);
    recompute_if_running(p);
  }
}

static void res_def(sushiv_panel_t *p){
  p->private->menu_cursamp=0;
  res_set(p,p->private->def_oversample_n,p->private->def_oversample_d);
}
static void res_1_32(sushiv_panel_t *p){
  p->private->menu_cursamp=1;
  res_set(p,1,32);
}
static void res_1_16(sushiv_panel_t *p){
  p->private->menu_cursamp=2;
  res_set(p,1,16);
}
static void res_1_8(sushiv_panel_t *p){
  p->private->menu_cursamp=3;
  res_set(p,1,8);
}
static void res_1_4(sushiv_panel_t *p){
  p->private->menu_cursamp=4;
  res_set(p,1,4);
}
static void res_1_2(sushiv_panel_t *p){
  p->private->menu_cursamp=5;
  res_set(p,1,2);
}
static void res_1_1(sushiv_panel_t *p){
  p->private->menu_cursamp=6;
  res_set(p,1,1);
}
static void res_2_1(sushiv_panel_t *p){
  p->private->menu_cursamp=7;
  res_set(p,2,1);
}
static void res_4_1(sushiv_panel_t *p){
  p->private->menu_cursamp=8;
  res_set(p,4,1);
}

static void cycle_res(sushiv_panel_t *p){
  switch(p->private->menu_cursamp){
  case 0:
    res_1_32(p);
    break;
  case 1:
    res_1_16(p);
    break;
  case 2:
    res_1_8(p);
    break;
  case 3:
    res_1_4(p);
    break;
  case 4:
    res_1_2(p);
    break;
  case 5:
    res_1_1(p);
    break;
  case 6:
    res_2_1(p);
    break;
  case 7:
    res_4_1(p);
    break;
  case 8:
    res_def(p);
    break;
  }
}

static void cycleB_res(sushiv_panel_t *p){
  switch(p->private->menu_cursamp){
  case 2:
    res_1_32(p);
    break;
  case 3:
    res_1_16(p);
    break;
  case 4:
    res_1_8(p);
    break;
  case 5:
    res_1_4(p);
    break;
  case 6:
    res_1_2(p);
    break;
  case 7:
    res_1_1(p);
    break;
  case 8:
    res_2_1(p);
    break;
  case 0:
    res_4_1(p);
    break;
  case 1:
    res_def(p);
    break;
  }
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

static void _sushiv_panel_print(sushiv_panel_t *p){
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

static menuitem *menu[]={
  &(menuitem){"Open","[<i>o</i>]",NULL,NULL},
  &(menuitem){"Save","[<i>s</i>]",NULL,NULL},
  &(menuitem){"Print/Export","[<i>p</i>]",NULL,&_sushiv_panel_print},

  &(menuitem){"",NULL,NULL,NULL},

  &(menuitem){"Undo","[<i>bksp</i>]",NULL,&_sushiv_panel_undo_down},
  &(menuitem){"Redo","[<i>space</i>]",NULL,&_sushiv_panel_undo_up},
  &(menuitem){"Start zoom box","[<i>enter</i>]",NULL,&wrap_enter},
  &(menuitem){"Clear selection","[<i>escape</i>]",NULL,&wrap_escape},
  &(menuitem){"Toggle Legend","[<i>l</i>]",NULL,&wrap_legend},

  &(menuitem){"",NULL,NULL,NULL},

  &(menuitem){"Background","...",NULL,NULL},
  &(menuitem){"Text color","...",NULL,NULL},
  &(menuitem){"Grid mode","...",NULL,NULL},
  &(menuitem){"Sampling","...",NULL,NULL},

  &(menuitem){"",NULL,NULL,NULL},

  &(menuitem){"Quit","[<i>q</i>]",NULL,&wrap_exit},

  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_bg[]={
  &(menuitem){"white","[<i>b</i>]",NULL,&white_bg},
  &(menuitem){"black","[<i>b</i>]",NULL,&black_bg},
  &(menuitem){"checks","[<i>b</i>]",NULL,&checked_bg},
  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_text[]={
  &(menuitem){"dark","[<i>t</i>]",NULL,&black_text},
  &(menuitem){"light","[<i>t</i>]",NULL,&white_text},
  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_scales[]={
  &(menuitem){"light","[<i>g</i>]",NULL,light_scale},
  &(menuitem){"mid","[<i>g</i>]",NULL,mid_scale},
  &(menuitem){"dark","[<i>g</i>]",NULL,dark_scale},
  &(menuitem){"tics","[<i>g</i>]",NULL,tic_scale},
  &(menuitem){"none","[<i>g</i>]",NULL,no_scale},
  &(menuitem){NULL,NULL,NULL,NULL}
};

static menuitem *menu_res[]={
  &(menuitem){"default","[<i>m</i>]",NULL,res_def},
  &(menuitem){"1:32","[<i>m</i>]",NULL,res_1_32},
  &(menuitem){"1:16","[<i>m</i>]",NULL,res_1_16},
  &(menuitem){"1:8","[<i>m</i>]",NULL,res_1_8},
  &(menuitem){"1:4","[<i>m</i>]",NULL,res_1_4},
  &(menuitem){"1:2","[<i>m</i>]",NULL,res_1_2},
  &(menuitem){"1","[<i>m</i>]",NULL,res_1_1},
  &(menuitem){"2:1","[<i>m</i>]",NULL,res_2_1},
  &(menuitem){"4:1","[<i>m</i>]",NULL,res_4_1},
  &(menuitem){NULL,NULL,NULL,NULL}
};

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
  switch(p->private->bg_type){ 
  case SUSHIV_BG_WHITE:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),10,menu_bg[0]->left);
    break;
  case SUSHIV_BG_BLACK:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),10,menu_bg[1]->left);
    break;
  default:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),10,menu_bg[2]->left);
    break;
  }

  switch(PLOT(p->private->graph)->bg_inv){ 
  case 0:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),11,menu_text[0]->left);
    break;
  default:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),11,menu_text[1]->left);
    break;
  }

  switch(PLOT(p->private->graph)->grid_mode){ 
  case PLOT_GRID_LIGHT:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),12,menu_scales[0]->left);
    break;
  case PLOT_GRID_NORMAL:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),12,menu_scales[1]->left);
    break;
  case PLOT_GRID_DARK:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),12,menu_scales[2]->left);
    break;
  case PLOT_GRID_TICS:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),12,menu_scales[3]->left);
    break;
  default:
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),12,menu_scales[4]->left);
    break;
  }

  {
    char buffer[80];
    snprintf(buffer,60,"%d:%d",p->private->oversample_n,p->private->oversample_d);
    if(p->private->def_oversample_n == p->private->oversample_n &&
       p->private->def_oversample_d == p->private->oversample_d)
      strcat(buffer," (default)");
    gtk_menu_alter_item_right(GTK_MENU(p->private->popmenu),13,buffer);
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
    
   case GDK_Escape:
     wrap_escape(p);
    return TRUE;

  case GDK_Return:case GDK_ISO_Enter:
    wrap_enter(p);
    return TRUE;
   
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

  case GDK_p:
    _sushiv_panel_print(p);
    return TRUE;

  case GDK_l:
    wrap_legend(p);
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

    // panel right-click menus
    GtkWidget *bgmenu = gtk_menu_new_twocol(NULL,menu_bg,p);
    GtkWidget *textmenu = gtk_menu_new_twocol(NULL,menu_text,p);
    GtkWidget *scalemenu = gtk_menu_new_twocol(NULL,menu_scales,p);
    GtkWidget *resmenu = gtk_menu_new_twocol(NULL,menu_res,p);

    // not thread safe, we're not threading yet
    menu[10]->submenu = bgmenu;
    menu[11]->submenu = textmenu;
    menu[12]->submenu = scalemenu;
    menu[13]->submenu = resmenu;

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
  return _sushiv_panel_background_i(p,bg);
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
    sushiv_objective_t *o = s->objective_list[objectives[i]];
    p->objective_list[i].o = o;
    p->objective_list[i].p = p;
  }

  i=0;
  while(dimensions && dimensions[i]>=0)i++;
  p->dimensions = i;
  p->dimension_list = malloc(i*sizeof(*p->dimension_list));
  for(i=0;i<p->dimensions;i++){
    sushiv_dimension_t *d = s->dimension_list[dimensions[i]];
    p->dimension_list[i].d = d;
    p->dimension_list[i].p = p;
  }

  return number;
}

