/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of runlevel-admin, a ximian-setup-tool frontend 
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

#include "xst.h"
#include "table.h"
#include "callbacks.h"

extern XstTool *tool;

extern GdkPixbuf *start_icon;
extern GdkPixbuf *stop_icon;
extern GdkPixbuf *do_nothing_icon;

GList *parameters_list = NULL;

/* Helpers */
static gchar*
service_get_description (xmlNodePtr service)
{
	gchar *description = xst_xml_get_child_content (service, "description");
	if (description == NULL)
		description = g_strdup (_("No description available."));

	return description;
}

static void
service_get_parameters (gchar *script)
{
	xmlNodePtr root, node;
	xmlDocPtr parameters = xst_tool_run_get_directive (tool, NULL, "service_parameters", script, NULL);
	gchar *param;
	GtkWidget *option_menu = xst_dialog_get_widget (tool->main_dialog, "dialog_service_parameters");
	GtkWidget *menu = gtk_menu_new ();
	GtkWidget *menu_item;

	root = xst_xml_doc_get_root (parameters);

	for (node = xst_xml_element_find_first (root, "parameter");
	     node != NULL;
	     node = xst_xml_element_find_next (node, "parameter"))
	{
		param = xst_xml_element_get_content (node);
		menu_item = gtk_menu_item_new_with_label (param);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);

		parameters_list = g_list_append (parameters_list, param);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);	
}

/* callbacks */

void
on_main_dialog_update_complexity (GtkWidget *main_dialog, gpointer data)
{
	XstDialogComplexity complexity;
	complexity = XST_DIALOG (main_dialog)->complexity;
	table_update_state (complexity);
}

gboolean
callbacks_conf_read_failed_hook (XstTool *tool, XstReportLine *rline, gpointer data)
{
	GtkWidget *dialog;
	gchar *txt;

	txt = g_strdup_printf (_("The file ``%s'' is missing or could not be read:\nThe configuration will show empty."), rline->argv[0]);
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, txt);
        gtk_dialog_run (GTK_DIALOG (dialog));
	
	g_free (txt);

	return TRUE;
}


static void
callbacks_service_toggled (GtkTreeView *treeview, GtkTreeIter iter, GtkTreeViewColumn *column, xmlNodePtr service)
{
	GtkTreeModel *model;
	GList *column_list;
	GdkPixbuf *image;
	gint ncol;
	xmlNodePtr runlevels, node;
	gchar *level;

	g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

	/* We get the column number we are clicking */
	column_list = gtk_tree_view_get_columns (GTK_TREE_VIEW (treeview));
	ncol = g_list_index (column_list, column);

	/* we really want to edit only the runlevel columns, not the service names */
	if (ncol > COL_SERVICE) {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

		/* get the image to change it */
		gtk_tree_model_get (model, &iter,
				    ncol, &image,
				    -1);

		runlevels = xst_xml_element_find_first (service, "runlevels");

		if (runlevels == NULL) 
			runlevels = xst_xml_element_add (service, "runlevels");

		/* the image will have the next states:   started --> stopped --> do nothing --> ... */
		if (image == start_icon) {
			/* The state turns to stopped */
			image = stop_icon;
			
			for (node = xst_xml_element_find_first (runlevels, "runlevel");
			     node != NULL;
			     node = xst_xml_element_find_next (node, "runlevel"))
			{
				level = xst_xml_get_child_content (node, "number");
				if (atoi (level) == ncol - 2) {
					g_free (level);
					xst_xml_set_child_content (node, "action", "stop");
					break;
				}
				
				g_free (level);
			}
		}
		else if (image == stop_icon) {
			/* The state turns to "do nothing" */
			image = do_nothing_icon;

			for (node = xst_xml_element_find_first (runlevels, "runlevel");
			     node != NULL;
			     node = xst_xml_element_find_next (node, "runlevel"))
			{
				level = xst_xml_get_child_content (node, "number");
				if (atoi (level) == ncol - 2) {
					g_free (level);
					xst_xml_element_destroy (node);
					break;
				}

				g_free (level);
			}
		}
		else {
			/* The state turns to started */
			gchar *buf;
		
			image = start_icon;

			if (runlevels == NULL){
				runlevels= xst_xml_element_add (node, "runlevels");
			}
			node = xst_xml_element_add (runlevels, "runlevel");
			xst_xml_element_add (node, "number");
			xst_xml_element_add (node, "action");
			buf = g_strdup_printf ("%i", ncol - 2);
			xst_xml_set_child_content (node, "number", buf);
			xst_xml_set_child_content (node, "action", "start");
			g_free (buf);
		}

		xst_dialog_modify (tool->main_dialog);	
	
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ncol, image, -1);
	}

	g_list_free (column_list);
}

