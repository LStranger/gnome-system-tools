/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* privileges-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gst.h"
#include "privileges-table.h"
#include "user_group.h"

extern GstTool *tool;

static void
on_user_privilege_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel*) data;
	GtkTreePath  *path  = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter   iter;
	gboolean      value;

	gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
	gtk_tree_model_get (model, &iter, 0, &value, -1);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, !value, -1);
}

void
create_user_privileges_table ()
{
	GtkWidget         *list = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	GtkTreeModel      *model;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GtkTreeIter        iter;

	model = GTK_TREE_MODEL (gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING));

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
	g_object_unref (model);

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "active", 0,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (on_user_privilege_toggled), model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "text", 1,
					     NULL);

	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list), column, 0);
	gtk_tree_view_column_clicked (column);
}

void
populate_user_privileges_table (gchar *username)
{
	GtkWidget    *list = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	xmlNodePtr    root = get_root_node (NODE_GROUP);
	xmlNodePtr    group, user;
	GtkTreeIter   iter;
	gchar        *groupname, *desc, *buf;
	gboolean      val;

	gtk_list_store_clear (GTK_LIST_STORE (model));
	
	for (group = gst_xml_element_find_first (root, "group");
	     group;
	     group = gst_xml_element_find_next (group, "group")) {
		desc = gst_xml_get_child_content (group, "allows_to");

		if (desc) {
			val = FALSE;

			groupname = gst_xml_get_child_content (group, "name");
			
			/* check whether the user is already in the group */
			if (username) {
				user = gst_xml_element_find_first (group, "users");
				
				for (user = gst_xml_element_find_first (user, "user");
				     user;
				     user = gst_xml_element_find_next (user, "user")) {
					buf = gst_xml_element_get_content (user);

					if (strcmp (username, buf) == 0)
						val = TRUE;

					g_free (buf);
				}
			}
			
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, val,
					    1, desc,
					    2, groupname,
					    -1);
			g_free (desc);
			g_free (groupname);
		}
	}
}

GList*
user_privileges_get_list ()
{
	GtkWidget    *list = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (list));
	GList        *groups = NULL;
	gboolean      valid, active;
	GtkTreeIter   iter;
	gchar        *group;

	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &active, 2, &group, -1);

		if (active)
			groups = g_list_prepend (groups, group);

		valid = gtk_tree_model_iter_next (model, &iter);
	}

	return groups;
}
