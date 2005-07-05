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

#define SERVICE_START  "start"
#define SERVICE_STOP   "stop"

extern GstTool *tool;

/* Helpers */
static void
show_settings (void)
{
	GtkTreeView *treeview = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "services_list"));
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	xmlNodePtr service;
	gchar *script, *title;
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "service_settings_dialog");

	model = gtk_tree_view_get_model (treeview);
	selection = gtk_tree_view_get_selection (treeview);

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	/* get the xmlNodePtr */
	gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

	/* get the description and the script name */
	script = gst_xml_get_child_content (service, "script");

	title = g_strdup_printf (_("Settings for \"%s\""), script);
	gtk_window_set_title (GTK_WINDOW (dialog), title);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);
}

static void
toggle_service_xml (GstTool *tool, xmlNodePtr service, gboolean status)
{
	xmlNodePtr runlevels = gst_xml_element_find_first (service, "runlevels");
	xmlNodePtr node;
	gchar *r, *default_runlevel, *action;
	gboolean found = FALSE;

	action = (status) ? SERVICE_START : SERVICE_STOP;
	default_runlevel = g_object_get_data (G_OBJECT (tool), "default_runlevel");

	if (runlevels == NULL)
		runlevels = gst_xml_element_add (service, "runlevels");

	/* if the node already exists, put its action to "start" or "stop" */
	for (node = gst_xml_element_find_first (runlevels, "runlevel");
	     node != NULL; node = gst_xml_element_find_next (node, "runlevel"))	{
		r = gst_xml_get_child_content (node, "name");

		if (r && default_runlevel &&
		    strcmp (r, default_runlevel) == 0) {
			gst_xml_set_child_content (node, "action", action);
			found = TRUE;
		}

		g_free (r);
	}

	/* if the node hasn't been found, create it */
	if (!found) {
		node = gst_xml_element_add (runlevels, "runlevel");

		gst_xml_element_add_with_content (node, "name",   default_runlevel);
		gst_xml_element_add_with_content (node, "action", action);
	}
}

static void
activate_deactivate_service (GstTool *tool, xmlNodePtr service, gboolean activate)
{
	gchar *script;

	script = gst_xml_get_child_content (service, "script");

	if (activate)
		gst_tool_run_get_directive (tool, NULL, "throw_service", script, SERVICE_START, NULL);
	else
		gst_tool_run_get_directive (tool, NULL, "throw_service", script, SERVICE_STOP, NULL);
}

static void
do_popup_menu (GtkWidget *popup, GdkEventButton *event)
{
	gint button, event_time;

	if (!popup)
		return;

	if (event) {
		button     = event->button;
		event_time = event->time;
	} else {
		button     = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

static gboolean
show_warning_dialog (GstTool *tool)
{
	GtkWidget *dialog;
	gint       response;

	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_YES_NO,
					 /* FIXME: put service name !!! */
					 _("Are you sure you want to deactivate this service?"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("This may affect your system behavior in "
						    "several ways, possibly leading to data loss"));

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return (response == GTK_RESPONSE_YES);
}

/* callbacks */
void
on_service_toggled (GtkWidget *widget, gchar *path_str, gpointer data)
{
	GtkTreeView  *treeview = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "services_list"));
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreePath  *path = gtk_tree_path_new_from_string (path_str);
	GstTool      *tool = GST_TOOL (data);
	GtkTreeIter   iter;
	xmlNodePtr    service;
	gboolean      value, new_value, dangerous;

	value = gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (widget));
	new_value = !value;

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model,
			    &iter,
			    COL_POINTER, &service,
			    COL_DANGEROUS, &dangerous,
			    -1);

	if (new_value || !dangerous || show_warning_dialog (tool)) {
		/* change the XML */
		toggle_service_xml (tool, service, new_value);

		/* activate/deactivate the service */
		activate_deactivate_service (tool, service, new_value);

		gtk_list_store_set (GTK_LIST_STORE (model),
				    &iter,
				    COL_ACTIVE, new_value,
				    -1);

		gst_dialog_modify (tool->main_dialog);
	}

	gtk_tree_path_free (path);
}

void
on_popup_settings_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	show_settings ();
}

gboolean
on_table_button_press_event (GtkWidget *widget, GdkEventButton *event, GtkWidget *popup)
{
	GtkTreePath *path;
	GtkTreeView *treeview = GTK_TREE_VIEW (widget);

	if (event->type == GDK_2BUTTON_PRESS ||
	    event->type == GDK_3BUTTON_PRESS) {
		show_settings ();

		return TRUE;
	}

	if (event->button == 3) {
		gtk_widget_grab_focus (widget);

		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			do_popup_menu (popup, event);
		}

		return TRUE;
	}

	return FALSE;
}

gboolean
on_table_popup_menu (GtkWidget *widget, GtkWidget *popup)
{
	do_popup_menu (popup, NULL);
	return TRUE;
}
