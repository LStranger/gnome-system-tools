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

#include <gnome.h>
#include <gtk/gtk.h>

#include "xst.h"

#include "table.h"
#include "groups-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

GroupsTableConfig groups_table_config [] = {
	{ "Group",	TRUE,	TRUE},
	{ "GID",	TRUE,	FALSE},
	{NULL}
};

extern XstTool *tool;

GtkWidget *groups_table;

static void
add_group_columns (GtkTreeView *treeview)
{
	GtkCellRenderer *renderer;
	GroupsTableConfig *i;
	guint j;
	
	for (i = groups_table_config, j = 0;
	     i->name != NULL;
	     i++, j++)
	{
		renderer = gtk_cell_renderer_text_new ();
		g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
		
		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
		                                             -1,
		                                             i->name,
		                                             renderer,
		                                             "text",
		                                             j,
		                                             NULL);
	}
}

static GtkTreeModel*
create_groups_model (void)
{
	GtkTreeStore *model;
	
	model = gtk_tree_store_new (COL_GROUP_LAST,
	                            G_TYPE_STRING,
	                            G_TYPE_INT);
	return GTK_TREE_MODEL (model);
}

static GtkWidget*
create_groups_table (void)
{
	GtkTreeModel *model;
	
	model = create_groups_model ();
	
	groups_table = gtk_tree_view_new_with_model (model);
	
	g_object_unref (model);

        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (groups_table), TRUE);
	
	add_group_columns (GTK_TREE_VIEW (groups_table));
	
	g_signal_connect  (G_OBJECT (groups_table),
	                   "cursor-changed",
	                   G_CALLBACK (on_group_table_clicked),
	                   NULL);
	
	return groups_table;
}

void
construct_groups_table (void)
{
	GtkWidget *sw;
	GtkWidget *list;

	sw = xst_dialog_get_widget (tool->main_dialog, "groups_table");

	list = create_groups_table ();

	gtk_widget_show_all (list);
	gtk_container_add (GTK_CONTAINER (sw), list);
}

static char*
groups_table_value (xmlNodePtr node, gchar *key)
{
        g_return_val_if_fail (node != NULL, NULL);
        return xst_xml_get_child_content (node, key);
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
	xmlNodePtr root = get_group_root_node ();
	GtkTreeIter iter;
	xmlNodePtr group;
	
	g_return_if_fail (model != NULL);
	g_return_if_fail (root != NULL);
	
	for (group = xst_xml_element_find_first (root, "group"); group != NULL; group = xst_xml_element_find_next (group, "group"))
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
	
	if (old_group == NULL)
		return;	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	if (gtk_tree_model_get_iter_first (model, &iter) != TRUE)
		return;
	do {
		gtk_tree_model_get (model, &iter, 0, &group, -1);
		if (strcmp (old_group, group) == 0 ) 
		{
			GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (groups_table), path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
	} while (gtk_tree_model_iter_next (model, &iter) != FALSE);
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
