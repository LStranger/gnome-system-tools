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
#include "profile-settings.h"
#include "profiles-table.h"
#include "search-bar/search-bar.h"

extern GstTool *tool;

extern GtkWidget *group_settings_all;
extern GtkWidget *group_settings_members;
extern GtkWidget *users_table;
extern GtkWidget *groups_table;

void
on_showall_toggled (GtkToggleButton *toggle, gpointer user_data)
{
	GstDialogComplexity complexity = tool->main_dialog->complexity;
	SearchBar *sb = SEARCH_BAR (g_object_get_data (G_OBJECT (tool->main_dialog), "SearchBar"));

	/* we ought to clear previous searches */
	search_bar_clear_search (sb);
	
	/* Only saves the change if we are in advanced mode, basic mode will be always FALSE (we don't need to save it) */
	if (complexity == GST_DIALOG_ADVANCED) {
		gst_conf_set_boolean (tool, "showall", gtk_toggle_button_get_active (toggle));
	}
	tables_update_content ();
}

/* Common stuff to users and groups tables */

void
counter (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	int *cont = (int *) data;

	(*cont) ++;
}

void
on_table_clicked (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeView *treeview;
	int cont;

	treeview = (GtkTreeView *) data;
	cont = 0;

	gtk_tree_selection_selected_foreach (selection, counter , &cont);
   
	if (users_table == GTK_WIDGET (treeview))
		actions_set_sensitive (NODE_USER, TRUE);
	else if (groups_table == GTK_WIDGET (treeview))
		actions_set_sensitive (NODE_GROUP, TRUE);
	else
		actions_set_sensitive (NODE_PROFILE, TRUE);
	
	if (cont > 1)
	{
		if (users_table == GTK_WIDGET (treeview))
			gst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_settings", FALSE);
		else if (groups_table == GTK_WIDGET (treeview))
			gst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_settings", FALSE);
		else
			gst_dialog_widget_set_user_sensitive (tool->main_dialog, "profile_settings", FALSE);
	}
}

gboolean
on_table_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata)
{
	GtkTreePath *path;
	GtkItemFactory *factory;
	GtkTreeSelection* selection;
	gint cont;

	factory = (GtkItemFactory *) gdata;

	cont = 0;

	selection = gtk_tree_view_get_selection (treeview);
	gtk_tree_selection_selected_foreach (selection, counter , &cont);
	
	if (event->button == 3)
	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			if (cont < 1)
			{
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			}

			if (cont > 1)
				gtk_widget_set_sensitive (gtk_item_factory_get_widget_by_action (factory,
												 POPUP_SETTINGS),
							  FALSE);
			else
				gtk_widget_set_sensitive (gtk_item_factory_get_widget_by_action (factory,
												 POPUP_SETTINGS),
							  TRUE);

			gtk_tree_path_free (path);
			
			gtk_item_factory_popup (factory, event->x_root, event->y_root,
						event->button, event->time);
		}
	}
	
	return FALSE;
}

/* Users Tab */

void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;
	GtkWidget *notebook = gst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");

	g_return_if_fail (gst_tool_get_access (tool));
	ud = g_new (ug_data, 1);

	ud->is_new = TRUE;
	ud->table = NODE_USER;
	ud->node = get_root_node (ud->table);

	if (gst_dialog_get_complexity (tool->main_dialog) == GST_DIALOG_ADVANCED) {
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
	GtkWidget *notebook = gst_dialog_get_widget (tool->main_dialog, "user_settings_notebook");

	ud = g_new (ug_data, 1);

	ud->is_new = FALSE;
	ud->table = NODE_USER;
	ud->node = get_selected_row_node (NODE_USER);

	if (gst_dialog_get_complexity (tool->main_dialog) == GST_DIALOG_ADVANCED) {
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

	g_return_if_fail (gst_tool_get_access (tool));
	
	gtk_tree_model_get (model, iter, COL_USER_POINTER, &node, -1);

	if ((delete_user (node)) == TRUE)
		* (gboolean *) data = TRUE;
}

void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (users_table));
	gboolean deleted = FALSE;
	
	gtk_tree_selection_selected_foreach (selection, on_user_delete_clicked_foreach, &deleted);
	
	/* At this point changed will be TRUE if any of the selected users has been deleted,
	 * the apply button must be set sensitive and the table must be updated */
	if (deleted == TRUE) {
		gst_dialog_modify (tool->main_dialog);
		users_table_update_content ();
	}

	gtk_tree_selection_unselect_all (selection);
	actions_set_sensitive (NODE_USER, FALSE);
}

