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
#include "gst-hig-dialog.h"
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

static void
on_boot_help_button_clicked (GtkWidget *widget, gpointer data)
{
	gst_tool_show_help (tool, NULL);
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

	g_signal_connect (G_OBJECT (glade_xml_get_widget (xml, "boot_dialog_help")),
			  "clicked", G_CALLBACK (on_boot_help_button_clicked), NULL);

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
	gboolean valid;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (boot_table));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, BOOT_LIST_COL_POINTER, &node, -1);
	}

	if (gst_tool_get_access (tool)) {
		image  = boot_image_get_by_node (node);
		editor = boot_image_editor_new (image);
		valid  = FALSE;

		while (!valid) {
			response = gtk_dialog_run (editor->dialog);
			switch (response)
			{
			case GTK_RESPONSE_OK:
				if (boot_settings_gui_save (editor->gui, TRUE))
				{
					boot_image_save (editor->gui->image);
					valid = TRUE;
				}
				
				break;
			case GTK_RESPONSE_CANCEL:
				valid = TRUE;
				break;
			default:
				valid = FALSE;
				break;
			}
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

	g_signal_connect (G_OBJECT (glade_xml_get_widget (xml, "boot_dialog_help")),
			  "clicked", G_CALLBACK (on_boot_help_button_clicked), NULL);

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
	gboolean valid;

	if (gst_tool_get_access (tool))
	{
		settings = (BootSettingsGui *) data;
		editor = boot_append_editor_new (settings);
		valid = FALSE;

		while (!valid) {
			response = gtk_dialog_run (editor->dialog);

			switch (response)
			{
			case GTK_RESPONSE_OK:
				if (boot_append_gui_save (editor->gui, &append))
					if (append) {
						gst_ui_entry_set_text (GTK_ENTRY (editor->gui->settings->append), g_strdup (append));
						g_free (append);
					}
				valid = TRUE;
				break;
			case GTK_RESPONSE_CANCEL:
				valid = TRUE;
				break;
			default:
				valid = FALSE;
				break;
			}
		}
		
		gtk_widget_destroy (GTK_WIDGET (editor->dialog));
		
	}
}

void
on_boot_table_default_toggled (GtkWidget *w, gchar *path_str, gpointer data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW (data);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreeIter iter;
	xmlNodePtr node;
	gchar *label;

	gtk_tree_model_get_iter (model, &iter, path);

	/* we get the selected entry iter and unselect it */
	gtk_tree_store_set (GTK_TREE_STORE (model),
			    &default_entry_iter,
			    BOOT_LIST_COL_DEFAULT, GINT_TO_POINTER (FALSE),
			    -1);

	/* we set as selected the current iter */
	gtk_tree_store_set (GTK_TREE_STORE (model),
			    &iter,
			    BOOT_LIST_COL_DEFAULT, GINT_TO_POINTER (TRUE),
			    -1);

	/* we set the current iter as the default */
	gtk_tree_model_get (model, &iter, BOOT_LIST_COL_POINTER, &node, -1);
	label = gst_xml_get_child_content (node, "label");
	gst_xml_set_child_content (gst_xml_doc_get_root (tool->config), "default", label);
	default_entry_iter = iter;

	gst_dialog_modify (tool->main_dialog);
	gtk_tree_path_free (path);
}

void
on_boot_table_cursor_changed (GtkTreeSelection *selection, gpointer data)
{
	callbacks_actions_set_sensitive (TRUE);	
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

gboolean
on_boot_table_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata)
{
	GtkTreePath *path;
	GtkWidget   *popup_menu;

	popup_menu = (GtkWidget*) gdata;

	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
		on_boot_settings_clicked (NULL, NULL);
		return TRUE;
	}
	
	if (event->button == 3)	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			do_popup_menu (popup_menu, event);
		}
		return TRUE;
	}
	return FALSE;
}

void
on_use_password_clicked (GtkWidget *use_password, gpointer gdata)
{
	BootSettingsGui *gui;
	gboolean state;
	
	gui = (BootSettingsGui *) gdata;
	
	state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_password));
	
	gtk_widget_set_sensitive (GTK_WIDGET (gui->password), state);
	gtk_widget_set_sensitive (GTK_WIDGET (gui->password_confirm), state);
	gtk_widget_set_sensitive (GTK_WIDGET (gui->pass_label), state);
	gtk_widget_set_sensitive (GTK_WIDGET (gui->confirm_label), state);
}

void
on_popup_add_activate (GtkAction *action, gpointer callback_data)
{
	on_boot_add_clicked (callback_data, NULL);
}

void
on_popup_settings_activate (GtkAction *action, gpointer callback_data)
{
	on_boot_settings_clicked (callback_data, NULL);
}

void
on_popup_delete_activate (GtkAction *action, gpointer callback_data)
{
	on_boot_delete_clicked (callback_data, NULL);
}

/* Helpers */

void
callbacks_actions_set_sensitive (gboolean state)
{
	if (gst_tool_get_access (tool)) {
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "boot_add"), TRUE);
		gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "boot_delete"),
					  state);
	}
	
	gtk_widget_set_sensitive (gst_dialog_get_widget (tool->main_dialog, "boot_settings"), state);
}

/* Main window callbacks */
void
on_boot_delete_clicked (GtkButton *button, gpointer data)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	xmlNodePtr   node;
	gchar       *label;
	gint         count, reply;
	GtkWidget   *d;
	
	g_return_if_fail (gst_tool_get_access (tool));

	count = boot_image_count (gst_xml_doc_get_root (tool->config));
	if (count <= 1) {
		d = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GST_HIG_MESSAGE_ERROR,
					_("Error deleting boot image"),
					_("Without at least one boot image "
					  "your computer will not start"),
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);
		return;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (boot_table));
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, BOOT_LIST_COL_POINTER, &node, -1);
	}
	
	label = gst_xml_get_child_content (node, "label");

	d = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GST_HIG_MESSAGE_QUESTION,
				NULL,
				_("This may leave this operating system unbootable"),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);
	gst_hig_dialog_set_primary_text (GST_HIG_DIALOG (d),
					 _("Are you sure you want to delete \"%s\"?"), label);

	reply = gtk_dialog_run (GTK_DIALOG (d));

	gtk_widget_destroy (d);
	g_free (label);

	if (reply != GTK_RESPONSE_ACCEPT)
		return;

	gst_xml_element_destroy (node);
	boot_table_update ();
	gst_dialog_modify (tool->main_dialog);
	callbacks_actions_set_sensitive (FALSE);
}

/* Hooks */
gboolean
callbacks_conf_read_failed_hook (GstTool *tool, GstReportLine *rline, gpointer data)
{
	GtkWidget *dialog;

	dialog = gst_hig_dialog_new (GTK_WINDOW (tool->main_dialog),
				     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				     GST_HIG_MESSAGE_ERROR,
				     _("The configuration will show empty"),
				     NULL,
				     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				     NULL);
	gst_hig_dialog_set_primary_text (GST_HIG_DIALOG (dialog),
					 _("The file \"%s\" is missing or could not be read"), rline->argv[0]);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	/* Handled, don't go looking for more hooks to run */
	return TRUE;
}
