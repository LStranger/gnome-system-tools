/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* groups-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gst.h"

#include "table.h"
#include "groups-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

extern GstTool *tool;

GtkWidget *groups_table;

GroupsTableConfig groups_table_config [] = {
	{ N_("Group"),	TRUE,	TRUE},
	{ N_("GID"),	TRUE,	FALSE},
	{NULL}
};

static void
add_group_columns (GtkTreeView *treeview)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GroupsTableConfig *i;
	guint j;
	
	for (i = groups_table_config, j = 0;
	     i->name != NULL;
	     i++, j++)
	{
		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes (_(i->name),
		                                                   renderer,
		                                                   "text",
		                                                   j,
		                                                   NULL);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_sort_column_id (column, j);

		gtk_tree_view_insert_column (GTK_TREE_VIEW (groups_table), column, j);
	}
}

static GtkTreeModel*
create_groups_model (void)
{
	GtkTreeStore *model;
	
	model = gtk_tree_store_new (COL_GROUP_LAST,
	                            G_TYPE_STRING,
	                            G_TYPE_INT,
	                            G_TYPE_POINTER);
	return GTK_TREE_MODEL (model);
}

void
create_groups_table (void)
{
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkWidget        *popup;
	
	model = create_groups_model ();
	
	groups_table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	gtk_tree_view_set_model (GTK_TREE_VIEW (groups_table), model);
	
	g_object_unref (model);

	add_group_columns (GTK_TREE_VIEW (groups_table));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (groups_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	popup = popup_menu_create (groups_table);
	
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_table_clicked),
			  (gpointer) groups_table);
	g_signal_connect (G_OBJECT (groups_table), "button_press_event",
			  G_CALLBACK (on_table_button_press), popup);
	g_signal_connect (G_OBJECT (groups_table), "popup-menu",
			  G_CALLBACK (on_table_popup_menu), popup);
}

static char*
groups_table_value (xmlNodePtr node, gchar *key)
{
        g_return_val_if_fail (node != NULL, NULL);
        return gst_xml_get_child_content (node, key);
}

static GroupTreeItem*
get_group_node_data (xmlNodePtr group)
{
        GroupTreeItem *item = g_malloc (sizeof(GroupTreeItem));
	
	item->group = groups_table_value (group, "name");
        item->GID = atoi (groups_table_value (group, "gid"));
        
        return item;
}

void
populate_groups_table (void)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	xmlNodePtr root = get_root_node (NODE_GROUP);
	GtkTreeIter iter;
	xmlNodePtr group;
	
	g_return_if_fail (model != NULL);
	g_return_if_fail (root != NULL);

	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	for (group = gst_xml_element_find_first (root, "group"); group != NULL; group = gst_xml_element_find_next (group, "group"))
	{
		if (check_node_visibility (group))
		{
			GroupTreeItem *item;
			item = get_group_node_data (group);
		
			gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
			gtk_tree_store_set (GTK_TREE_STORE (model),
			                    &iter,
					    COL_GROUP_NAME, item->group,
					    COL_GROUP_GID, item->GID,
			                    COL_GROUP_POINTER, group,
					    -1);
		}
	}
}

static void
clear_groups_table (void)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

static void
groups_table_set_cursor (gchar *old_group)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *group;
	gboolean valid;
	
	if (old_group == NULL)
		return;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &group, -1);

		if (strcmp (old_group, group) == 0 ) {
			GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (groups_table), path, NULL, FALSE);
			gtk_tree_path_free (path);

			return;
		}

		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

void
groups_table_update_content (void)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *group = NULL;
	
	/* gets the group in which the cursor is */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (groups_table), &path, NULL);
	if (path != NULL) {
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, 0, &group, -1);
	}
	
	clear_groups_table ();
	populate_groups_table ();
	
	/* restores the cursor if the group is in the new list */
	groups_table_set_cursor (group);
	
	if (path != NULL) {
		gtk_tree_path_free (path);
		g_free (group);
	}

}
