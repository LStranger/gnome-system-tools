/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of boot-admin, a ximian-setup-tool frontend 
 * for boot administration.
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <gnome.h>

#include "gst.h"
#include "gst-report-hook.h"
#include "boot-druid.h"
#include "boot-settings.h"
#include "boot-append-gui.h"
#include "callbacks.h"
#include "transfer.h"
#include "table.h"

extern GstTool *tool;
extern GtkWidget *boot_table;
extern GtkTreeIter default_entry_iter;

extern GdkPixbuf *selected_icon;
extern GdkPixbuf *not_selected_icon;


void
on_boot_add_clicked (GtkButton *button, gpointer data)
{
	BootDruid *druid;

	if (gst_tool_get_access (tool)) {
		druid = boot_druid_new ();

		if (druid)
			gtk_widget_show_all (GTK_WIDGET (druid));
		else {
			gchar *error = g_strdup ("Can't add more images, maximum count reached.");

			boot_settings_gui_error (GTK_WINDOW (tool->main_dialog), error);
		}
	}
}

static gboolean
boot_image_editor_construct (BootImageEditor *editor, BootImage *image)
{
	GtkWidget *w;

	editor->gui = boot_settings_gui_new (image, GTK_WIDGET (editor->dialog));

	if (!editor->gui)
		return FALSE;

	w = glade_xml_get_widget (editor->gui->xml, "boot_settings_editor");
	gtk_widget_reparent (w, GTK_DIALOG (editor->dialog)->vbox);

	gtk_window_set_title (GTK_WINDOW (editor->dialog), _("Boot Image Editor"));
	gtk_window_set_policy (GTK_WINDOW (editor->dialog), FALSE, TRUE, TRUE);
	gtk_window_set_modal (GTK_WINDOW (editor->dialog), TRUE);

	boot_settings_gui_setup (editor->gui, GTK_DIALOG (editor->dialog)->vbox);

	return TRUE;
}

static BootImageEditor *
boot_image_editor_new (BootImage *image)
{
	BootImageEditor *new;
	GladeXML *xml;

	if (!image)
		return NULL;

	new = g_new0 (BootImageEditor, 1);

	xml = glade_xml_new (tool->glade_path, "boot_dialog", NULL);
	new->dialog = GTK_DIALOG (glade_xml_get_widget (xml, "boot_dialog"));
	g_object_unref (xml);
	
	if (boot_image_editor_construct (new, image))
		return new;
	else
	{
		gtk_widget_destroy (GTK_WIDGET (new->dialog));
		return NULL;
	}
}

void
on_boot_settings_clicked (GtkButton *button, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	BootImage *image;
	BootImageEditor *editor;
	xmlNodePtr node;
	gint response;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (boot_table));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, BOOT_LIST_COL_POINTER, &node, -1);
	}


	if (gst_tool_get_access (tool)) {
		image = boot_image_get_by_node (node);
		editor = boot_image_editor_new (image);

		response = gtk_dialog_run (editor->dialog);
		switch (response)
		{
		case GTK_RESPONSE_OK:
			if (boot_settings_gui_save (editor->gui, TRUE))
				boot_image_save (editor->gui->image);
			else
				return;
			break;
		default:
			break;
		}

		boot_table_update ();
		gtk_widget_destroy (GTK_WIDGET (editor->dialog));
	}
}

static gboolean
boot_append_editor_construct (BootAppendEditor *editor, BootSettingsGui *settings)
{
	GtkWidget *w;

	editor->gui = boot_append_gui_new (settings, GTK_WIDGET (editor->dialog));

	if (!editor->gui)
		return FALSE;

	w = glade_xml_get_widget (editor->gui->xml, "boot_append_editor");
	gtk_widget_reparent (w, GTK_DIALOG (editor->dialog)->vbox);

	gtk_window_set_title (GTK_WINDOW (editor->dialog), _("Boot Append Editor"));
	gtk_window_set_policy (GTK_WINDOW (editor->dialog), FALSE, TRUE, TRUE);
	gtk_window_set_modal (GTK_WINDOW (editor->dialog), TRUE);

	boot_append_gui_setup (editor->gui, settings);

	return TRUE;
}

static BootAppendEditor *
boot_append_editor_new (BootSettingsGui *settings)
{
	BootAppendEditor *new;
	GladeXML *xml;

	if (!settings)
		return NULL;

	new = g_new0 (BootAppendEditor, 1);

	xml = glade_xml_new (tool->glade_path, "boot_dialog", NULL);
	new->dialog = GTK_DIALOG (glade_xml_get_widget (xml, "boot_dialog"));
	g_object_unref (xml);

	if (boot_append_editor_construct (new, settings))
		return new;
	else
	{
		gtk_widget_destroy (GTK_WIDGET (new->dialog));
		return NULL;
	}
}

void
on_boot_append_browse_clicked (GtkButton *button, gpointer data)
{
	BootSettingsGui *settings;
	BootAppendEditor *editor;
	gint response;
	gchar *append;

	if (gst_tool_get_access (tool))
	{
		settings = (BootSettingsGui *) data;
		editor = boot_append_editor_new (settings);
		response = gtk_dialog_run (editor->dialog);

		switch (response)
		{
		case GTK_RESPONSE_OK:
			if (boot_append_gui_save (editor->gui, &append))
				if (append)
				{
					gst_ui_entry_set_text (GTK_ENTRY (editor->gui->settings->append), g_strdup (append));
					g_free (append);
				}
			break;
		default:
			break;
		}
		
		gtk_widget_destroy (GTK_WIDGET (editor->dialog));
		
	}
}

