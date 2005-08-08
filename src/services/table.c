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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gst.h"

#include "table.h"
#include "service.h"
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

typedef struct _TreeItem TreeItem;
	
struct _TreeItem {
	gboolean   active;
	gchar     *desc;
	GdkPixbuf *image;
	gboolean   dangerous;
};

extern GstTool *tool;

static void
free_tree_item (TreeItem *item)
{
	g_free (item->desc);
	g_object_unref (item->image);
	g_free (item);
}

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
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();

	/* Checkbox */
	renderer = gtk_cell_renderer_toggle_new ();

	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (on_service_toggled), tool);

	g_object_set (G_OBJECT (renderer), "xpad", 12, NULL);

	gtk_tree_view_insert_column_with_attributes (treeview,
						     -1, "",
						     renderer,
						     "active", COL_ACTIVE,
						     NULL);
	/* Image */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COL_IMAGE,
					     NULL);

	/* Text */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "markup", COL_DESC,
					     NULL);

	g_object_set (G_OBJECT (renderer), "yalign", 0, NULL);
	gtk_tree_view_insert_column (treeview, column, -1);

	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_column_clicked (column);
}

static GtkTreeModel*
create_model (void)
{
	GtkListStore *model;

	model = gtk_list_store_new (COL_LAST,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_INT,
				    G_TYPE_POINTER);
	return GTK_TREE_MODEL(model);
}

void
table_create (void)
{
	GtkWidget        *runlevel_table = gst_dialog_get_widget (tool->main_dialog, "services_list");
	GtkTreeModel     *model;
	GtkTreeSelection *selection;
	GtkWidget        *menu;
	
	model = create_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (runlevel_table), model);
	g_object_unref (G_OBJECT (model));
	
	add_columns (GTK_TREE_VIEW (runlevel_table));

	/* FIXME: remove this until I figure out what to do with the advanced properties

	menu = table_popup_menu_create (GTK_TREE_VIEW (runlevel_table));
	g_signal_connect (G_OBJECT (runlevel_table), "button_press_event",
			  G_CALLBACK (on_table_button_press_event), menu);
	g_signal_connect (G_OBJECT (runlevel_table), "popup_menu",
			  G_CALLBACK (on_table_popup_menu), menu);
	*/

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (runlevel_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
}

static gboolean
table_value_active (xmlNodePtr node, gchar *runlevel)
{
	xmlNodePtr runlevels = gst_xml_element_find_first (node, "runlevels");
	xmlNodePtr rl;
	gchar *str, *action;
	gboolean value, found;

	if (!runlevels)
		return FALSE;

	value = found = FALSE;
	rl = gst_xml_element_find_first (runlevels, "runlevel");

	while (!found && rl) {
		str = gst_xml_get_child_content (rl, "name");

		if (str && runlevel && (strcmp (str, runlevel) == 0)) {
			action = gst_xml_get_child_content (rl, "action");
			found  = TRUE;

			if (action && strcmp (action, "start") == 0)
				value = TRUE;
			else
				value = FALSE;

			g_free (action);
		}

		g_free (str);
		rl = gst_xml_element_find_next (rl, "runlevel");
	}

	return value;
}

static TreeItem*
get_node_data (xmlNodePtr service, gchar *runlevel)
{
	const ServiceDescription *desc;
	TreeItem *item;
	gchar *script, *role;

	script = gst_xml_get_child_content (service, "script");
	role   = gst_xml_get_child_content (service, "role");
	desc   = service_search (role);

	if (!desc)
		return NULL;

	item = g_malloc0 (sizeof (TreeItem));
	item->desc      = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s (<i>%s</i>)</span>\n%s",
					   _(desc->description), script, _(desc->long_description));

	item->image     = gtk_icon_theme_load_icon (tool->icon_theme, desc->icon, 48, 0, NULL);
	item->active    = table_value_active (service, runlevel);
	item->dangerous = desc->dangerous;

	return item;
}

void 
table_populate (xmlNodePtr root, gchar *runlevel)
{
	xmlNodePtr    service,services;
	GtkTreeView  *treeview;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	g_return_if_fail (root != NULL);
	
	treeview = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "services_list"));
	model    = gtk_tree_view_get_model (treeview);
	services = gst_xml_element_find_first (root, "services");
	
	for (service = gst_xml_element_find_first (services, "service");
	     service; service = gst_xml_element_find_next (service, "service")) {
		TreeItem *item;

		item = get_node_data (service, runlevel);

		/* FIXME: should do something with unknown services? */
		if (!item)
			continue;

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model),
		                    &iter,
		                    COL_ACTIVE,    item->active,
		                    COL_DESC,      item->desc,
				    COL_IMAGE,     item->image,
				    COL_POINTER,   service,
				    COL_DANGEROUS, item->dangerous,
		                    -1);

		free_tree_item (item);
	}
}
