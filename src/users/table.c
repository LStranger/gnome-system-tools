/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <gtk/gtk.h>

#include "xst.h"

#include "table.h"
#include "users-table.h"
#include "groups-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

extern XstTool *tool;

extern GtkWidget *users_table;
extern GtkWidget *groups_table;

GtkWidget*
create_gtktree_list (GtkWidget *sw)
{
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_store_new (1, G_TYPE_STRING));
	GtkWidget *list;
	GtkCellRenderer *renderer;
	
	list = gtk_tree_view_new_with_model (model);
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (list), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);
	
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list),
                                                     -1,
                                                     "group",
                                                     renderer,
                                                     "text",
                                                     0,
                                                     NULL);
	g_object_unref (model);
	
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	
	gtk_widget_show_all (list);
	gtk_container_add (GTK_CONTAINER (sw), list);
	
	return list;
}

void
construct_tables (void)
{
	construct_users_table ();
	construct_groups_table ();
}

void 
update_tables_complexity (XstDialogComplexity complexity) 
{
	update_users_table_complexity (complexity);
}

void
populate_all_tables (void)
{
	populate_users_table ();
	populate_groups_table ();
}

void 
tables_update_content (void)
{
	users_table_update_content ();
	groups_table_update_content ();
}

xmlNodePtr get_selected_row_node (gint tbl)
{
	GtkTreeView *table;
	xmlNodePtr root;
	
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *name;
	
	switch (tbl) {
		case TABLE_USER:
			table = GTK_TREE_VIEW (users_table);
			root = get_user_root_node ();
			break;
		case TABLE_GROUP:
			table = GTK_TREE_VIEW (groups_table);
			root = get_group_root_node ();
			break;
		default:
			return NULL;
	}
	
	model = gtk_tree_view_get_model (table);
	
	gtk_tree_view_get_cursor (table, &path, NULL);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter,
	                    0, &name,
	                    -1);
	
	return find_node_with_name (root, name);
}

void
delete_selected_row (gint tbl)
{
	GtkTreeView *table;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	
	switch (tbl) {
		case TABLE_USER:
			table = GTK_TREE_VIEW (users_table);
			break;
		case TABLE_GROUP:
			table = GTK_TREE_VIEW (groups_table);
			break;
	}
	
	model = gtk_tree_view_get_model (table);
	gtk_tree_view_get_cursor (table, &path, NULL);
	
	if (!path)
		return;
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
}

