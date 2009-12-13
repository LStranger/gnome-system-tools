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

#include <config.h>
#include "gst.h"

#include "passwd.h"
#include "callbacks.h"
#include "table.h"
#include "user-settings.h"
#include "users-table.h"
#include "users-tool.h"
#include "group-settings.h"
#include "groups-table.h"

extern GstTool *tool;


/* Common stuff to users and groups tables */

static void
actions_set_sensitive (gint table, gint count)
{
	switch (table) {
	case TABLE_USERS:
		gst_dialog_try_set_sensitive (tool->main_dialog,
					      gst_dialog_get_widget (tool->main_dialog, "user_new"),
					      TRUE);
		gst_dialog_try_set_sensitive (tool->main_dialog,
					      gst_dialog_get_widget (tool->main_dialog, "user_delete"),
					      (count > 0));
		break;
	case TABLE_GROUPS:
		gst_dialog_try_set_sensitive (tool->main_dialog,
					      gst_dialog_get_widget (tool->main_dialog, "group_new"),
					      TRUE);
		gst_dialog_try_set_sensitive (tool->main_dialog,
					      gst_dialog_get_widget (tool->main_dialog, "group_delete"),
					      (count > 0));
		gst_dialog_try_set_sensitive (tool->main_dialog,
					      gst_dialog_get_widget (tool->main_dialog, "group_settings"),
					      (count == 1));
		break;
	default:
		g_assert_not_reached ();
		return;
	}
}

void
on_table_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	gint count, table;
	OobsUser *user = NULL;

	table = GPOINTER_TO_INT (data);
	count = gtk_tree_selection_count_selected_rows (selection);

	if (table == TABLE_USERS) {
		user = users_table_get_current ();
	}

	/* this happens when we unselect all users before selecting a single one */
	if (!user)
		return;

	actions_set_sensitive (table, count);

	/* Show the settings for the selected user */
	user_settings_show (user);

	g_object_unref (user);
}

static void
do_popup_menu (GtkTreeView *treeview, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	GtkUIManager *ui_manager;
	gint cont, button, event_time;
	GtkWidget *popup;

	popup = g_object_get_data (G_OBJECT (treeview), "popup");

	if (event) {
		button     = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	selection = gtk_tree_view_get_selection (treeview);
	cont = gtk_tree_selection_count_selected_rows (selection);
	ui_manager = g_object_get_data (G_OBJECT (treeview), "ui-manager");

	gtk_widget_set_sensitive (gtk_ui_manager_get_widget (ui_manager, "/MainMenu/Add"),
				  cont == 1);
	gtk_widget_set_sensitive (gtk_ui_manager_get_widget (ui_manager, "/MainMenu/Properties"),
				  cont == 1);
	gtk_widget_set_sensitive (gtk_ui_manager_get_widget (ui_manager, "/MainMenu/Delete"),
				  cont > 0);

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

gboolean
on_table_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer data)
{
	GtkTreePath *path;
	GtkTreeSelection *selection;
	gint cont, table;

	table = GPOINTER_TO_INT (data);
	selection = gtk_tree_view_get_selection (treeview);
	cont = gtk_tree_selection_count_selected_rows (selection);

	if (event->window != gtk_tree_view_get_bin_window (treeview))
		return FALSE;

	if (event->button == 3)	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
			if (cont < 1) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			}

			gtk_tree_path_free (path);

			do_popup_menu (treeview, event);
		}

		return TRUE;
	}

	if (cont != 0 && (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
	    && (table == TABLE_GROUPS)
	    && gst_dialog_is_authenticated (tool->main_dialog))
		on_group_settings_clicked (NULL, NULL);

	return FALSE;
}

gboolean
on_table_popup_menu (GtkTreeView *treeview, GtkWidget *popup)
{
	do_popup_menu (treeview, NULL);
	return TRUE;
}

void
on_popup_add_activate (GtkAction *action, gpointer data)
{
	gint table = GPOINTER_TO_INT (data);

	if (table == TABLE_GROUPS)
		on_group_new_clicked (NULL, NULL);
	else if (table == TABLE_USERS)
		on_user_new (NULL, NULL);
}

void
on_popup_settings_activate (GtkAction *action, gpointer data)
{
	gint table = GPOINTER_TO_INT (data);

	if (table == TABLE_GROUPS)
		on_group_settings_clicked (NULL, NULL);
	else if (table == TABLE_USERS)
		on_user_settings_clicked (NULL, NULL);
}

void
on_popup_delete_activate (GtkAction *action, gpointer data)
{
	gint table = GPOINTER_TO_INT (data);

	if (table == TABLE_GROUPS)
		on_group_delete_clicked (NULL, NULL);
	else if (table == TABLE_USERS)
		on_user_delete_clicked (NULL, NULL);
}

/* Users Tab */

void
on_user_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *table;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsUser *user;
	OobsListIter *list_iter;

	table = gst_dialog_get_widget (tool->main_dialog, "users_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (table), &path, NULL);

	if (!path)
		return;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;

	gtk_tree_model_get (model, &iter,
			    COL_USER_OBJECT, &user,
			    COL_USER_ITER, &list_iter,
			    -1);
#if 0 /* FIXME: adapt this code to work when applying changes */
	dialog = user_settings_dialog_new (user);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	response = user_settings_dialog_run (dialog);
	g_object_unref (user);

	if (response == GTK_RESPONSE_OK) {
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model),
								  &filter_iter, &iter);
		user = user_settings_dialog_get_data (dialog);

		if (!user) /* Means an error has already occurred and been displayed, stop here */
			return;

		if (gst_dialog_is_authenticated (tool->main_dialog)) {
			/* change users/groups configuration */
			if (gst_tool_commit (tool, GST_USERS_TOOL (tool)->users_config) == OOBS_RESULT_OK) {
				gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);
				users_table_set_user (user, list_iter, &filter_iter);
			}
