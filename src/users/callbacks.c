/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com>,
 *          Arturo Espinosa <arturo@ximian.com> and
 *          Carlos Garnacho Parro <garparr@teleline.es>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <gnome.h>

#include "callbacks.h"
#include "user_group.h"
#include "transfer.h"
#include "passwd.h"
#include "table.h"
#include "user-settings.h"
#include "users-table.h"
#include "group-settings.h"
#include "groups-table.h"

extern XstTool *tool;

extern GtkWidget *group_settings_all;
extern GtkWidget *group_settings_members;
extern GtkWidget *users_table;
extern GtkWidget *groups_table;

void
on_showall_toggled (GtkToggleButton *toggle, gpointer user_data)
{
	XstDialogComplexity complexity = tool->main_dialog->complexity;

	/* Only saves the change if we are in advanced mode, basic mode will be always FALSE (we don't need to save it) */
	if (complexity == XST_DIALOG_ADVANCED) {
		xst_conf_set_boolean (tool, "showall", gtk_toggle_button_get_active (toggle));
	}
	tables_update_content ();
}

/* Users tab */

void
on_user_table_clicked (GtkTreeSelection *selection, gpointer data)
{
	actions_set_sensitive (TABLE_USER, TRUE);
}

void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;
	GtkWidget *notebook = xst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");

	g_return_if_fail (xst_tool_get_access (tool));
	ud = g_new (ug_data, 1);

	ud->is_new = TRUE;
	ud->table = TABLE_USER;
	ud->node = get_root_node (ud->table);

	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED) {
		/* user settings dialog */
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
	} else {
		/* user settings druid */
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
	}
	user_new_prepare (ud);
}

void
on_user_settings_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;
	GtkWidget *notebook = xst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");

	ud = g_new (ug_data, 1);

	ud->is_new = FALSE;
	ud->table = TABLE_USER;
	ud->node = get_selected_row_node (TABLE_USER);

	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED) {
		/* user settings dialog */
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
	} else {
		/* user settings simple dialog */
		gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
	}

	user_settings_prepare (ud);
}

static void
on_user_delete_clicked_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));
	
	gtk_tree_model_get (model, iter, COL_USER_POINTER, &node, -1);
	delete_user (node);
}

void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (users_table));
	
	gtk_tree_selection_selected_foreach (selection, on_user_delete_clicked_foreach, NULL);
	
	xst_dialog_modify (tool->main_dialog);
	users_table_update_content ();
	actions_set_sensitive (TABLE_USER, FALSE);
}

/* Groups tab */

void
on_group_table_clicked (GtkTreeSelection *selection, gpointer user_data)
{
	actions_set_sensitive (TABLE_GROUP, TRUE);
}

void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *gd;

	g_return_if_fail (xst_tool_get_access (tool));

	gd = g_new (ug_data, 1);

	gd->is_new = TRUE;
	gd->table = TABLE_GROUP;
	gd->node = get_root_node (gd->table);

	group_new_prepare (gd);
}

void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *gd;

	gd = g_new (ug_data, 1);

	gd->is_new = FALSE;
	gd->table = TABLE_GROUP;
	gd->node = get_selected_row_node (TABLE_GROUP);

	group_settings_prepare (gd);
}

static void
on_group_delete_clicked_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));
	
	gtk_tree_model_get (model, iter, COL_GROUP_POINTER, &node, -1);
	delete_group (node);
}

void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (groups_table));
	
	gtk_tree_selection_selected_foreach (selection, on_group_delete_clicked_foreach, NULL);
	
	xst_dialog_modify (tool->main_dialog);
	groups_table_update_content ();
	actions_set_sensitive (TABLE_GROUP, FALSE);
}

#ifdef NIS
/* Network tab. */

void
on_network_delete_clicked (GtkWidget *button, gpointer user_data)
{
	xmlNodePtr node;
	gboolean delete = FALSE;
	gint tbl = -1;

	g_return_if_fail (xst_tool_get_access (tool));

	node = get_selected_node ();

	/* Is it user or group? */

	if (!strcmp (node->name, "user"))
	{
		delete = check_login_delete (node);
		tbl = TABLE_NET_USER;
	}
	else if (!strcmp (node->name, "group"))
	{
		delete = check_group_delete (node);
		tbl = TABLE_NET_GROUP;
	}

	if (delete)
	{
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (tbl))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_GROUP, FALSE);
	}
}

void
on_network_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	user_settings_prepare (get_root_node (TABLE_NET_USER));
}

void
on_network_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	ud = g_new (ug_data, 1);

	ud->new = TRUE;
	ud->table = TABLE_NET_GROUP;
	ud->node = get_root_node (ud->table);

	group_new_prepare (ud);
}
#endif

/* User settings callbacks */

