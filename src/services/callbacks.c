/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of services-admin, a gnome-system-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gst.h"
#include "table.h"
#include "callbacks.h"

#define RESPONSE_START_SERVICE 1
#define RESPONSE_STOP_SERVICE 2

extern GstTool *tool;

/* Helpers */
static gchar*
service_get_description (xmlNodePtr service)
{
	gchar *description = gst_xml_get_child_content (service, "description");
	if (description == NULL)
		description = g_strdup (_("No description available."));

	return description;
}

static gchar*
get_current_runlevel (GstTool *tool)
{
	GtkWidget *option_menu, *menu, *selected_option;
	
	option_menu = gst_dialog_get_widget (tool->main_dialog, "runlevels_menu");
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	selected_option = gtk_menu_get_active (GTK_MENU (menu));

	return g_object_get_data (G_OBJECT (selected_option), "runlevel");
}

static gboolean
current_runlevel_is_default (GstTool *tool)
{
	GtkWidget *option_menu, *menu, *selected_option;
	gboolean default_runlevel;
	
	option_menu = gst_dialog_get_widget (tool->main_dialog, "runlevels_menu");
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	selected_option = gtk_menu_get_active (GTK_MENU (menu));

	default_runlevel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (selected_option), "default"));

	return default_runlevel;
}

/* if we are changing the same runlevel than the default,
 * start or stop the service */
static void
run_service (GstTool *tool, xmlNodePtr service, gchar *runlevel, gchar *action)
{
	gchar *script;
	
	if (current_runlevel_is_default (tool)) {
		script = gst_xml_get_child_content (service, "script");
		gst_tool_run_get_directive (tool, NULL, "throw_service", script, action, NULL);

		g_free (script);
	}
}

static void
toggle_service (GstTool *tool, xmlNodePtr service, gchar* runlevel, gboolean status)
{
	xmlNodePtr runlevels = gst_xml_element_find_first (service, "runlevels");
	xmlNodePtr node;
	gchar *r, *action;
	gboolean found = FALSE;

	if (status == TRUE)
		action = g_strdup_printf ("start");
	else
		action = g_strdup_printf ("stop");

	if (runlevels == NULL)
		runlevels = gst_xml_element_add (service, "runlevels");
	else {
		/* if the node already exists, put its action to "start" or "stop" */
		for (node = gst_xml_element_find_first (runlevels, "runlevel");
		     node != NULL;
		     node = gst_xml_element_find_next (node, "runlevel"))
		{
			r = gst_xml_get_child_content (node, "number");
		
			if (strcmp (r, runlevel) == 0) {
				gst_xml_set_child_content (node, "action", action);
				found = TRUE;
			}

			g_free (r);
		}
	}

	/* if the node hasn't been found, create it */
	if (!found) {
		node = gst_xml_element_add (runlevels, "runlevel");
		gst_xml_element_add (node, "number");
		gst_xml_element_add (node, "action");
		gst_xml_set_child_content (node, "number", runlevel);
		gst_xml_set_child_content (node, "action", action);
	}
}

void
change_runlevel (gchar *runlevel)
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	table_clear ();
	table_populate (root, runlevel);
}

/* check the first service, if it doesn't have priority,
 * hide the "order by startup sequence" button */
void
hide_sequence_ordering_toggle_button (xmlNodePtr root)
{
	GtkWidget *widget = gst_dialog_get_widget (tool->main_dialog, "sequence_ordering");
	xmlNodePtr services, service, priority;

	services = gst_xml_element_find_first (root, "services");
	g_return_if_fail (services != NULL);

	service = gst_xml_element_find_first (services, "service");
	g_return_if_fail (service != NULL);

	priority = gst_xml_element_find_first (service, "priority");

	if (!priority)
		gtk_widget_hide (widget);
	else
		gtk_widget_show (widget);
}

/* callbacks */
static void
callbacks_set_buttons_sensitive (gboolean enabled)
{
	GtkWidget *settings_button = gst_dialog_get_widget (tool->main_dialog, "settings_button");

	gtk_widget_set_sensitive (settings_button, enabled);
}

static void
callbacks_description_changed (xmlNodePtr service)
{
	gchar *description = service_get_description (service);
	GtkWidget *label = gst_dialog_get_widget (tool->main_dialog, "description_label");

	gtk_label_set_text (GTK_LABEL (label), description);
	g_free (description);
}

