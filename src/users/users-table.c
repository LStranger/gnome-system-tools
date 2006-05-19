/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* users-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
#include "users-table.h"
#include "callbacks.h"

extern GstTool *tool;

static void
add_user_columns (GtkTreeView *treeview)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	guint i;

	/* Face */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Name"));
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COL_USER_FACE, NULL);

	/* User full name */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", COL_USER_NAME, NULL);

	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_column_set_expand (column, TRUE);

	gtk_tree_view_insert_column (treeview, column, -1);

	/* Login name */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Login name"),
							   renderer,
							   "text", COL_USER_LOGIN, NULL);
	gtk_tree_view_insert_column (treeview, column, -1);

	/* Home directory */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Home directory"),
							   renderer,
							   "text", COL_USER_HOME, NULL);
	gtk_tree_view_insert_column (treeview, column, -1);
}

static gboolean
users_model_filter (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	GstUsersTool *tool = (GstUsersTool *) data;
	gint uid;

	gtk_tree_model_get (model, iter,
			    COL_USER_ID, &uid,
			    -1);
	return (tool->showall ||
		(uid == 0 ||
		 (uid >= tool->minimum_uid &&
		  uid <= tool->maximum_uid)));
}

static GtkTreeModel*
create_users_model (GstUsersTool *tool)
{
	GtkListStore *store;
	GtkTreeModel *filter_model;
	
	store = gtk_list_store_new (COL_USER_LAST,
				    GDK_TYPE_PIXBUF,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_BOOLEAN,
				    G_TYPE_POINTER,
				    G_TYPE_POINTER);
	filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);

	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						users_model_filter, tool, NULL);
	return filter_model;
}

void
create_users_table (GstUsersTool *tool)
{
	GtkWidget *users_table;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkWidget *popup;
	
	users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");

	model = create_users_model (tool);
	gtk_tree_view_set_model (GTK_TREE_VIEW (users_table), model);
	g_object_unref (model);
	
	add_user_columns (GTK_TREE_VIEW (users_table));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (users_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	popup = popup_menu_create (users_table, TABLE_USERS);
	g_object_set_data_full (G_OBJECT (users_table),
				"popup", popup,
				(GDestroyNotify) gtk_widget_destroy);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_table_clicked),
			  GINT_TO_POINTER (TABLE_USERS));
	g_signal_connect (G_OBJECT (users_table), "button_press_event",
			  G_CALLBACK (on_table_button_press),
			  GINT_TO_POINTER (TABLE_USERS));
	g_signal_connect (G_OBJECT (users_table), "popup-menu",
			  G_CALLBACK (on_table_popup_menu), NULL);
}

static GdkPixbuf*
get_user_face (const gchar *homedir)
{
	gchar *face_path = g_strdup_printf ("%s/.face", homedir);
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_file_at_size (face_path, 24, 24, NULL);

	if (!pixbuf)
		pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "stock_person", 24, 0, NULL);

	g_free (face_path);

	return pixbuf;
}

void
users_table_set_user (OobsUser *user, OobsListIter *list_iter, GtkTreeIter *iter)
{
	GtkWidget *users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	GtkTreeModel *filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	GtkTreeModel *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));
	GdkPixbuf *face;

	face = get_user_face (oobs_user_get_home_directory (user));

	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    COL_USER_FACE, face,
			    COL_USER_NAME, oobs_user_get_full_name (user),
			    COL_USER_LOGIN, oobs_user_get_login_name (user),
			    COL_USER_HOME, oobs_user_get_home_directory (user),
			    COL_USER_ID, oobs_user_get_uid (user),
			    COL_USER_OBJECT, user,
			    COL_USER_ITER, oobs_list_iter_copy (list_iter),
			    -1);
	if (face)
		g_object_unref (face);
}

void
users_table_add_user (OobsUser *user, OobsListIter *list_iter)
{
	GtkWidget *users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	GtkTreeModel *filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	GtkTreeModel *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));
	GtkTreeIter iter;

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	users_table_set_user (user, list_iter, &iter);
}
