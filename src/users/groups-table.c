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

#include <config.h>
#include "gst.h"
#include <glib/gi18n.h>

#include "table.h"
#include "groups-table.h"
#include "callbacks.h"

extern GstTool *tool;

static void
add_group_columns (GtkTreeView *treeview)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	guint i;

	column = gtk_tree_view_column_new ();

	/* Group name */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Group name"),
							   renderer,
							   "text", COL_GROUP_NAME, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_column_set_expand (column, TRUE);

	gtk_tree_view_insert_column (treeview, column, -1);
}

static gboolean
groups_model_filter (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GstUsersTool *tool = (GstUsersTool *) data;
	gint gid;

	gtk_tree_model_get (model, iter,
			    COL_GROUP_ID, &gid,
			    -1);

	return (tool->showall ||
		(gid == 0 ||
		 (gid >= tool->minimum_gid &&
		  gid <= tool->maximum_gid)));
}

static GtkTreeModel*
create_groups_model (void)
{
	GtkListStore *store;
	GtkTreeModel *filter_model;
	
	store = gtk_list_store_new (COL_GROUP_LAST,
	                            G_TYPE_STRING,
	                            G_TYPE_INT,
	                            G_TYPE_POINTER,
				    G_TYPE_POINTER);
	filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);

	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						groups_model_filter, tool, NULL);
	return filter_model;
}

void
create_groups_table (void)
{
	GtkWidget *groups_table;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkWidget *popup;
	
	groups_table = gst_dialog_get_widget (tool->main_dialog, "groups_table");

	model = create_groups_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (groups_table), model);
	g_object_unref (model);

	add_group_columns (GTK_TREE_VIEW (groups_table));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (groups_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	popup = popup_menu_create (groups_table, TABLE_GROUPS);
	g_object_set_data_full (G_OBJECT (groups_table),
				"popup", popup,
				(GDestroyNotify) gtk_widget_destroy);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_table_clicked),
			  GINT_TO_POINTER (TABLE_GROUPS));
	g_signal_connect (G_OBJECT (groups_table), "button_press_event",
			  G_CALLBACK (on_table_button_press),
			  GINT_TO_POINTER (TABLE_GROUPS));
	g_signal_connect (G_OBJECT (groups_table), "popup-menu",
			  G_CALLBACK (on_table_popup_menu), NULL);
}

void
groups_table_set_group (OobsGroup *group, OobsListIter *list_iter, GtkTreeIter *iter)
{
	GtkWidget *groups_table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	GtkTreeModel *filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	GtkTreeModel *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    COL_GROUP_NAME, oobs_group_get_name (group),
			    COL_GROUP_ID, oobs_group_get_gid (group),
			    COL_GROUP_OBJECT, group,
			    COL_GROUP_ITER, oobs_list_iter_copy (list_iter),
			    -1);
}

void
groups_table_add_group (OobsGroup *group, OobsListIter *list_iter)
{
	GtkWidget *groups_table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	GtkTreeModel *filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	GtkTreeModel *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));
	GtkTreeIter iter;

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	groups_table_set_group (group, list_iter, &iter);
}
