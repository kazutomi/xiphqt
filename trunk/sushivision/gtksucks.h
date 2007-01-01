/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
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

#ifndef _GTK_SUCKS_H_
#define _GTK_SUCKS_H_

extern void gtk_widget_set_sensitive_fixup(GtkWidget *w, gboolean state);
extern void gtk_widget_remove_events (GtkWidget *widget, gint events);
extern void gtk_button3_fixup();
extern void gtk_mutex_fixup();
extern pthread_mutex_t *gtk_get_mutex();
extern GtkWidget *gtk_menu_new_twocol(GtkWidget *bind, 
				      char **menu_list, 
				      char **shortcuts,
				      void (*callbacks[])(void *),
				      void *callback_data);
extern GtkWidget *gtk_menu_get_item(GtkMenu *m, int pos);
extern void gtk_menu_alter_item_label(GtkMenu *m, int pos, char *text);
extern GtkWidget * gtk_combo_box_new_markup (void);

#endif
