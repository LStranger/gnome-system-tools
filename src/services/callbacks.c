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

#include <gnome.h>

#include "gst.h"
#include "table.h"
#include "callbacks.h"

extern GstTool *tool;

GList *parameters_list = NULL;

/* Helpers */
static gchar*
service_get_description (xmlNodePtr service)
{
	gchar *description = gst_xml_get_child_content (service, "description");
	if (description == NULL)
		description = g_strdup (_("No description available."));

	return description;
}

static void
service_get_parameters (gchar *script)
{
	xmlNodePtr root, node;
	xmlDocPtr parameters = gst_tool_run_get_directive (tool, NULL, "service_parameters", script, NULL);
	gchar *param;
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "dialog_service_parameters");
	GtkWidget *menu = gtk_menu_new ();
	GtkWidget *menu_item;

	root = gst_xml_doc_get_root (parameters);

	for (node = gst_xml_element_find_first (root, "parameter");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "parameter"))
	{
		param = gst_xml_element_get_content (node);
		menu_item = gtk_menu_item_new_with_label (param);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);

		parameters_list = g_list_append (parameters_list, param);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
}

static gint
get_current_runlevel (GstTool *tool)
{
	GtkWidget *option_menu, *menu, *selected_option;
	gint runlevel;
	
	option_menu = gst_dialog_get_widget (tool->main_dialog, "runlevels_menu");
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	selected_option = gtk_menu_get_active (GTK_MENU (menu));

	runlevel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (selected_option), "runlevel"));

	return runlevel;
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
toggle_service (GstTool *tool, xmlNodePtr service, gint runlevel, gboolean status)
{
	xmlNodePtr runlevels = gst_xml_element_find_first (service, "runlevels");
	xmlNodePtr node;
	gchar *buf, *r, *action;
	gboolean found = FALSE;



	buf = g_strdup_printf ("%i", runlevel);

	if (status == TRUE)
		action = g_strdup_printf ("start");
	else
		action = g_strdup_printf ("stop");

	if (runlevels == NULL) {
		runlevels = gst_xml_element_add (node, "runlevels");
	}
	
	/* if the node already exists, put its action to "start" or "stop" */
	for (node = gst_xml_element_find_first (runlevels, "runlevel");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "runlevel"))
	{
		r = gst_xml_get_child_content (node, "number");
		
		if (strcmp (r, buf) == 0) {
			gst_xml_set_child_content (node, "action", action);
			found = TRUE;
		}

		g_free (r);
	}

	/* if the node hasn't been found, create it */
	if (!found) {
		node = gst_xml_element_add (runlevels, "runlevel");
		gst_xml_element_add (node, "number");
		gst_xml_element_add (node, "action");
		gst_xml_set_child_content (node, "number", buf);
		gst_xml_set_child_content (node, "action", action);
	}

/*	run_service (tool, service, buf, action); */

	g_free (buf);

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
on_throw_service_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *script_label = gst_dialog_get_widget (tool->main_dialog, "dialog_script_name");
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "dialog_service_parameters");
	gchar *script = (gchar *) gtk_label_get_text (GTK_LABEL (script_label));
	gchar *parameter = g_list_nth_data (parameters_list, gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu)));

	gst_tool_run_get_directive (tool, NULL, "throw_service", script, parameter, NULL);
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

	gst_dialog_modify (tool->main_dialog);

	g_free (value);
}

void
on_settings_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "service_settings_dialog");
	GtkWidget *script_name = gst_dialog_get_widget (tool->main_dialog, "dialog_script_name");
	GtkWidget *service_description = gst_dialog_get_widget (tool->main_dialog, "dialog_service_description");
	GtkWidget *service_priority = gst_dialog_get_widget (tool->main_dialog, "dialog_service_priority");
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog, "dialog_service_parameters");

	/* we need these to get the xmlNodePtr */
	GtkTreeView *runlevel_table = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
	GtkTreeModel *model = gtk_tree_view_get_model (runlevel_table);
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	xmlNodePtr service;
	
	gchar *description, *script, *title;
	gint priority;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (runlevel_table));
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the xmlNodePtr */
	gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

	/* get the description and the script name */
	script = gst_xml_get_child_content (service, "script");
	description = service_get_description (service);

	/* get the priority */
	priority = atoi (gst_xml_get_child_content (service, "priority"));
	
	gtk_label_set_text (GTK_LABEL (script_name), script);
	gtk_label_set_text (GTK_LABEL (service_description), description);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (service_priority), priority);

	/* sets the option menu with the valid service parameters */
	service_get_parameters (script);

	title = g_strdup_printf (_("Settings for service %s"), script);
	gtk_window_set_title (GTK_WINDOW (dialog), title);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	/* we don't need this menu anymore */
	gtk_option_menu_remove_menu (GTK_OPTION_MENU (option_menu));
	g_free (description);
	g_free (script);
	g_free (title);

	/* we need to free the list and put it again to NULL, it may be used again */
	g_list_free (parameters_list);
	parameters_list = NULL;
}

void
on_runlevel_changed (GtkWidget *widget, gpointer data)
{
	gint runlevel = GPOINTER_TO_INT (data);
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	table_clear ();
	table_populate (root, runlevel);
}

void
on_service_toggled (GtkWidget *widget, gchar *path_str, gpointer data)
{
	gboolean value = gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (widget));
	gboolean new_value = !value;
	GstTool *tool = GST_TOOL (data);
	gint runlevel = get_current_runlevel (tool);
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