#ifdef HAVE_POLKIT
		/* With PolicyKit1, we don't have to check for authorizations: just try to commit,
		 * the backend will trigger authentication if possible, or fail */
		} else {
			OobsObject *object = GST_USERS_TOOL (tool)->self_config;

			/* change self, only if it is the modified user */
			if (user == oobs_self_config_get_user (OOBS_SELF_CONFIG (object)) &&
			    gst_tool_commit (tool, GST_USERS_TOOL (tool)->self_config) == OOBS_RESULT_OK) {
				gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);
				users_table_set_user (user, list_iter, &filter_iter);
			}
#endif
		}
	}
#endif

	oobs_list_iter_free (list_iter);
}

void
on_manage_groups_clicked (GtkWidget *widget, gpointer user_data)
{
	GtkWidget *dialog;

	dialog = gst_dialog_get_widget (tool->main_dialog, "groups_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_HELP);
	gtk_widget_hide (dialog);
}

/* Groups tab */

void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	GtkWidget *parent_dialog;
	OobsGroupsConfig *config;
	OobsGroup *group;
	OobsList *groups_list;
	OobsListIter list_iter;
	gint response;

	group = oobs_group_new (NULL);
	parent_dialog = gst_dialog_get_widget (tool->main_dialog, "groups_dialog");
	dialog = group_settings_dialog_new (group);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_dialog));
	response = group_settings_dialog_run (dialog, group);

	if (response == GTK_RESPONSE_OK) {
		group = group_settings_dialog_get_group ();

		config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
		groups_list = oobs_groups_config_get_groups (config);
		oobs_list_append (groups_list, &list_iter);
		oobs_list_set (groups_list, &list_iter, group);

		if (gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config) == OOBS_RESULT_OK)
			groups_table_add_group (group, &list_iter);
	}
}

void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *table, *dialog, *parent_dialog;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter filter_iter, iter;
	OobsGroup *group;
	OobsListIter *list_iter;
	gint response;

	table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (table), &path, NULL);

	if (!path)
		return;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;

	gtk_tree_model_get (model, &iter,
			    COL_GROUP_OBJECT, &group,
			    COL_GROUP_ITER, &list_iter,
			    -1);
	dialog = group_settings_dialog_new (group);
	parent_dialog = gst_dialog_get_widget (tool->main_dialog, "groups_dialog");

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_dialog));
	response = group_settings_dialog_run (dialog, group);

	if (response == GTK_RESPONSE_OK) {
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model),
								  &filter_iter, &iter);
		group_settings_dialog_get_data (group);
		groups_table_set_group (group, list_iter, &filter_iter);
		gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);
	}

	g_object_unref (group);
	oobs_list_iter_free (list_iter);
}

void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GList *list, *elem;

	list = elem = table_get_row_references (TABLE_GROUPS, &model);

	while (elem) {
		path = gtk_tree_row_reference_get_path (elem->data);
		group_delete (model, path);

		gtk_tree_path_free (path);
		elem = elem->next;
	}

	g_list_foreach (list, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (list);

	gst_tool_commit (tool, GST_USERS_TOOL (tool)->groups_config);
}

/* User settings callbacks */

void
on_user_settings_passwd_changed (GtkEntry *entry, gpointer data)
{
	g_object_set_data (G_OBJECT (entry), "changed", GINT_TO_POINTER (TRUE));
}

void
on_user_settings_passwd_random_new (GtkButton *button, gpointer data)
{
	GtkWidget *widget;
	gchar *passwd;

	widget = gst_dialog_get_widget (tool->main_dialog, "user_settings_random_passwd");

	passwd = passwd_get_random ();
	gtk_entry_set_text (GTK_ENTRY (widget), passwd);
	g_free (passwd);
}

void
on_user_settings_passwd_toggled (GtkToggleButton *toggle, gpointer data)
{
	GtkWidget *user_passwd_random_new = gst_dialog_get_widget (tool->main_dialog, "user_passwd_random_new");
	GtkWidget *user_passwd_random_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_random_passwd");
	GtkWidget *user_passwd_entry1 = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd1");
	GtkWidget *user_passwd_entry2 = gst_dialog_get_widget (tool->main_dialog, "user_settings_passwd2");
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
on_user_settings_login_changed (GtkEditable *editable,
				gpointer     data)
{
	gchar *home, *base_dir;
	GtkWidget *home_entry;

	home_entry = gst_dialog_get_widget (tool->main_dialog, "user_settings_home");
	base_dir = g_object_get_data (G_OBJECT (home_entry), "default-home");

	if (!base_dir)
		g_object_get (GST_USERS_TOOL (tool)->users_config,
			      "default-home", &base_dir, NULL);

	home = g_build_path (G_DIR_SEPARATOR_S,
			     base_dir,
			     gtk_entry_get_text (GTK_ENTRY (editable)),
			     NULL);

	gtk_entry_set_text (GTK_ENTRY (home_entry), home);
	g_free (home);
}

void
on_groups_dialog_show_help (GtkWidget *widget, gpointer data)
{
	GstDialog *dialog = GST_DIALOG (data);
	GstTool *tool = gst_dialog_get_tool (dialog);

	gst_tool_show_help (tool, NULL);
}
