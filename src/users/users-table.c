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

	column = gtk_tree_view_column_new ();

	/* Face */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COL_USER_FACE,
					     "sensitive", COL_USER_SENSITIVE,
					     NULL);
	g_object_set (G_OBJECT (renderer),
		      "xpad", 3,
		      "ypad", 6,
		      NULL);
	/* User full name and login, on two lines */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "markup", COL_USER_LABEL,
					     NULL);
	g_object_set (G_OBJECT (renderer),
		      "xpad", 3,
		      "ypad", 6,
		      NULL);

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
	                            G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_BOOLEAN,
				    G_TYPE_OBJECT,
				    OOBS_TYPE_LIST_ITER,
				    G_TYPE_BOOLEAN);
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
			  G_CALLBACK (on_table_selection_changed),
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

	pixbuf = gdk_pixbuf_new_from_file_at_size (face_path, 48, 48, NULL);

	if (!pixbuf)
		pixbuf = gtk_icon_theme_load_icon (tool->icon_theme, "stock_person", 48, 0, NULL);

	g_free (face_path);

	return pixbuf;
}

void
users_table_set_user (OobsUser *user, OobsListIter *list_iter, GtkTreeIter *iter)
{
	GtkWidget *users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	GtkTreeModel *filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	GtkTreeModel *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));
	OobsObject *object = GST_USERS_TOOL (tool)->self_config;
	GdkPixbuf *face;
	const char *name;
	const char *login;
	char *label;

	face = get_user_face (oobs_user_get_home_directory (user));
	name = oobs_user_get_full_name (user);
	login = oobs_user_get_login_name (user);
	label = g_strdup_printf ("<big><b>%s</b>\n<span color=\'dark grey\'><i>%s</i></span></big>", name, login);

	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    COL_USER_FACE, face,
			    COL_USER_NAME, name,
			    COL_USER_LOGIN, login,
			    COL_USER_LABEL, label,
			    COL_USER_HOME, oobs_user_get_home_directory (user),
			    COL_USER_ID, oobs_user_get_uid (user),
			    COL_USER_OBJECT, user,
			    COL_USER_ITER, list_iter,
			    COL_USER_SENSITIVE, sensitive,
			    -1);
	g_free (label);
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

void
users_table_clear (void)
{
        GtkWidget *users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
        GtkTreeModel *filter_model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
        GtkTreeModel *model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

        gtk_list_store_clear (GTK_LIST_STORE (model));
}

void
users_table_select_first (void)
{
	GtkWidget *users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (users_table));
	GtkTreePath *first_user = gtk_tree_path_new_first ();

	gtk_tree_selection_select_path (selection, first_user);
}

