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

#include <gnome.h>
#include <gtk/gtk.h>

#include "gst.h"

#include "table.h"
#include "callbacks.h"

GtkItemFactoryEntry popup_menu_items[] = {
	{ N_("/_Properties"), NULL, G_CALLBACK (on_popup_settings_activate), POPUP_SETTINGS, "<StockItem>", GTK_STOCK_PROPERTIES }
};

typedef struct TreeItem_ TreeItem;
	
struct TreeItem_
{
	const gchar *service;
	gboolean active;
};

extern GstTool *tool;

static gchar*
table_item_factory_trans (const gchar *path, gpointer data)
{
	return _((gchar *) path);
}

static GtkItemFactory*
table_popup_item_factory_create (GtkTreeView *treeview)
{
	GtkItemFactory *item_factory;

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
	gtk_item_factory_set_translate_func (item_factory, table_item_factory_trans,
						  NULL, NULL);
	gtk_item_factory_create_items (item_factory, G_N_ELEMENTS (popup_menu_items),
				       popup_menu_items, treeview);

	return item_factory;
}

static void
add_columns (GtkTreeView *treeview)
{
	gint i, *col;
	gchar *label;
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_insert_column_with_attributes (treeview, -1,
						     _("Active"),
						     renderer,
						     "active", COL_ACTIVE,
						     NULL);

	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (on_service_toggled), tool);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (treeview, -1,
						     _("Service"),
						     renderer,
						     "text", COL_SERVICE,
						     NULL);
}

static GtkTreeModel*
create_model (void)
{
	GtkTreeStore *model;

	model = gtk_tree_store_new (COL_LAST,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING,
				    G_TYPE_POINTER);
	return GTK_TREE_MODEL(model);
}

void
table_create (void)
{
	GtkWidget *runlevel_table = gst_dialog_get_widget (tool->main_dialog, "runlevel_table");
	GtkTreeModel *model;
	GtkItemFactory *item_factory;
	GtkTreeSelection *selection;
	
	model = create_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (runlevel_table), model);
	g_object_unref (G_OBJECT (model));
	
	add_columns (GTK_TREE_VIEW (runlevel_table));

	item_factory = table_popup_item_factory_create (GTK_TREE_VIEW (runlevel_table));
	g_signal_connect (G_OBJECT (runlevel_table), "button_press_event",
			  G_CALLBACK (on_table_button_press_event), item_factory);

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
table_value_active (xmlNodePtr node, gint runlevel)
{
	gboolean value = FALSE;
	xmlNodePtr runlevels = gst_xml_element_find_first (node, "runlevels");
	xmlNodePtr rl;
	gchar *number, *action;

	if (runlevels) {
		for (rl = gst_xml_element_find_first (runlevels, "runlevel");
		     rl != NULL;
		     rl = gst_xml_element_find_next (rl, "runlevel"))
		{
			number = gst_xml_get_child_content (rl, "number");

			if (g_ascii_isdigit (number[0])) {
				if (atoi (number) == runlevel) {
					action = gst_xml_get_child_content (rl, "action");

					if (strcmp (action, "start") == 0) {
						g_free (action);

						value = TRUE;
					}
				}
			}

			g_free (number);
		}
	}

	return value;
}

static TreeItem*
get_node_data (xmlNodePtr service, gint runlevel)
{
	TreeItem *item = g_malloc (sizeof(TreeItem));

	item->service = table_value_service (service);
	item->active = table_value_active (service, runlevel);

	return item;
}

void 
table_populate (xmlNodePtr root, gint runlevel)
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