void
on_user_settings_passwd_changed (GtkEntry *entry, gpointer data)
{
	g_object_set_data (G_OBJECT (entry), "changed", GINT_TO_POINTER (TRUE));
}


void
on_user_settings_dialog_show (GtkWidget *widget, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_name"));
}

void
on_user_settings_dialog_delete_event (GtkWidget *w, gpointer user_data)
{
	user_settings_dialog_close ();
}

void
on_user_settings_ok_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *widget;
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	widget = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	ud = g_object_get_data (G_OBJECT (widget), "data");

	if (user_update (ud))
	{
		xst_dialog_modify (tool->main_dialog);
		user_settings_dialog_close ();
	}
}

void
on_user_settings_passwd_random_new (GtkButton *button, gpointer data)
{
	gchar *passwd;

	passwd = passwd_get_random ();
	gtk_label_set_text (GTK_LABEL (xst_dialog_get_widget (tool->main_dialog, "user_passwd_random_label")), passwd);
	g_free (passwd);
}

void
on_user_settings_passwd_toggled (GtkToggleButton *toggle, gpointer data)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (xst_dialog_get_widget (tool->main_dialog, "user_passwd_notebook"));
	GtkToggleButton *pwd_manual = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_manual"));
	

	if (gtk_toggle_button_get_active (pwd_manual)) {
		gtk_notebook_set_current_page (notebook, 0);
	} else {
		gtk_notebook_set_current_page (notebook, 1);
		on_user_settings_passwd_random_new (NULL, NULL);
	}
}

void
on_user_settings_profile_changed (GtkWidget *widget, gpointer data)
{
	gchar *profile = data;
}

/* Group settings callbacks */

void
on_group_settings_dialog_show (GtkWidget *widget, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "group_settings_name"));
}

void
on_group_settings_dialog_delete_event (GtkWidget *w, gpointer user_data)
{
	group_settings_dialog_close ();
}

void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *widget;
	ug_data *gd;

	g_return_if_fail (xst_tool_get_access (tool));

	widget = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	gd = g_object_get_data (G_OBJECT (widget), "data");

	if (group_update (gd))
	{
		xst_dialog_modify (tool->main_dialog);
		group_settings_dialog_close ();
	}
}

void
on_add_remove_button_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeView *in, *out;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList *in_list, *out_list;
	GList *element;
	
	g_return_if_fail (xst_tool_get_access (tool));
	
	in = g_object_get_data (G_OBJECT (button), "in");
	out = g_object_get_data (G_OBJECT (button), "out");
	
	g_return_if_fail (in != NULL || out != NULL);

	in_list = g_object_get_data (G_OBJECT (in), "list");
	out_list = g_object_get_data (G_OBJECT (out), "list");
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (in));
	model = gtk_tree_view_get_model (in);
	gtk_tree_model_get_iter_first (model, &iter);
	
	do {
		if (gtk_tree_selection_iter_is_selected (selection, &iter)) {
			/* gets the user that is being removed from the 'in' list */
			gtk_tree_model_get (model, &iter,
	                                    1, &element,
	                                    -1);
			/* adds the user to the 'out' list */
                        out_list = g_list_insert_sorted (out_list, element->data, my_strcmp);
			
			/* removes the user from the 'in' list */
			in_list = g_list_remove (in_list, element->data);
		}
	} while (gtk_tree_model_iter_next (model, &iter));
	
	/* reattach GLists to GtkTreeViews */
	g_object_set_data (G_OBJECT (in), "list", in_list);
	g_object_set_data (G_OBJECT (out), "list", out_list);

	/* Refresh the 'in' and 'out' lists */
	clear_gtk_tree_list (in);
	clear_gtk_tree_list (out);
	populate_gtk_tree_list (in, in_list);
	populate_gtk_tree_list (out, out_list);
	
	/* sets unsensitive the button again */
	gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);
}

void
on_list_select_row (GtkTreeSelection *selection, gpointer data)
{
	GtkTreePath *path;
	GtkTreeView *list;
	GtkWidget *widget;
	gchar *button;
	
	list = gtk_tree_selection_get_tree_view (selection);
	button = g_object_get_data (G_OBJECT (list), "button");
	
	widget = xst_dialog_get_widget (tool->main_dialog, button);
	gtk_widget_set_sensitive (widget, TRUE);
}

/* Helpers .*/

void
actions_set_sensitive (gint table, gboolean state)
{
	switch (table)
	{
	case TABLE_USER:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_new",      TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_delete",   state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_settings", state);
		break;
	case TABLE_GROUP:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_new",      TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_delete",   state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_settings", state);
		break;
/*	case TABLE_NET_USER:
	case TABLE_NET_GROUP:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_group_new", TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_user_new",  TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_delete",    state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_settings",  state);
		break;*/
	default:
		g_warning ("actions_set_sensitive: Shouldn't be here.");
		return;
	}
}
