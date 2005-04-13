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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <gtk/gtk.h>

#include "gst.h"

#include "table.h"
#include "users-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

extern GstTool *tool;

GtkWidget *users_table;
GArray *users_array;

typedef struct _UserTreeItem UserTreeItem;

struct _UserTreeItem
{
	gchar *login;
	guint UID;
	gchar *home;
	gchar *shell;
	gchar *comment;
	
	UserTreeItem *children;
};


gchar *users_table_columns [] = {
	N_("User"),
	N_("Home"),
	N_("User details"),
	N_("UID"),
	N_("Shell"),
	NULL
};

static void
add_user_columns (GtkTreeView *treeview)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	guint i;
	
	for (i = 0; users_table_columns [i] != NULL; i++) {

		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes (_(users_table_columns [i]),
	                                                           renderer,
	                                                           "text", i,
	                                                           NULL);
		gtk_tree_view_column_set_resizable (column, TRUE);
		gtk_tree_view_column_set_sort_column_id (column, i);

		gtk_tree_view_insert_column (GTK_TREE_VIEW (users_table), column, i);
	}
}

static GtkTreeModel*
create_users_model (void)
{
	GtkTreeStore *model;
	
	model = gtk_tree_store_new (COL_USER_LAST,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_STRING,
	                            G_TYPE_INT,
				    G_TYPE_POINTER);
	return GTK_TREE_MODEL (model);
}

void
create_users_table (void)
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkWidget        *popup;
	
	model = create_users_model ();
	
	users_table = gst_dialog_get_widget (tool->main_dialog, "users_table");
	gtk_tree_view_set_model (GTK_TREE_VIEW (users_table), model);
	
	g_object_unref (model);
	
	add_user_columns (GTK_TREE_VIEW (users_table));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (users_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	popup = popup_menu_create (users_table);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_table_clicked),
			  (gpointer) users_table);
	g_signal_connect (G_OBJECT (users_table), "button_press_event",
			  G_CALLBACK (on_table_button_press), popup);
	g_signal_connect (G_OBJECT (users_table), "popup-menu",
			  G_CALLBACK (on_table_popup_menu), popup);
}

static char*
users_table_value (xmlNodePtr node, gchar *key)
{
        g_return_val_if_fail (node != NULL, NULL);
        return gst_xml_get_child_content (node, key);
}

static UserTreeItem*
get_user_node_data (xmlNodePtr user)
{
	gint i, j, len;
        UserTreeItem *item = g_malloc (sizeof(UserTreeItem));
	
	item->login = users_table_value (user, "login");
        item->UID = atoi (users_table_value (user, "uid"));
	item->home = users_table_value (user, "home");
	item->shell = users_table_value (user, "shell");
	item->comment = users_table_value (user, "comment");

	/* Remove ending and double commas from comment */
	len = strlen (item->comment);
	j = 0;
	for (i = 0; i < len; i++) {
		if (item->comment[i] == ',') 	{
			if (item->comment[i+1] == '\0') {
				item->comment[j] = '\0';
			} else 	if (item->comment[i+1] != ',') {
				item->comment[j] = item->comment[i];
				j++;						
			}
		} else {
			item->comment[j] = item->comment[i];
			j++;
		}	
	}
	item->comment[j] = '\0';
        return item;
}

static void 
free_user_tree_item (UserTreeItem *user_tree_item)
{
	g_free(user_tree_item->login);
	g_free(user_tree_item->home);
	g_free(user_tree_item->shell);
	g_free(user_tree_item->comment);

	g_free(user_tree_item);
}
void
populate_users_table (void)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	xmlNodePtr root = get_root_node (NODE_USER);
	GtkTreeIter iter;
	xmlNodePtr user;
	UserTreeItem *item;

	g_return_if_fail (model != NULL);
	g_return_if_fail (root != NULL);

	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	users_array = g_array_new (FALSE, FALSE, sizeof (xmlNodePtr));

	for (user = gst_xml_element_find_first (root, "user"); user != NULL; user = gst_xml_element_find_next (user, "user")) {
		if (check_node_visibility (user)) {
			item = get_user_node_data (user);

			gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
			gtk_tree_store_set (GTK_TREE_STORE (model),
			                    &iter,
					    COL_USER_LOGIN, item->login,
					    COL_USER_HOME, item->home,
					    COL_USER_COMMENT, item->comment,
					    COL_USER_UID, item->UID,
					    COL_USER_SHELL, item->shell,
			                    COL_USER_POINTER, user,
					    -1);
			free_user_tree_item (item);
		}
	}
}

static void
clear_users_table (void)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}

static void
users_table_set_cursor (gchar *old_login)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *login;
	gboolean valid;
	
	if (old_login == NULL)
		return;	

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &login, -1);

		if (strcmp (old_login, login) == 0 ) {
			GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (users_table), path, NULL, FALSE);
			gtk_tree_path_free (path);
			g_free (login);

			return;
		}
		g_free (login);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}

void
users_table_update_content (void)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *login = NULL;
	
	/* gets the user in which the cursor is */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (users_table), &path, NULL);
	if (path != NULL) {
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, 0, &login, -1);
	}
	
	clear_users_table ();
	populate_users_table ();
	
	/* restores the cursor if the user is in the new list */
	users_table_set_cursor (login);
	
	if (path != NULL) {
		gtk_tree_path_free (path);
		g_free (login);
	}
}

