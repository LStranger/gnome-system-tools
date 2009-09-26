/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2005 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>
 */

#include <glib.h>
#include <glib/gi18n.h>
#include "callbacks.h"
#include "user-profiles.h"
#include "users-tool.h"
#include "gst.h"

static void  gst_users_tool_class_init     (GstUsersToolClass *class);
static void  gst_users_tool_init           (GstUsersTool      *tool);
static void  gst_users_tool_finalize       (GObject           *object);
static void  gst_users_tool_update_config  (GstTool *tool);

static GObject* gst_users_tool_constructor (GType                  type,
					    guint                  n_construct_properties,
					    GObjectConstructParam *construct_params);

G_DEFINE_TYPE (GstUsersTool, gst_users_tool, GST_TYPE_TOOL);

static void
gst_users_tool_class_init (GstUsersToolClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GstToolClass *tool_class = GST_TOOL_CLASS (class);

	object_class->constructor = gst_users_tool_constructor;
	object_class->finalize = gst_users_tool_finalize;
	tool_class->update_gui = gst_users_tool_update_gui;
	tool_class->update_config = gst_users_tool_update_config;
}

static void
on_showall_changed (GConfClient *client,
		    guint        conn_id,
		    GConfEntry  *entry,
		    gpointer     data)
{
	GstTool *tool = GST_TOOL (data);
	GConfValue *value;
	GtkWidget *widget;
	GtkTreeModel *model;

	value = gconf_entry_get_value (entry);
	GST_USERS_TOOL (tool)->showall = gconf_value_get_bool (value);

	widget = gst_dialog_get_widget (tool->main_dialog, "users_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
	
	widget = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
}

static void
gst_users_tool_init (GstUsersTool *tool)
{
	tool->users_config = oobs_users_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->users_config);

	tool->groups_config = oobs_groups_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->groups_config);

	tool->self_config = oobs_self_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->self_config);

	tool->profiles = gst_user_profiles_get ();
}

static GObject*
gst_users_tool_constructor (GType                  type,
			    guint                  n_construct_properties,
			    GObjectConstructParam *construct_params)
{
	GObject *object;
	GstTool *tool;

	object = (* G_OBJECT_CLASS (gst_users_tool_parent_class)->constructor) (type,
										n_construct_properties,
										construct_params);

	tool = GST_TOOL (object);
	GST_USERS_TOOL (tool)->showall = gst_conf_get_boolean (GST_TOOL (tool), "showall");

	gst_conf_add_notify (GST_TOOL (tool), "showall",
			     on_showall_changed, tool);

	g_signal_connect (G_OBJECT (tool->main_dialog), "lock_changed",
			  G_CALLBACK (on_lock_changed), NULL);

	return object;
}

static void
gst_users_tool_finalize (GObject *object)
{
	GstUsersTool *tool = GST_USERS_TOOL (object);

	g_object_unref (tool->users_config);
	g_object_unref (tool->groups_config);
	g_object_unref (tool->profiles);

	(* G_OBJECT_CLASS (gst_users_tool_parent_class)->finalize) (object);
}

static void
update_users (GstUsersTool *tool)
{
	OobsList *list;
	OobsListIter iter;
	GObject *user;
	gboolean valid;

	users_table_clear ();
	list = oobs_users_config_get_users (OOBS_USERS_CONFIG (tool->users_config));

	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		user = oobs_list_get (list, &iter);
		users_table_add_user (OOBS_USER (user), &iter);

		g_object_unref (user);
		valid = oobs_list_iter_next (list, &iter);
	}

	users_table_select_first ();
}

static void
update_groups (GstUsersTool *tool)
{
	OobsList *list;
	OobsListIter iter;
	GObject *group;
	gboolean valid;

	groups_table_clear ();
	privileges_table_clear ();
	list = oobs_groups_config_get_groups (OOBS_GROUPS_CONFIG (tool->groups_config));

	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		group = oobs_list_get (list, &iter);
		groups_table_add_group (OOBS_GROUP (group), &iter);

		/* update privileges table too */
		privileges_table_add_group (OOBS_GROUP (group), &iter);

		g_object_unref (group);
		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
update_profiles (GstUsersTool *tool)
{
	GList *names = NULL;

	names = gst_user_profiles_get_names (tool->profiles);
	table_populate_profiles (tool, names);
	g_list_free (names);
}

void
gst_users_tool_update_gui (GstTool *tool)
{
	update_users (GST_USERS_TOOL (tool));
	update_groups (GST_USERS_TOOL (tool));
	update_profiles (GST_USERS_TOOL (tool));
}

static void
gst_users_tool_update_config (GstTool *tool)
{
	GstUsersTool *users_tool;

	users_tool = GST_USERS_TOOL (tool);

	g_object_get (G_OBJECT (users_tool->users_config),
		      "minimum-uid", &users_tool->minimum_uid,
		      "maximum-uid", &users_tool->maximum_uid,
		      NULL);
	g_object_get (G_OBJECT (users_tool->groups_config),
		      "minimum-gid", &users_tool->minimum_gid,
		      "maximum-gid", &users_tool->maximum_gid,
		      NULL);
}

GstTool*
gst_users_tool_new (void)
{
	return g_object_new (GST_TYPE_USERS_TOOL,
			     "name", "users",
			     "title", _("Users Settings"),
			     "icon", "config-users",
			     NULL);
}