void
on_profile_settings_dialog_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *profile_window = gst_dialog_get_widget (tool->main_dialog, "profiles_dialog");
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "user_settings_profile_menu");
	
	gtk_dialog_run (GTK_DIALOG (profile_window));
	gtk_widget_hide (profile_window);
}

void
on_profile_settings_users_dialog_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *profile_window = gst_dialog_get_widget (tool->main_dialog, "profiles_dialog");
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "user_settings_profile_menu");
	
	gtk_dialog_run (GTK_DIALOG (profile_window));
	gtk_widget_hide (profile_window);

	gtk_option_menu_remove_menu (GTK_OPTION_MENU (option_menu));
	option_menu_add_profiles (option_menu);
}

/* Groups tab */

void
on_group_table_clicked (GtkTreeSelection *selection, gpointer user_data)
{
	actions_set_sensitive (NODE_GROUP, TRUE);
}

void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *gd;

	g_return_if_fail (gst_tool_get_access (tool));

	gd = g_new (ug_data, 1);

	gd->is_new = TRUE;
	gd->table = NODE_GROUP;
	gd->node = get_root_node (gd->table);

	group_new_prepare (gd);
}

void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *gd;

	gd = g_new (ug_data, 1);

	gd->is_new = FALSE;
	gd->table = NODE_GROUP;
	gd->node = get_selected_row_node (NODE_GROUP);

	group_settings_prepare (gd);
}

static void
on_group_delete_clicked_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	xmlNodePtr node;

	g_return_if_fail (gst_tool_get_access (tool));
	
	gtk_tree_model_get (model, iter, COL_GROUP_POINTER, &node, -1);

	if ((delete_group (node)) == TRUE)
		* (gboolean *) data = TRUE;
}

void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (groups_table));
	gboolean deleted = FALSE;
	
	gtk_tree_selection_selected_foreach (selection, on_group_delete_clicked_foreach, &deleted);

	/* At this point changed will be true only if one of the selected groups has been deleted */
	if (deleted == TRUE) {	
		gst_dialog_modify (tool->main_dialog);
		groups_table_update_content ();
	}

	gtk_tree_selection_unselect_all (selection);
	actions_set_sensitive (NODE_GROUP, FALSE);
}

void
on_popup_add_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	if (GTK_WIDGET (callback_data) == groups_table)
		on_group_new_clicked (callback_data, NULL);
	else if (GTK_WIDGET (callback_data) == users_table)
		on_user_new_clicked (callback_data, NULL);
}

void
on_popup_settings_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	if (GTK_WIDGET (callback_data) == groups_table)
		on_group_settings_clicked (callback_data, NULL);
	else if (GTK_WIDGET (callback_data) == users_table)
		on_user_settings_clicked (callback_data, NULL);
}

void
on_popup_delete_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	if (GTK_WIDGET (callback_data) == groups_table)
		on_group_delete_clicked (callback_data, NULL);
	else if (GTK_WIDGET (callback_data) == users_table)
		on_user_delete_clicked (callback_data, NULL);
}

#ifdef NIS
/* Network tab. */

void
on_network_delete_clicked (GtkWidget *button, gpointer user_data)
{
	xmlNodePtr node;
	gboolean delete = FALSE;
	gint tbl = -1;

	g_return_if_fail (gst_tool_get_access (tool));

	node = get_selected_node ();

	/* Is it user or group? */

	if (!strcmp (node->name, "user"))
	{
		delete = check_login_delete (node);
		tbl = NODE_NET_USER;
	}
	else if (!strcmp (node->name, "group"))
	{
		delete = check_group_delete (node);
		tbl = NODE_NET_GROUP;
	}

	if (delete)
	{
		gst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (tbl))
			gst_xml_element_destroy (node);

		actions_set_sensitive (NODE_GROUP, FALSE);
	}
}

