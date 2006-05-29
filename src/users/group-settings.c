/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* group_settings.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */

#include <config.h>
#include "gst.h"
#include <glib/gi18n.h>

#include <string.h>

#include "groups-table.h"
#include "table.h"
#include "callbacks.h"
#include "group-settings.h"
#include "test-battery.h"

extern GstTool *tool;

static gboolean
check_group_delete (OobsGroup *group)
{
	GtkWidget *parent;
	GtkWidget *dialog;
	gint reply;

	parent = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");

	if (oobs_group_get_gid (group) == 0) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Administrator group can not be deleted"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("This would leave the system unusable."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return FALSE;
	}

	/* FIXME: should check that any user of this group is logged in */

	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Are you sure you want to delete group \"%s\"?"),
					 oobs_group_get_name (group));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("This may leave files with invalid group ID in the filesystem."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);

	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return (reply == GTK_RESPONSE_ACCEPT);
}

gboolean
group_delete (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	OobsGroupsConfig *config;
	OobsGroup *group;
	OobsList *groups_list;
	OobsListIter *list_iter;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return FALSE;

	gtk_tree_model_get (model, &iter,
			    COL_GROUP_OBJECT, &group,
			    COL_GROUP_ITER, &list_iter,
			    -1);

	if (check_group_delete (group)) {
		config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
		groups_list = oobs_groups_config_get_groups (config);
		oobs_list_remove (groups_list, list_iter);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

		return TRUE;
	}

	return FALSE;

}

gid_t
group_settings_find_new_gid (void)
{
	OobsGroupsConfig *config;
	OobsList *list;
	OobsListIter list_iter;
	GObject *group;
	gboolean valid;
	gid_t new_gid, gid, gid_min, gid_max;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	list = oobs_groups_config_get_groups (config);
	valid = oobs_list_get_iter_first (list, &list_iter);

	gid_min = GST_USERS_TOOL (tool)->minimum_gid;
	gid_max = GST_USERS_TOOL (tool)->maximum_gid;

	new_gid = gid_min - 1;

	while (valid) {
		group = oobs_list_get (list, &list_iter);
		gid = oobs_group_get_gid (OOBS_GROUP (group));
		g_object_unref (group);

		if (gid <= gid_max && gid >= gid_min && new_gid < gid)
			new_gid = gid;

		valid = oobs_list_iter_next (list, &list_iter);
	}

	new_gid++;

	return new_gid;
}

static void
set_entry_text (GtkWidget *entry, const gchar *text)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (text) ? text : "");
}

GtkWidget*
group_settings_dialog_new (OobsGroup *group)
{
	GtkWidget *dialog, *widget;
	const gchar *name;
	gchar *title;

	dialog = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	name = oobs_group_get_name (group);

	if (!name) {
		g_object_set_data (G_OBJECT (dialog), "is_new", GINT_TO_POINTER (FALSE));
		gtk_window_set_title (GTK_WINDOW (dialog), _("New group"));

		widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
					   group_settings_find_new_gid ());
	} else {
		g_object_set_data (G_OBJECT (dialog), "is_new", GINT_TO_POINTER (FALSE));

		title = g_strdup_printf (_("Group '%s' Properties"), name);
		gtk_window_set_title (GTK_WINDOW (dialog), title);
		g_free (title);

		widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), oobs_group_get_gid (group));
	}

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	set_entry_text (widget, name);

	group_members_table_set_from_group (group);

	return dialog;
}

gboolean
group_settings_dialog_group_is_new (void)
{
	GtkWidget *dialog;
	gboolean is_new;

	dialog = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	is_new = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "is_new"));

	return is_new;
}

static gboolean
is_group_root (OobsGroup *group)
{
	const gchar *name = oobs_group_get_name (group);

	if (!name)
		return FALSE;

	return (strcmp (name, "root") == 0);
}

static gboolean
group_exists (const gchar *name)
{
	OobsGroupsConfig *config;
	OobsList *groups_list;
	OobsListIter iter;
	GObject *group;
	gboolean valid;
	const gchar *group_name;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	groups_list = oobs_groups_config_get_groups (config);
	valid = oobs_list_get_iter_first (groups_list, &iter);

	while (valid) {
		group = oobs_list_get (groups_list, &iter);
		group_name = oobs_group_get_name (OOBS_GROUP (group));
		g_object_unref (group);

		if (group_name && strcmp (name, group_name) == 0)
			return TRUE;

		valid = oobs_list_iter_next (groups_list, &iter);
	}

	return FALSE;
}

/* FIXME: this function is duplicated in user-settings.c */
static gboolean
is_valid_name (const gchar *name)
{
	/*
	 * User/group names must start with a letter, and may not
	 * contain colons, commas, newlines (used in passwd/group
	 * files...) or any non-printable characters.
	 */
        if (!*name || !isalpha(*name))
                return FALSE;

        while (*name) {
		if (!isdigit (*name) && !islower (*name) && *name != '-')
                        return FALSE;
                name++;
        }

        return TRUE;
}

static void
check_name (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsGroup *group = OOBS_GROUP (data);
	GtkWidget *widget;
	const gchar *name;

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	name = gtk_entry_get_text (GTK_ENTRY (widget));

	if (strlen (name) < 1) {
		*primary_text = g_strdup (_("Group name is empty"));
		*secondary_text = g_strdup (_("A group name must be specified."));
	} else if (is_group_root (group) && strcmp (name, "root") != 0) {
		*primary_text = g_strdup (_("Group name of the administrator group user should not be modified"));
		*secondary_text = g_strdup (_("This would leave the system unusable."));
	} else if (!is_valid_name (name)) {
		*primary_text = g_strdup (_("Group name has invalid characters"));
		*secondary_text = g_strdup (_("Please set a valid group name consisting of "
					      "a lower case letter followed by lower case "
					      "letters and numbers."));
	} else if (group_settings_dialog_group_is_new () && group_exists (name)) {
		*primary_text = g_strdup_printf (_("Group \"%s\" already exists"), name);
		*secondary_text = g_strdup (_("Please select a different user name."));
	}
}

static void
check_gid (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsGroup *group = OOBS_GROUP (data);
	GtkWidget *widget;
	gint gid;

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	gid = gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));

	if (is_group_root (group) && gid != 0) {
		*primary_text = g_strdup (_("Group ID of the Administrator account should not be modified"));
		*secondary_text = g_strdup (_("This would leave the system unusable."));
	}
}

gint
group_settings_dialog_run (GtkWidget *dialog, OobsGroup *group)
{
	gint response;
	gboolean valid;

	TestBattery battery[] = {
		check_name,
		check_gid,
		NULL
	};

	do {
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		valid = (response == GTK_RESPONSE_OK) ?
			test_battery_run (battery, GTK_WINDOW (dialog), group) : TRUE;
	} while (!valid);

	gtk_widget_hide (dialog);
	return response;
}

void
group_settings_dialog_get_data (OobsGroup *group)
{
	GtkWidget *widget;

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	oobs_group_set_name (group, gtk_entry_get_text (GTK_ENTRY (widget)));

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	oobs_group_set_gid (group, gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));

	group_members_table_save (group);
}
