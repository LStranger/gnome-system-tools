/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
#include "groups-table.h"
#include "privileges-table.h"
#include "group-members-table.h"
#include "callbacks.h"

extern GstTool *tool;

GtkActionEntry popup_menu_items [] = {
	{ "Add",         GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_popup_add_activate)      },
	{ "Properties",  GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_popup_settings_activate) },
	{ "Delete",      GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_popup_delete_activate)   }
};

const gchar *ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Add'/>"
	"    <separator/>"
	"    <menuitem action='Properties'/>"
	"    <menuitem action='Delete'/>"
	"  </popup>"
	"</ui>";

GtkWidget*
popup_menu_create (GtkWidget *widget, gint table)
{
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
	GtkWidget      *popup;

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, popup_menu_items,
				      G_N_ELEMENTS (popup_menu_items),
				      GINT_TO_POINTER (table));

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
		return NULL;

	g_object_set_data (G_OBJECT (widget), "ui-manager", ui_manager);
	popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

	return popup;
}

static void
setup_groups_combo (void)
{
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	GtkWidget *table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
}

static void
setup_shells_combo (GstUsersTool *tool)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *shells;

	combo = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_settings_shell");
	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	shells = oobs_users_config_get_available_shells (OOBS_USERS_CONFIG (tool->users_config));

	while (shells) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, shells->data,
				    -1);
		shells = shells->next;
	}

	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
	g_object_unref (model);

	gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (combo), 0);
}

void
table_populate_profiles (GstUsersTool *tool,
			 GList        *names)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;

	combo = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_settings_profile_menu");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	while (names) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, names->data, -1);
		names = names->next;
	}
}

void
table_set_default_profile (GstUsersTool *tool)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GstUserProfile *default_profile;
	gchar *profile;
	gboolean valid;

	default_profile = gst_user_profiles_get_default_profile (tool->profiles);
	combo = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_settings_profile_menu");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &profile, -1);

		if (default_profile &&
		    strcmp (profile, default_profile->name) == 0) {
			g_signal_handlers_block_by_func (G_OBJECT (combo), on_user_settings_profile_changed, GST_TOOL (tool)->main_dialog);
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			g_signal_handlers_unblock_by_func (G_OBJECT (combo), on_user_settings_profile_changed, GST_TOOL (tool)->main_dialog);
			valid = FALSE;
		} else
			valid = gtk_tree_model_iter_next (model, &iter);

		g_free (profile);
	}

	user_settings_apply_profile (tool, default_profile);
}

void
create_tables (GstUsersTool *tool)
{
	create_users_table (tool);
	create_groups_table ();
	create_user_privileges_table ();
	create_group_members_table ();

	/* not strictly tables, but uses a model */
	setup_groups_combo ();
	setup_shells_combo (tool);
}

static GtkWidget*
get_table (gint table)
{
	switch (table) {
	case TABLE_USERS:
		return gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	case TABLE_GROUPS:
		return gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "groups_table");
	default:
		g_assert_not_reached ();
	}
}

GList*
table_get_row_references (gint table, GtkTreeModel **model)
{
	GtkTreeSelection *selection;
	GtkTreeModel *filter_model;
	GtkTreePath *child_path;
	GList *paths, *elem, *list = NULL;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (get_table (table)));
	paths = elem = gtk_tree_selection_get_selected_rows (selection, &filter_model);
	*model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

	if (!paths)
		return NULL;

	while (elem) {
		child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (filter_model),
									       (GtkTreePath *) elem->data);

		list = g_list_prepend (list, gtk_tree_row_reference_new (*model, child_path));
		gtk_tree_path_free (child_path);
		elem = elem->next;
	}

	list = g_list_reverse (list);
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	return list;
}
