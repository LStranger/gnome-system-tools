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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "gst.h"
#include "gst-hig-dialog.h"
#include "groups-table.h"
#include "table.h"
#include "callbacks.h"
#include "user-group-xml.h"
#include "user_group.h"
#include "group-settings.h"

extern GstTool *tool;

static void
create_users_gtk_trees (void)
{
	GtkWidget *group_settings_all = gst_dialog_get_widget (tool->main_dialog, "group_settings_all");
	GtkWidget *group_settings_members = gst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	GtkWidget *add_button, *remove_button;
	GtkTreeSelection *selection;
	GtkSizeGroup *sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	GtkTargetEntry target = { "user", GTK_TARGET_SAME_APP, 1 };
	
	/* We create the widgets, connect signals and attach data if they haven't been created already */
	if (g_object_get_data (G_OBJECT (group_settings_all), "button") == NULL) {
		create_gtk_tree_list (group_settings_all, target);
		g_object_set_data (G_OBJECT (group_settings_all), "button", "group_settings_add");

		create_gtk_tree_list (group_settings_members, target);
		g_object_set_data (G_OBJECT (group_settings_members), "button", "group_settings_remove");

		/* We also need to attach some data to the 'add' and 'remove' buttons */
		add_button = gst_dialog_get_widget (tool->main_dialog, "group_settings_add");
		g_object_set_data (G_OBJECT (add_button), "in", group_settings_all);
		g_object_set_data (G_OBJECT (add_button), "out", group_settings_members);
		
		remove_button = gst_dialog_get_widget (tool->main_dialog, "group_settings_remove");
		g_object_set_data (G_OBJECT (remove_button), "in", group_settings_members);
		g_object_set_data (G_OBJECT (remove_button), "out", group_settings_all);

		/* Add the scrolled windows that contain the trees in the same GtkSizeGroup */
		gtk_size_group_add_widget (sizegroup, gst_dialog_get_widget (tool->main_dialog, "group_settings_all_container"));
		gtk_size_group_add_widget (sizegroup, gst_dialog_get_widget (tool->main_dialog, "group_settings_members_container"));
		
	}
}

void
group_new_prepare (ug_data *gd)
{
	GtkWidget *group_settings_all = gst_dialog_get_widget (tool->main_dialog, "group_settings_all");
	GtkWidget *group_settings_members = gst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	GtkWidget *widget;
	gchar     *buf;
	GList *group_settings_all_list = NULL;
	
	/* Fill all users list, don't exclude anything */
	create_users_gtk_trees ();
	group_settings_all_list = get_list_from_node ("login", NODE_USER);
	populate_gtk_tree_list (GTK_TREE_VIEW (group_settings_all), group_settings_all_list);
	
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (widget), _("Create New Group"));

	/* Fill in first available gid */
	buf = (gchar *) find_new_id (gd->node, NULL);
	if (buf) {
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "group_settings_gid")), 
		                           g_strtod (buf, NULL));
		g_free (buf);
	}

	g_object_set_data (G_OBJECT (widget), "data", gd);
	gtk_widget_show (widget);
}

void
group_settings_dialog_close (void)
{
	GtkWidget *group_settings_all = gst_dialog_get_widget (tool->main_dialog, "group_settings_all");
	GtkWidget *group_settings_members = gst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	GtkWidget *widget;
	GtkTreeModel *model;
	ug_data *gd;

	/* Clear group name */
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (widget), "");

	/* Clear both lists. */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (group_settings_all));
	gtk_tree_store_clear (GTK_TREE_STORE (model));

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (group_settings_members));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	/* Set unsensitive the add and remove buttons */
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_add");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_remove");
	gtk_widget_set_sensitive (widget, FALSE);
	
	/* Clear group data attached to the dialog */
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	gd = g_object_get_data (G_OBJECT (widget), "data");
	g_free (gd);
	g_object_steal_data (G_OBJECT (widget), "data");
	gtk_widget_hide (widget);
}

