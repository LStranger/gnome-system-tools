/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of services-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho <garnacho@tuxerver.net>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gst.h"

#include "table.h"
#include "callbacks.h"

GtkActionEntry popup_menu_items[] = {
	{ "Properties", GTK_STOCK_PROPERTIES, "_Properties", NULL, NULL, G_CALLBACK (on_popup_settings_activate) },
};

const gchar *ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Properties'/>"
	"  </popup>"
	"</ui>";

typedef struct TreeItem_ TreeItem;
	
struct TreeItem_ {
	const gchar *service;
	gboolean active;
	gint priority;
};

extern GstTool *tool;

static GtkWidget*
table_popup_menu_create (GtkTreeView *treeview)
{
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
	GtkWidget      *popup;

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_add_actions (action_group, popup_menu_items, G_N_ELEMENTS (popup_menu_items), treeview);

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
		return NULL;

	popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
	return popup;
}

static void
add_columns (GtkTreeView *treeview)
{
	gint i, *col;
	gchar *label;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_insert_column_with_attributes (treeview, -1,
						     _("Active"),
						     renderer,
						     "active", COL_ACTIVE,
						     NULL);

	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (on_service_toggled), tool);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Service"),
							   renderer,
							   "text", COL_SERVICE,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_SERVICE);
	gtk_tree_view_insert_column (treeview, column, -1);
	
	column = gtk_tree_view_column_new_with_attributes ("priority",
							   renderer,
							   "text", COL_PRIORITY,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_PRIORITY);
	gtk_tree_view_column_set_sort_indicator (column, FALSE);
	gtk_tree_view_column_set_visible (column, FALSE);

	gtk_tree_view_insert_column (treeview, column, -1);
}

static GtkTreeModel*
create_model (void)
{
	GtkTreeStore *model;

	model = gtk_tree_store_new (COL_LAST,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_POINTER);
	return GTK_TREE_MODEL(model);
}

void
table_create (void)
{
	GtkWidget        *runlevel_table = gst_dialog_get_widget (tool->main_dialog, "runlevel_table");
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkWidget        *menu;
	
	model = create_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (runlevel_table), model);
	g_object_unref (G_OBJECT (model));
	
	add_columns (GTK_TREE_VIEW (runlevel_table));

	menu = table_popup_menu_create (GTK_TREE_VIEW (runlevel_table));
	g_signal_connect (G_OBJECT (runlevel_table), "button_press_event",
			  G_CALLBACK (on_table_button_press_event), menu);
	g_signal_connect (G_OBJECT (runlevel_table), "popup_menu",
			  G_CALLBACK (on_table_popup_menu), menu);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (runlevel_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_services_table_select_row), NULL);
}

static gchar*
table_value_service(xmlNodePtr node)
{
	gchar *buf, *name, *script;
	g_return_val_if_fail (node != NULL, NULL);

	name = gst_xml_get_child_content (node, "name");
	script = gst_xml_get_child_content (node, "script");
	
	if (name == NULL)
		buf = script;
	else {
		buf = g_strdup_printf ("%s - %s", script, name);
		g_free (name);
		g_free (script);
	}
	
	return buf;
}

static gboolean
table_value_active (xmlNodePtr node, gchar *runlevel)
{
	gboolean value = FALSE;
	xmlNodePtr runlevels = gst_xml_element_find_first (node, "runlevels");
	xmlNodePtr rl;
	gchar *str, *action;

	if (runlevels) {
		for (rl = gst_xml_element_find_first (runlevels, "runlevel");
		     rl != NULL;
		     rl = gst_xml_element_find_next (rl, "runlevel"))
		{
			str = gst_xml_get_child_content (rl, "number");

			if (str && runlevel && (strcmp (str, runlevel) == 0)) {
				action = gst_xml_get_child_content (rl, "action");

				if (strcmp (action, "start") == 0) {
					g_free (action);

					value = TRUE;
				}
			}

			g_free (str);
		}
	}

	return value;
}

static gint
table_value_priority (xmlNodePtr node)
{
	gchar *str = gst_xml_get_child_content (node, "priority");

	if (!str)
		return 0;
	
	return (gint) g_strtod (str, NULL);
}

static TreeItem*
get_node_data (xmlNodePtr service, gchar *runlevel)
{
	TreeItem *item = g_malloc (sizeof(TreeItem));

	item->service = table_value_service (service);
	item->active = table_value_active (service, runlevel);
	item->priority = table_value_priority (service);

	return item;
}

void 
table_populate (xmlNodePtr root, gchar *runlevel)
{
	xmlNodePtr service,services;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(gst_dialog_get_widget (tool->main_dialog, "runlevel_table")));
	
	g_return_if_fail (root != NULL);
	
	services = gst_xml_element_find_first (root, "services");
	
	for (service = gst_xml_element_find_first (services, "service"); service != NULL; service = gst_xml_element_find_next (service, "service"))
	{
		TreeItem *item;

		item = get_node_data (service, runlevel);

		gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
		gtk_tree_store_set (GTK_TREE_STORE (model),
		                    &iter,
		                    COL_ACTIVE, item->active,
		                    COL_SERVICE, item->service,
				    COL_PRIORITY, item->priority,
				    COL_POINTER, service,
		                    -1);
	}
	
}

void
table_clear (void)
{
	GtkWidget *runlevel_table = gst_dialog_get_widget (tool->main_dialog, "runlevel_table");
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (runlevel_table));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
}