void
on_services_table_select_row (GtkTreeSelection *selection, gpointer data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	xmlNodePtr service;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		callbacks_set_buttons_sensitive (TRUE);

		/* get the xmlNodePtr */
		gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

		/* Change the description label */
		callbacks_description_changed (service);

	} else {
		callbacks_set_buttons_sensitive (FALSE);
	}
}

void
on_service_priority_changed (GtkWidget *spin_button, gpointer data)
{
	GtkTreeView *runlevel_table = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
	GtkTreeModel *model = gtk_tree_view_get_model (runlevel_table);
	GtkTreePath *path;
	GtkTreeIter iter;
	xmlNodePtr service;
	gint val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_button));
	gchar *value = g_strdup_printf ("%0.2i", val);
	gchar *old_value;

	gtk_tree_view_get_cursor (runlevel_table, &path, NULL);
	gtk_tree_model_get_iter (model, &iter, path);
	
	/* get the xmlNodePtr */
	gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

	/* if the new value is equal to the old value, don't do nothing */
	old_value = gst_xml_get_child_content (service, "priority");

	if (strcmp (old_value, value) == 0)
		return;

	gst_xml_set_child_content (service, "priority", value);

	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, COL_PRIORITY, val, -1);
	gtk_tree_view_scroll_to_cell (runlevel_table, path, NULL, TRUE, 0.5, 0.5);

	gst_dialog_modify (tool->main_dialog);

	g_free (value);
}

static void
dialog_service_get_status (gchar *script)
{
	xmlNodePtr root, node;
	xmlDocPtr doc;
	GtkWidget *status_label = gst_dialog_get_widget (tool->main_dialog, "dialog_status_label");
	GtkWidget *start_button = gst_dialog_get_widget (tool->main_dialog, "dialog_start_button");
	GtkWidget *stop_button = gst_dialog_get_widget (tool->main_dialog, "dialog_stop_button");

	doc = gst_tool_run_get_directive (tool, NULL, "get_status", script, NULL);

	if (!doc)
		return;

	root = gst_xml_doc_get_root (doc);
	node = gst_xml_element_find_first (root, "active");

	if (node) {
		if (gst_xml_element_get_bool_attr (node, "state")) {
			gtk_label_set_text (GTK_LABEL (status_label), _("Running"));
			gtk_widget_set_sensitive (start_button, FALSE);
			gtk_widget_set_sensitive (stop_button, TRUE);
		} else {
			gtk_label_set_text (GTK_LABEL (status_label), _("Stopped"));
			gtk_widget_set_sensitive (start_button, TRUE);
			gtk_widget_set_sensitive (stop_button, FALSE);
		}
	} else {
		gtk_label_set_text (GTK_LABEL (status_label), _("Could not get info"));
		gtk_widget_set_sensitive (start_button, TRUE);
		gtk_widget_set_sensitive (stop_button, TRUE);
	}

	gst_xml_doc_destroy (doc);
}