gboolean
on_boot_table_clicked (GtkWidget *w, gpointer data)
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *column_list;
	gint ncol;
	xmlNodePtr node;
	gchar *label;

	column_list = gtk_tree_view_get_columns (GTK_TREE_VIEW (w));
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (w), &path, &column);

	ncol = g_list_index (column_list, column);

	if (ncol == BOOT_LIST_COL_DEFAULT) {
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (w));
		gtk_tree_model_get_iter (model, &iter, path);		
		
		/* we get the selected entry iter and unselect it */
		gtk_tree_store_set (GTK_TREE_STORE (model), &default_entry_iter, BOOT_LIST_COL_DEFAULT, not_selected_icon, -1);
		
		/* we set as selected the current iter */
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter, BOOT_LIST_COL_DEFAULT, selected_icon, -1);
		g_object_set_data (G_OBJECT (w), "default", &iter);

		/* we set the current iter as the default */
		gtk_tree_model_get (model, &iter, BOOT_LIST_COL_POINTER, &node, -1);
		label = gst_xml_get_child_content (node, "label");
		gst_xml_set_child_content (gst_xml_doc_get_root (tool->config), "default", label);
		default_entry_iter = iter;

		gst_dialog_modify (tool->main_dialog);
	}
	
	g_list_free (column_list);
	gtk_tree_path_free (path);
	
	return FALSE;
}

void
on_boot_table_cursor_changed (GtkTreeSelection *selection, gpointer data)
{
	callbacks_actions_set_sensitive (TRUE);
}

gboolean
on_boot_table_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata)
{
	GtkTreePath *path;
	GtkItemFactory *factory;

	factory = (GtkItemFactory *) gdata;

	if (event->button == 3)
	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			gtk_item_factory_popup (factory, event->x_root, event->y_root,
						event->button, event->time);
		}
		return TRUE;
	}
	return FALSE;
}

void
on_popup_add_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_boot_add_clicked (callback_data, NULL);
}

void
on_popup_settings_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_boot_settings_clicked (callback_data, NULL);
}

void
on_popup_delete_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	on_boot_delete_clicked (callback_data, NULL);
}

/* Helpers */

void
callbacks_actions_set_sensitive (gboolean state)
{
	GstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;

	if (gst_tool_get_access (tool) && complexity == GST_DIALOG_ADVANCED) {
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "boot_add"), TRUE);
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "boot_delete"),
					  state);
	}
	
	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "boot_settings"), state);
}

static void
callbacks_buttons_set_visibility (GstDialog *main_dialog)
{
	switch (gst_dialog_get_complexity (main_dialog)) {
	case GST_DIALOG_ADVANCED:
		gtk_widget_show (gst_dialog_get_widget (main_dialog, "boot_add"));
		gtk_widget_show (gst_dialog_get_widget (main_dialog, "boot_delete"));
		break;
	case GST_DIALOG_BASIC:
		gtk_widget_hide (gst_dialog_get_widget (main_dialog, "boot_add"));
		gtk_widget_hide (gst_dialog_get_widget (main_dialog, "boot_delete"));
		break;
	default:
		g_warning ("Unknown complexity.");
		break;
	}
}

/* Main window callbacks */

void
on_boot_delete_clicked (GtkButton *button, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	xmlNodePtr   node;
	gchar       *label, *buf;
	gint         count, reply;
	GtkWidget   *d;
	
	g_return_if_fail (gst_tool_get_access (tool));

	count = boot_image_count (gst_xml_doc_get_root (tool->config));
	if (count <= 1) {
		d = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_OK,
					    _("Without at least one boot image,\n"
					      "your system will not start.\n"));

		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);
		return;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (boot_table));
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, BOOT_LIST_COL_POINTER, &node, -1);
	}
	
	label = gst_xml_get_child_content (node, "label");
	buf = g_strdup_printf (_("Are you sure you want to delete '%s'?"), label);
	g_free (label);

	d = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_YES_NO,
				    buf);

	g_free (buf);
	reply = gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);

	if (reply != GTK_RESPONSE_YES)
		return;

	gst_xml_element_destroy (node);
	boot_table_update ();
	gst_dialog_modify (tool->main_dialog);
	callbacks_actions_set_sensitive (FALSE);
}

void
on_main_dialog_update_complexity (GstDialog *main_dialog, gpointer data)
{
	GstDialogComplexity complexity = gst_dialog_get_complexity (tool->main_dialog);
	GtkTreeView *boot_table = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "boot_table"));

	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (boot_table));

	callbacks_actions_set_sensitive (FALSE);
	
	boot_table_update_state (complexity);
	
	callbacks_buttons_set_visibility (tool->main_dialog);
}

/* Hooks */

gboolean
callbacks_conf_read_failed_hook (GstTool *tool, GstReportLine *rline, gpointer data)
{
	GtkWidget *dialog;
	gchar *txt;

	txt = g_strdup_printf (_("The file ``%s'' is missing or could not be read:\n"
				 "The configuration will show empty."), rline->argv[0]);

	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 txt);

	g_free (txt);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	/* Handled, don't go looking for more hooks to run */
	return TRUE;
}