void
on_network_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (gst_tool_get_access (tool));

	user_settings_prepare (get_root_node (NODE_NET_USER));
}

void
on_network_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	g_return_if_fail (gst_tool_get_access (tool));

	ud = g_new (ug_data, 1);

	ud->new = TRUE;
	ud->table = NODE_NET_GROUP;
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
	gtk_widget_grab_focus (gst_dialog_get_widget (tool->main_dialog, "user_settings_name"));
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

	g_return_if_fail (gst_tool_get_access (tool));

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
	ud = g_object_get_data (G_OBJECT (widget), "data");

	if (user_update (ud))
	{
		gst_dialog_modify (tool->main_dialog);
		user_settings_dialog_close ();
	}
}

void
on_user_settings_passwd_random_new (GtkButton *button, gpointer data)
{
	gchar *passwd;

	passwd = passwd_get_random ();
	gtk_entry_set_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "user_passwd_random_entry")), passwd);
	g_free (passwd);
}

void
on_user_settings_passwd_toggled (GtkToggleButton *toggle, gpointer data)
{
	GtkWidget *user_passwd_random_new = gst_dialog_get_widget (tool->main_dialog, "user_passwd_random_new");
	GtkWidget *user_passwd_random_entry = gst_dialog_get_widget (tool->main_dialog, "user_passwd_random_entry");
	GtkWidget *user_passwd_entry1 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry1");
	GtkWidget *user_passwd_entry2 = gst_dialog_get_widget (tool->main_dialog, "user_passwd_entry2");
	GtkToggleButton *pwd_manual = GTK_TOGGLE_BUTTON (gst_dialog_get_widget (tool->main_dialog, "user_passwd_manual"));
	

	if (gtk_toggle_button_get_active (pwd_manual)) {
		gtk_widget_set_sensitive (user_passwd_random_new, FALSE);
		gtk_widget_set_sensitive (user_passwd_random_entry, FALSE);
		gtk_widget_set_sensitive (user_passwd_entry1, TRUE);
		gtk_widget_set_sensitive (user_passwd_entry2, TRUE);
		gtk_entry_set_text (GTK_ENTRY (user_passwd_random_entry), "");
	} else {
		gtk_widget_set_sensitive (user_passwd_random_new, TRUE);
		gtk_widget_set_sensitive (user_passwd_random_entry, TRUE);
		gtk_widget_set_sensitive (user_passwd_entry1, FALSE);
		gtk_widget_set_sensitive (user_passwd_entry2, FALSE);
		gtk_entry_set_text (GTK_ENTRY (user_passwd_entry1), "");
		gtk_entry_set_text (GTK_ENTRY (user_passwd_entry2), "");
		on_user_settings_passwd_random_new (NULL, NULL);
	}
}

void
on_user_settings_profile_changed (GtkWidget *widget, gpointer data)
{
	xmlNodePtr profile = data;

	user_set_profile (profile);
}

/* Group settings callbacks */

void
on_group_settings_dialog_show (GtkWidget *widget, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (gst_dialog_get_widget (tool->main_dialog, "group_settings_name"));
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

	g_return_if_fail (gst_tool_get_access (tool));

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	gd = g_object_get_data (G_OBJECT (widget), "data");

	if (group_update (gd))
	{
		gst_dialog_modify (tool->main_dialog);
		group_settings_dialog_close ();
	}
}

/* Profile settings dialog callbacks */
void
on_profile_settings_dialog_delete_event (GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "profile_settings_dialog");
	xmlNodePtr node = g_object_get_data (G_OBJECT (dialog), "data");

	if (node != NULL) {
		/* we are in settings mode, we have to unref the data */
		g_object_steal_data (G_OBJECT (dialog), "data");
	}
	
	profile_settings_clear_dialog ();
	gtk_widget_hide (dialog);
}

void
on_profile_settings_ok_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "profile_settings_dialog");
	xmlNodePtr node = g_object_get_data (G_OBJECT (dialog), "data");
	gchar *error = profile_settings_check ();

	if (error != NULL) {
		/* the profile cannot be added/modified, show the error and exit */
		show_error_message ("profile_settings_dialog", error);
	} else {
		/* the profile is OK, we can add/modify it */
		if (node == NULL) {
			/* it's a new profile, we have to create a node before saving changes */
			node = gst_xml_element_add (get_root_node (NODE_PROFILE), "profile");
		}

		profile_settings_save_data (node);

		profile_settings_clear_dialog ();
		gtk_widget_hide (dialog);

		profiles_table_update_content ();

		gst_dialog_modify (tool->main_dialog);
	}
}

