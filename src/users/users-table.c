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

#include "xst.h"

#include "users-table.h"
#include "user_group.h"
#include "callbacks.h"
#include "user-group-xml.h"

TableConfig users_table_config [] = {
	/*Column name,          Adv_state_show,	Basic_state_show*/
	{ "User",		TRUE,		TRUE},
	{ "UID",		TRUE,		FALSE},
	{ "Home",		TRUE,		FALSE},
	{ "Shell",		TRUE,		FALSE},
	{ "Comments",		FALSE,		TRUE},
	{ "Group",		FALSE,		TRUE},
//	{ "GID",		FALSE,		FALSE},
	{NULL}
};

extern XstTool *tool;

GtkWidget *users_table;
GArray *users_array;

static void
add_user_columns (GtkTreeView *treeview)
{
	GtkCellRenderer *renderer;
	TableConfig *i;
	guint j;
	
	for (i = users_table_config, j = 0; 
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
create_users_model (void)
{
	GtkTreeStore *model;
	
	model = gtk_tree_store_new (COL_USER_LAST,
	                            G_TYPE_STRING,
	                            G_TYPE_INT,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_STRING,
	                            G_TYPE_INT);
	return GTK_TREE_MODEL (model);
}

static GtkWidget*
create_users_table (void)
{
	GtkTreeModel *model;
	
	model = create_users_model ();
	
	users_table = gtk_tree_view_new_with_model (model);
	
	g_object_unref (model);
	
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (users_table), TRUE);

	add_user_columns (GTK_TREE_VIEW (users_table));
	
	gtk_signal_connect (GTK_OBJECT (users_table),
	                    "cursor_changed",
	                    G_CALLBACK (on_user_table_clicked),
	                    NULL);
	
	return users_table;
}

void 
construct_users_table (void)
{
	GtkWidget *sw;
	GtkWidget *list;
	
	sw = xst_dialog_get_widget (tool->main_dialog, "users_table");
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	
	list = create_users_table ();
	
	gtk_widget_show_all (list);
	gtk_container_add (GTK_CONTAINER (sw), list);
}

static char*
users_table_value (xmlNodePtr node, gchar *key)
{
        g_return_val_if_fail (node != NULL, NULL);
        return xst_xml_get_child_content (node, key);
}

static UserTreeItem*
get_user_node_data (xmlNodePtr user)
{
        UserTreeItem *item = g_malloc (sizeof(UserTreeItem));
	
	item->login = users_table_value (user, "login");
        item->UID = atoi (users_table_value (user, "uid"));
	item->home = users_table_value (user, "home");
	item->shell = users_table_value (user, "shell");
	item->comment = users_table_value (user, "comment");
	item->group = user_value_group_peek (user);
        
        return item;
}

void
populate_users_table (void)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	xmlNodePtr root = get_user_root_node ();
	GtkTreeIter iter;
	xmlNodePtr user;
	
	g_return_if_fail (model != NULL);
	g_return_if_fail (root != NULL);
	
	users_array = g_array_new (FALSE, FALSE, sizeof (xmlNodePtr));
	
	for (user = xst_xml_element_find_first (root, "user"); user != NULL; user = xst_xml_element_find_next (user, "user"))
	{
		if (check_node_visibility (user))
		{
			UserTreeItem *item;
			item = get_user_node_data (user);
		
			gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
			gtk_tree_store_set (GTK_TREE_STORE (model),
			                    &iter,
					    COL_USER_LOGIN, item->login,
					    COL_USER_UID, item->UID,
					    COL_USER_HOME, item->home,
					    COL_USER_SHELL, item->shell,
					    COL_USER_COMMENT, item->comment,
					    COL_USER_GROUP, item->group,
					    -1);
		}
	}
}

void
update_users_table_complexity (XstDialogComplexity complexity)
{
	GtkTreeView *u_table = GTK_TREE_VIEW (users_table);
	GtkTreeViewColumn *column;
	TableConfig *i;
	guint j;
	
	switch (complexity) {
	case XST_DIALOG_BASIC:
		for (i = users_table_config, j=0;
		     i->name != NULL;
		     i++, j++)
		{
			column = gtk_tree_view_get_column (u_table, j);
			gtk_tree_view_column_set_visible (column, i->basic_state_showable);
		}
		break;
	case XST_DIALOG_ADVANCED:
		for (i = users_table_config, j=0;
		     i->name != NULL;
		     i++, j++)
		{
			column = gtk_tree_view_get_column (u_table, j);
			gtk_tree_view_column_set_visible (column, i->advanced_state_showable);
		}
		break;
	default:
		g_warning ("tables_update_complexity: Unsupported complexity.");
		return;
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
	
	if (old_login == NULL)
		return;	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));
	if (gtk_tree_model_get_iter_first (model, &iter) != TRUE)
		return;
	do {
		gtk_tree_model_get (model, &iter, 0, &login, -1);
		if (strcmp (old_login, login) == 0 ) 
		{
			GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (users_table), path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
	} while (gtk_tree_model_iter_next (model, &iter) != FALSE);
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

