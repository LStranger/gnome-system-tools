/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* profiles-table.c: this file is part of users-admin, a gnome-system-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gst.h"
#include "table.h"
#include "profiles-table.h"
#include "user_group.h"
#include "callbacks.h"

ProfilesTableConfig profiles_table_config [] = {
	   /*Column name,          Adv_state_show,	Basic_state_show*/
	   { N_("Profile"),        TRUE,             TRUE },
	   { N_("Comment"),        TRUE,             TRUE },
	   {NULL}
};

extern GstTool *tool;

static void
add_profiles_columns (GtkTreeView *treeview)
{
	   GtkTreeViewColumn *column;
	   GtkCellRenderer *renderer;
	   ProfilesTableConfig *i;
	   guint j;

	   for (i = profiles_table_config, j = 0;
		   i->name != NULL;
		   i++, j++)
	   {
			 renderer = gtk_cell_renderer_text_new ();

			 column = gtk_tree_view_column_new_with_attributes (i->name,
									    renderer,
									    "text",
									    j,
									    NULL);
			 gtk_tree_view_column_set_resizable (column, TRUE);
			 gtk_tree_view_column_set_sort_column_id (column, j);

			 gtk_tree_view_insert_column (treeview, column, j);
	   }
}

static GtkTreeModel*
create_profiles_model (void)
{
	   GtkTreeStore *model;

	   model = gtk_tree_store_new (COL_PROFILE_LAST,
				       G_TYPE_STRING,
				       G_TYPE_STRING,
				       G_TYPE_POINTER);
	   return GTK_TREE_MODEL (model);
}

void
create_profiles_table (void)
{
	   GtkWidget *profiles_table = gst_dialog_get_widget (tool->main_dialog, "profiles_table");
	   GtkTreeModel *model = create_profiles_model ();
	   GtkTreeSelection *selection;

	   gtk_tree_view_set_model (GTK_TREE_VIEW (profiles_table), model);

	   g_object_unref (model);

	   add_profiles_columns (GTK_TREE_VIEW (profiles_table));

	   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profiles_table));
	   gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	   g_signal_connect (G_OBJECT (selection), "changed",
			     G_CALLBACK (on_table_clicked),
			     (gpointer) profiles_table);
}

void
populate_profiles_table (void)
{
	   GtkWidget *profile_table = gst_dialog_get_widget (tool->main_dialog, "profiles_table");
	   GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (profile_table));
	   xmlNodePtr root = get_root_node (NODE_PROFILE);
	   xmlNodePtr profile;
	   GtkTreeIter iter;

	   for (profile = gst_xml_element_find_first (root, "profile");
		   profile != NULL;
		   profile = gst_xml_element_find_next (profile, "profile"))
	   {
			 gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
			 gtk_tree_store_set (GTK_TREE_STORE (model),
					     &iter,
					     COL_PROFILE_NAME, gst_xml_get_child_content (profile, "name"),
					     COL_PROFILE_COMMENT, gst_xml_get_child_content (profile, "comment"),
					     COL_PROFILE_POINTER, profile,
					     -1);
	   }
}

void
profiles_table_update_content (void)
{
	GtkWidget *profile_table = gst_dialog_get_widget (tool->main_dialog, "profiles_table");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (profile_table));

	gtk_tree_store_clear (GTK_TREE_STORE (model));
	populate_profiles_table ();
}