void
on_settings_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "service_settings_dialog");
	GtkWidget *script_name = gst_dialog_get_widget (tool->main_dialog, "dialog_script_name");
	GtkWidget *service_description = gst_dialog_get_widget (tool->main_dialog, "dialog_service_description");
	GtkWidget *service_priority = gst_dialog_get_widget (tool->main_dialog, "dialog_service_priority");
	GtkWidget *service_priority_label = gst_dialog_get_widget (tool->main_dialog, "dialog_service_priority_label");

	/* we need these to get the xmlNodePtr */
	GtkTreeView *runlevel_table = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
	GtkTreeModel *model = gtk_tree_view_get_model (runlevel_table);
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	xmlNodePtr service;
	
	gchar *description, *script, *title, *p;
	gint priority, response;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (runlevel_table));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the xmlNodePtr */
	gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

	/* get the description and the script name */
	script = gst_xml_get_child_content (service, "script");
	description = service_get_description (service);

	/* get the priority, if it doesn't exist, we simply hide the spinbutton */
	p = gst_xml_get_child_content (service, "priority");

	if (!p) {
		gtk_widget_hide (service_priority);
		gtk_widget_hide (service_priority_label);
	} else {
		gtk_widget_show (service_priority);
		gtk_widget_show (service_priority_label);
		
		priority = atoi (p);

		/* we're modifying the spin button, so we need to block its signal handlers */
		g_signal_handlers_block_by_func (G_OBJECT (service_priority),
						 G_CALLBACK (on_service_priority_changed), tool->main_dialog);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (service_priority), priority);

		g_signal_handlers_unblock_by_func (G_OBJECT (service_priority),
						   G_CALLBACK (on_service_priority_changed), tool->main_dialog);

		g_free (p);
	}

	gtk_label_set_text (GTK_LABEL (script_name), script);
	gtk_label_set_text (GTK_LABEL (service_description), description);

	title = g_strdup_printf (_("Settings for service %s"), script);
	gtk_window_set_title (GTK_WINDOW (dialog), title);

	dialog_service_get_status (script);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == RESPONSE_STOP_SERVICE)
		gst_tool_run_get_directive (tool, NULL, "throw_service", script, "stop", NULL);
	else if (response == RESPONSE_START_SERVICE)
		gst_tool_run_get_directive (tool, NULL, "throw_service", script, "start", NULL);
		
	gtk_widget_hide (dialog);

	/* we don't need this menu anymore */
	g_free (description);
	g_free (script);
	g_free (title);
}

void
on_runlevel_changed (GtkWidget *widget, gpointer data)
{
	change_runlevel (data);
}

void
on_service_toggled (GtkWidget *widget, gchar *path_str, gpointer data)
{
	gboolean value = gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (widget));
	gboolean new_value = !value;
	GstTool *tool = GST_TOOL (data);
	gchar *runlevel = get_current_runlevel (tool);
	GtkTreeView *runlevel_table = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
	GtkTreeModel *model = gtk_tree_view_get_model (runlevel_table);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	xmlNodePtr service;

	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model,
			    &iter,
			    COL_POINTER, &service,
			    -1);

	/* change the XML */
	toggle_service (tool, service, runlevel, new_value);
	
	gtk_tree_store_set (GTK_TREE_STORE (model),
			    &iter,
			    COL_ACTIVE, new_value,
			    -1);

	gst_dialog_modify (tool->main_dialog);

	gtk_tree_path_free (path);
}

void
on_popup_settings_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_settings_button_clicked (widget, callback_data);
}

gboolean
on_table_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkTreePath *path;
	GtkItemFactory *item_factory;
	GtkTreeView *treeview = GTK_TREE_VIEW (widget);

	item_factory = (GtkItemFactory *) data;

	if (gst_dialog_get_complexity (tool->main_dialog) == GST_DIALOG_BASIC)
		return;

	if (event->button == 3) {
		gtk_widget_grab_focus (widget);

		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			gtk_item_factory_popup (item_factory, event->x_root, event->y_root,
						event->button, event->time);
		}

		return TRUE;
	}

	return FALSE;
}

void
on_sequence_ordering_changed (GtkWidget *widget, gpointer data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
	GtkTreeViewColumn *services_column = gtk_tree_view_get_column (treeview, COL_SERVICE);
	GtkTreeViewColumn *priority_column = gtk_tree_view_get_column (treeview, COL_PRIORITY);
	gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	if (active) {
		/* FIXME: is there any other way? */
		gtk_tree_view_column_set_visible (priority_column, TRUE);
		gtk_tree_view_column_clicked (priority_column);
		gtk_tree_view_column_set_visible (priority_column, FALSE);
	} else {
		gtk_tree_view_column_clicked (services_column);
	}
}

void
on_dialog_complexity_change (GtkWidget *widget, GstTool *tool)
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	GstWidget *option_menu = gst_dialog_get_gst_widget (tool->main_dialog, "runlevels_menu");
	gchar *runlevel;
	gint n_option;

	hide_sequence_ordering_toggle_button (root);

	if ((option_menu->advanced != GST_WIDGET_MODE_HIDDEN) &&
	    (gst_dialog_get_complexity (tool->main_dialog) == GST_DIALOG_BASIC))
	{
		runlevel = g_object_get_data (G_OBJECT (option_menu->widget), "default_runlevel");
		n_option = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (option_menu->widget),
							       "default_item"));
		
		gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu->widget), n_option);
		change_runlevel (runlevel);
	}
}