/* Profile settings callbacks */

void
on_profile_new_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "profile_settings_dialog");
	GtkWidget *groups_option_menu = gst_dialog_get_widget (tool->main_dialog, "profile_settings_group");
	GtkWidget *shells_combo = gst_dialog_get_widget (tool->main_dialog, "profile_settings_shell");

	combo_add_shells (shells_combo);
	option_menu_add_groups (groups_option_menu, TRUE);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Create New profile"));

	gtk_widget_show (dialog);
}


void
on_profile_settings_clicked (GtkButton *button, gpointer data)
{
	GtkWidget *groups_option_menu = gst_dialog_get_widget (tool->main_dialog, "profile_settings_group");
	GtkWidget *shells_combo = gst_dialog_get_widget (tool->main_dialog, "profile_settings_shell");
	GtkWidget *dialog =gst_dialog_get_widget (tool->main_dialog, "profile_settings_dialog");
	GtkWidget *profiles_table = gst_dialog_get_widget (tool->main_dialog, "profiles_table");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (profiles_table));
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	xmlNodePtr node;
	gchar *profile_name, *window_title;

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (profiles_table), &path, NULL);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, COL_PROFILE_POINTER, &node, -1);

	combo_add_shells (shells_combo);
	option_menu_add_groups (groups_option_menu, TRUE);
	profile_settings_set_data (node);

	g_object_set_data (G_OBJECT (dialog), "data", node);

	profile_name = gst_xml_get_child_content (node, "name");
	window_title = g_strdup_printf (_("Settings for profile %s"), profile_name);
	gtk_window_set_title (GTK_WINDOW (dialog), window_title);
	g_free (profile_name);
	g_free (window_title);

	gtk_widget_show (dialog);
}

static void
on_profile_delete_clicked_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	xmlNodePtr node;

	g_return_if_fail (gst_tool_get_access (tool));
	
	gtk_tree_model_get (model, iter, COL_PROFILE_POINTER, &node, -1);

	if ((profile_delete (node)) == TRUE)
		* (gboolean *) data = TRUE;
}

void
on_profile_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *profiles_table = gst_dialog_get_widget (tool->main_dialog, "profiles_table");
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profiles_table));
	gboolean deleted = FALSE;
	
	gtk_tree_selection_selected_foreach (selection, on_profile_delete_clicked_foreach, &deleted);
	
	/* At this point changed will be TRUE if any of the selected users has been deleted,
	 * the apply button must be set sensitive and the table must be updated */
	if (deleted == TRUE) {
		gst_dialog_modify (tool->main_dialog);
		profiles_table_update_content ();
	}

	gtk_tree_selection_unselect_all (selection);
	actions_set_sensitive (NODE_PROFILE, FALSE);
}

/* general callbacks */

void
on_add_remove_button_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeView *in, *out;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList *in_list, *out_list;
	GList *element;
	
	g_return_if_fail (gst_tool_get_access (tool));
	
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
	
	widget = gst_dialog_get_widget (tool->main_dialog, button);
	gtk_widget_set_sensitive (widget, TRUE);
}

/* Helpers .*/

void
actions_set_sensitive (gint table, gboolean state)
{
	switch (table)
	{
	case NODE_USER:
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_new",      TRUE);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_delete",   state);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_settings", state);
		break;
	case NODE_GROUP:
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_new",      TRUE);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_delete",   state);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_settings", state);
		break;
/*	case NODE_NET_USER:
	case NODE_NET_GROUP:
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_group_new", TRUE);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_user_new",  TRUE);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_delete",    state);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_settings",  state);
		break;*/
	case NODE_PROFILE:
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "profile_new",      TRUE);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "profile_settings", state);
		gst_dialog_widget_set_user_sensitive (tool->main_dialog, "profile_delete",   state);
		break;
	default:
		g_warning ("actions_set_sensitive: Shouldn't be here.");
		return;
	}
}