static void
callbacks_set_buttons_sensitive ()
{
	GtkWidget *settings_button = xst_dialog_get_widget (tool->main_dialog, "settings_button");

	gtk_widget_set_sensitive (settings_button, TRUE);
}

static void
callbacks_description_changed (xmlNodePtr service)
{
	gchar *description = service_get_description (service);
	GtkWidget *label = xst_dialog_get_widget (tool->main_dialog, "description_label");

	gtk_label_set_text (GTK_LABEL (label), description);
	g_free (description);
}

void
on_runlevel_table_clicked (GtkTreeView *treeview, gpointer data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeViewColumn *column;
	xmlNodePtr service;

	g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (treeview), &path, &column);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_tree_model_get_iter (model, &iter, path);
	
	/* get the xmlNodePtr */
	gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

	/* if we are clicking to edit a service action in a runlevel, we want to call this */
	callbacks_service_toggled (treeview, iter, column, service);

	/* Change the description label */
	callbacks_description_changed (service);

	/* we also want to show the service description in the window */
	callbacks_set_buttons_sensitive (); 

	/* free the variables */
	gtk_tree_path_free (path);
}

void
on_throw_service_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *script_label = xst_dialog_get_widget (tool->main_dialog, "dialog_script_name");
	GtkWidget *option_menu = xst_dialog_get_widget (tool->main_dialog, "dialog_service_parameters");
	gchar *script = (gchar *) gtk_label_get_text (GTK_LABEL (script_label));
	gchar *parameter = g_list_nth_data (parameters_list, gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu)));

	xst_tool_run_get_directive (tool, NULL, "throw_service", script, parameter, NULL);
}

void
on_service_priority_changed (GtkWidget *spin_button, gpointer data)
{
	GtkTreeView *runlevel_table = GTK_TREE_VIEW (xst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
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
	old_value = xst_xml_get_child_content (service, "priority");
	if (strcmp (old_value, value) == 0)
		return;

	xst_xml_set_child_content (service, "priority", value);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, COL_PRIORITY, value, -1);

	xst_dialog_modify (tool->main_dialog);

	g_free (value);
}

void
on_settings_button_clicked (GtkWidget *button, gpointer data)
{
	GtkWidget *dialog = xst_dialog_get_widget (tool->main_dialog, "service_settings_dialog");
	GtkWidget *script_name = xst_dialog_get_widget (tool->main_dialog, "dialog_script_name");
	GtkWidget *service_description = xst_dialog_get_widget (tool->main_dialog, "dialog_service_description");
	GtkWidget *service_priority = xst_dialog_get_widget (tool->main_dialog, "dialog_service_priority");
	GtkWidget *option_menu = xst_dialog_get_widget (tool->main_dialog, "dialog_service_parameters");

	/* we need these to get the xmlNodePtr */
	GtkTreeView *runlevel_table = GTK_TREE_VIEW (xst_dialog_get_widget (tool->main_dialog, "runlevel_table"));
	GtkTreeModel *model = gtk_tree_view_get_model (runlevel_table);
	GtkTreePath *path;
	GtkTreeIter iter;
	xmlNodePtr service;
	
	gchar *description, *script;
	gint priority;

	gtk_tree_view_get_cursor (runlevel_table, &path, NULL);
	gtk_tree_model_get_iter (model, &iter, path);

	/* get the xmlNodePtr */
	gtk_tree_model_get (model, &iter, COL_POINTER, &service, -1);

	/* get the description and the script name */
	script = xst_xml_get_child_content (service, "script");
	description = service_get_description (service);

	/* get the priority */
	priority = atoi (xst_xml_get_child_content (service, "priority"));
	
	gtk_label_set_text (GTK_LABEL (script_name), script);
	gtk_label_set_text (GTK_LABEL (service_description), description);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (service_priority), priority);

	/* sets the option menu with the valid service parameters */
	service_get_parameters (script);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	/* we don't need this menu anymore */
	gtk_option_menu_remove_menu (GTK_OPTION_MENU (option_menu));
	g_list_free (parameters_list);
	g_free (description);
	g_free (script);
}
