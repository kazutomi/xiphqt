/*
 *    Playlist widget
 *    Copyright (c) 2005, 2006 Martin Grimme  <martin.grimme@lintegra.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "playlistwidget.h"


static void
click_cb(GtkTreeView *treeview,
	 GtkTreePath *path,
	 GtkTreeViewColumn *column,
	 PLWidget *plw) {

  gint *indices;


  indices = gtk_tree_path_get_indices(path);
  if (indices) {
    playlist_jump_to(plw->playlist, indices[0]);
  }
  
}


static void
change_cb(PLWidget *plw, gboolean renew) {

  GPtrArray *list = plw->playlist->list;
  int position = plw->playlist->position;
  GtkTreeIter iter;
  GtkTreePath *treepath;
  char *item, *tmpitem;
  int i;

  if (renew) {
    gtk_list_store_clear(plw->liststore);

    /* did I mention that the API sucks..? ;) */
    for (i = 0; i < list->len; i++) {
      
      item = g_path_get_basename(g_ptr_array_index(list, i));
      gtk_list_store_append(plw->liststore, &iter);
      gtk_list_store_set(plw->liststore, &iter, 1, item, -1);
      g_free(item);
      
    }
  }

  item = g_path_get_basename(g_ptr_array_index(list,
					       plw->current_selection));
  treepath = gtk_tree_path_new_from_indices(plw->current_selection, -1);
  gtk_tree_model_get_iter(plw->liststore, &iter, treepath);
  gtk_list_store_set(plw->liststore, &iter, 1, item, -1);
  gtk_tree_path_free(treepath);
  g_free(item);

  
  tmpitem = g_path_get_basename(g_ptr_array_index(list, position));
  item = g_strconcat("<b>", tmpitem, "</b>", NULL);
  g_free(tmpitem);
  treepath = gtk_tree_path_new_from_indices(position, -1);
  gtk_tree_model_get_iter(plw->liststore, &iter, treepath);
  gtk_list_store_set(plw->liststore, &iter, 1, item, -1);
  gtk_tree_path_free(treepath);
  g_free(item);
  plw->current_selection = position;

}



PLWidget *
plwidget_new() {

  /* the API for list widgets in GTK totally sucks... */
  GtkTreeViewColumn *col1;
  GtkTreeViewColumn *col2;
  GtkCellRenderer *crender1;
  GtkCellRenderer *crender2;
  GtkTreeIter iter;
  GValue fontsize = {0,};

  PLWidget *plw = g_new(PLWidget, 1);

  plw->liststore = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);
  plw->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(plw->liststore));
  plw->current_selection = 0;

  /* column 1: image */
  crender1 = gtk_cell_renderer_pixbuf_new();
  col1 = gtk_tree_view_column_new_with_attributes("", crender1, "pixbuf", 0,
						  NULL);
  gtk_tree_view_append_column(plw->treeview, col1);

  /* column 2: song name */
  crender2 = gtk_cell_renderer_text_new();
  g_value_init(&fontsize, G_TYPE_DOUBLE);
  g_value_set_double(&fontsize, 14.0);
  g_object_set_property(G_OBJECT(crender2), "size-points", &fontsize);
  col2 = gtk_tree_view_column_new_with_attributes("", crender2, "markup", 1, 
						  NULL);
  gtk_tree_view_append_column(plw->treeview, col2);

  g_signal_connect(G_OBJECT(plw->treeview), "row-activated",
		   G_CALLBACK(click_cb), plw);

  return plw;

}



GtkWidget *
plwidget_get_widget(PLWidget *plw) {

  return plw->treeview;

}


void
plwidget_set_playlist(PLWidget *plw,
		      Playlist *pl) {

  plw->playlist = pl;
  playlist_set_change_cb(pl, change_cb, (void *) plw);

}
