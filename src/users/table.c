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

#include "gst.h"

#include "table.h"
#include "users-table.h"
#include "groups-table.h"
#include "profiles-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

extern GstTool *tool;

extern GtkWidget *users_table;
extern GtkWidget *groups_table;

void
create_gtk_tree_list (GtkWidget *list)
{
	GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER));
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
	
	renderer = gtk_cell_renderer_text_new ();
	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (list),
                                                     -1,
                                                     "group",
                                                     renderer,
                                                     "text",
                                                     0,
                                                     NULL);
	g_object_unref (model);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
}

void
clear_gtk_tree_list (GtkTreeView *list)
{
	GtkTreeModel *model;
	
	g_return_if_fail (list != NULL);
	g_return_if_fail (GTK_IS_TREE_VIEW (list));
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

void
populate_gtk_tree_list (GtkTreeView *list, GList *items)
{
	gchar *entry;
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail (list != NULL);
	g_return_if_fail (GTK_IS_TREE_VIEW (list));
	
	model = gtk_tree_view_get_model (list);

	while (items)
	{
		entry = items->data;

		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model),
		                    &iter,
				    0, entry,
		                    1, items,
				    -1);
		items = items->next;
	}
}

void
create_tables (void)
{
	create_users_table ();
	create_groups_table ();
	create_profiles_table ();
}

void 
update_tables_complexity (GstDialogComplexity complexity) 
{
	update_users_table_complexity (complexity);
}

void
populate_all_tables (void)
{
	populate_users_table ();
	populate_groups_table ();
	populate_profiles_table ();
}

void 
tables_update_content (void)
{
	users_table_update_content ();
	groups_table_update_content ();
}

xmlNodePtr get_selected_row_node (gint tbl)
{
	xmlNodePtr node;
	GtkTreeView *table;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint column;
	
	switch (tbl) {
		case NODE_USER:
			table = GTK_TREE_VIEW (users_table);
			column = COL_USER_POINTER;
			break;
		case NODE_GROUP:
			table = GTK_TREE_VIEW (groups_table);
			column = COL_GROUP_POINTER;
			break;
		default:
			return NULL;
	}
	
        model = gtk_tree_view_get_model (table);
        
        gtk_tree_view_get_cursor (table, &path, NULL);
        gtk_tree_model_get_iter (model, &iter, path);

        gtk_tree_model_get (model, &iter, column, &node, -1);
	
	return node;
}