static gboolean
is_group_root (xmlNodePtr node)
{
	gchar *group;

	if (strcmp (node->name, "groupdb") == 0) {
		/* node is set to userdb node, this means that the user is new, thus cannot be root */
		return FALSE;
	} else {
		/* node isn't set to userdb node, this means that the user is being modified */
		group = gst_xml_get_child_content (node, "name");
	
		if (strcmp (group, "root") == 0)
			return TRUE;
	}
	return FALSE;
}

static gboolean
is_group_gid_valid (xmlNodePtr group_node, const gchar* gid)
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;

	if (!is_valid_id (gid)) {
		primary_text   = g_strdup (_("Invalid group ID"));
		secondary_text = g_strdup (_("Group ID must be a positive number"));
	} else if (is_group_root (group_node) && (strcmp (gid, "0") != 0)) {
		/* if group is root */
		primary_text   = g_strdup (_("Group ID of the \"root\" user should not be modified"));
		secondary_text = g_strdup (_("This would leave the system unusable"));
	} else if (node_exists (group_node, "gid", gid)) {
		/* Check if gid is available */
		primary_text   = g_strdup (_("Invalid group ID"));
		secondary_text = g_strdup_printf (_("Group ID %s already exists"), gid);
	}
	
	if (primary_text) {
		show_error_message ("group_settings_dialog", primary_text, secondary_text);
		g_free (primary_text);
		g_free (secondary_text);
		
		return FALSE;
	}

	return TRUE;
}

static gboolean
is_group_name_valid (xmlNodePtr node, const gchar *name)
{
	gchar *primary_text   = NULL;
	gchar *secondary_text = NULL;
	
	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	if (strlen (name) < 1) {
		/* If empty. */
		primary_text   = g_strdup (_("Group name is empty"));
		secondary_text = g_strdup (_("A group name must be specified"));
	} else if (strlen (name) > 16) {
		/* If too long. */
		primary_text   = g_strdup (_("The group name is too long"));
		secondary_text = g_strdup_printf (_("The group name should have less than %i characters for being valid"), 16);
	} else if (!is_valid_name (name)) {
		/* if invalid. */
		primary_text   = g_strdup (_("Group name has invalid characters"));
		secondary_text = g_strdup (_("Please set a valid group name, using only lower case letters"));
	} else if (node_exists (node, "name", name)) {
		/* if !exist. */
		primary_text   = g_strdup_printf (_("Group \"%s\" already exists"), name);
		secondary_text = g_strdup (_("Please select a different group name"));
	} else if (is_group_root (node) && (strcmp (name, "root") != 0)) {
		/* if it's the root group */
		primary_text   = g_strdup (_("Group name of the \"root\" user should not be modified"));
		secondary_text = g_strdup (_("This would leave the system unusable"));
	}

	/* If anything is wrong. */
	if (primary_text) {
		show_error_message ("group_settings_dialog", primary_text, secondary_text);
		g_free (primary_text);
		g_free (secondary_text);

		return FALSE;
	} else {
		return TRUE;
	}
}

gboolean
group_update (ug_data *gd)
{
	GtkWidget *group_settings_members = gst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	gchar *name, *gid;
	GList *users;
	
	name = (gchar *) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "group_settings_name")));
	if (!is_group_name_valid (gd->node, name)) {
		return FALSE;
	}
	
	gid = g_strdup_printf ("%i", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (gst_dialog_get_widget (tool->main_dialog, "group_settings_gid"))));
	if (!is_group_gid_valid (gd->node, gid)) {
		return FALSE;
	}

	users = get_gtk_tree_list_items (GTK_TREE_VIEW (group_settings_members));

	if (gd->is_new) {
		/* Add new group, update table. */
		xmlNodePtr node;
		
		node = group_add_blank_xml (gd->node);
		group_update_xml (node, name, gid, users);
	} else {
		/* Entered data ok, not new: just update */
		group_update_xml (gd->node, name, gid, users);
	}

	groups_table_update_content ();
	
	return TRUE;
}

