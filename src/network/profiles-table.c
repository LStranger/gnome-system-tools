/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "gst.h"
#include "profiles-table.h"
#include "callbacks.h"

static GtkTreeModel*
profiles_table_create_model (void)
{
	GtkTreeStore *store = gtk_tree_store_new (PROFILES_TABLE_COL_LAST,
						  G_TYPE_STRING,
						  G_TYPE_STRING,
						  G_TYPE_POINTER);
	
	return GTK_TREE_MODEL (store);
}

static void
profiles_table_add_columns (GtkTreeView *table)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	
	/* Name */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Profile name"), cell,
							   "text", PROFILES_TABLE_COL_NAME,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, PROFILES_TABLE_COL_NAME);
	gtk_tree_view_append_column (table, column);
	
	/* Description */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"), cell,
							   "text", PROFILES_TABLE_COL_DESCRIPTION,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, PROFILES_TABLE_COL_DESCRIPTION);
	gtk_tree_view_append_column (table, column);
}

void
profiles_table_populate (GstTool *tool, xmlNodePtr root)
{
	GtkWidget *profiles_table = gst_dialog_get_widget (tool->main_dialog, "network_profiles_table");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (profiles_table));
	xmlNodePtr profiledb = gst_xml_element_find_first (root, "profiledb");
	xmlNodePtr profile;
	GtkTreeIter iter;

	for (profile = gst_xml_element_find_first (profiledb, "profile");
	     profile != NULL;
	     profile = gst_xml_element_find_next (profile, "profile"))
	{
		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model),
				    &iter,
				    PROFILES_TABLE_COL_NAME, gst_xml_get_child_content (profile, "name"),
				    PROFILES_TABLE_COL_DESCRIPTION, gst_xml_get_child_content (profile, "description"),
				    PROFILES_TABLE_COL_POINTER, profile,
				    -1);
	}
}

void
profiles_table_create (GstTool *tool)
{
	GtkWidget *table = gst_dialog_get_widget (tool->main_dialog, "network_profiles_table");
	GtkTreeModel *model = profiles_table_create_model ();
	GtkTreeSelection *selection;
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);
	profiles_table_add_columns (GTK_TREE_VIEW (table));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_network_profile_table_selection_changed), NULL);
}

static void
profiles_table_clear (GstTool *tool)
{
	GtkWidget *table = gst_dialog_get_widget (tool->main_dialog, "network_profiles_table");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

void
profiles_table_update_content (GstTool *tool)
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	profiles_table_clear (tool);
	profiles_table_populate (tool, root);
}