static gboolean
check_group_delete (xmlNodePtr node)
{
	gchar *name, *primary_text, *secondary_text;
	GtkWindow *parent;
	GtkWidget *dialog;
	gint reply;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	name = gst_xml_get_child_content (node, "name");

	if (!name) {
		g_warning ("check_group_delete: Can't get group's name");
		return FALSE;
	}

	if (strcmp (name, "root") == 0) {
		primary_text   = g_strdup (_("The group \"root\" should not be deleted"));
	        secondary_text = g_strdup (_("This would leave the system unusable"));
		show_error_message ("group_settings_dialog", primary_text, secondary_text);

		g_free (name);
		g_free (primary_text);
		g_free (secondary_text);

		return FALSE;
	}

	dialog = gst_hig_dialog_new (parent,
				     GTK_DIALOG_MODAL,
				     GST_HIG_MESSAGE_WARNING,
				     NULL,
				     _("This may leave files with invalid group ID in the filesystem"),
				     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				     GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				     NULL);
	gst_hig_dialog_set_primary_text (GST_HIG_DIALOG (dialog),
					 _("Are you sure you want to delete group \"%s\"?"), name);
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (name);
	
	if (reply == GTK_RESPONSE_ACCEPT)
		return TRUE;
        else
		return FALSE;
}

gboolean 
delete_group (xmlNodePtr node)
{
	if (check_group_delete (node)) {
		gst_xml_element_destroy (node);
		return TRUE;
	}

	return FALSE;
}

static void
group_settings_dialog_prepare (ug_data *gd)
{
	GtkWidget *group_settings_all = gst_dialog_get_widget (tool->main_dialog, "group_settings_all");
	GtkWidget *group_settings_members = gst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	GtkWidget *w0;
	gchar *txt, *name;
	GList *group_settings_all_list = NULL;
	GList *group_settings_members_list = NULL;

	g_return_if_fail (gd != NULL);
	
	create_users_gtk_trees ();
	group_settings_members_list = get_group_users (gd->node);
	group_settings_all_list = my_g_list_remove_duplicates (get_list_from_node ("login", NODE_USER),
							       group_settings_members_list);
	populate_gtk_tree_list (GTK_TREE_VIEW (group_settings_members), group_settings_members_list);
	populate_gtk_tree_list (GTK_TREE_VIEW (group_settings_all), group_settings_all_list);

	/* Set group name */
	name = gst_xml_get_child_content (gd->node, "name");
	w0 = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_widget_set_sensitive (w0, gst_tool_get_access (tool));
	gst_ui_entry_set_text (w0, name);

	/* Set group id */
	txt = gst_xml_get_child_content (gd->node, "gid");
	w0 = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	gtk_widget_set_sensitive (w0, gst_tool_get_access (tool));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (txt, NULL));

	/* Show group settings dialog */
	w0 = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	txt = g_strdup_printf (_("Settings for Group %s"), name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (name);
	g_free (txt);

	g_object_set_data (G_OBJECT (w0), "data", gd);
	gtk_widget_show (w0);
}

void
group_settings_prepare (ug_data *gd)
{
	gchar *buf;
	
	g_return_if_fail (gd != NULL);

	buf = gst_xml_get_child_content (gd->node, "login");

	if (buf)
	{
		/* Has to be some kind of user */
		g_free (buf);
		g_warning ("settings_prepare: Deprecated, shouldn't be here");
		return;
	}

	buf = gst_xml_get_child_content (gd->node, "name");

	if (buf)
	{
		/* Has to be some kind of group */
		g_free (buf);

		group_settings_dialog_prepare (gd);
		return;
	}

	g_warning ("settings_prepare: shouldn't be here");
}


