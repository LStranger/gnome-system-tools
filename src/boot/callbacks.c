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

#include "xst.h"
#include "xst-report-hook.h"
#include "callbacks.h"
#include "transfer.h"
#include "table.h"

extern XstTool *tool;
extern GtkWidget *boot_table;
extern GtkTreeIter default_entry_iter;

extern GdkPixbuf *selected_icon;
extern GdkPixbuf *not_selected_icon;

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
		label = xst_xml_get_child_content (node, "label");
		xst_xml_set_child_content (xst_xml_doc_get_root (tool->config), "default", label);
		default_entry_iter = iter;

		xst_dialog_modify (tool->main_dialog);
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

/* Helpers */

void
callbacks_actions_set_sensitive (gboolean state)
{
	XstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;

	if (xst_tool_get_access (tool) && complexity == XST_DIALOG_ADVANCED) {
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_add"), TRUE);
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_delete"),
					  state);
	}
	
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_settings"), state);
}

static void
callbacks_buttons_set_visibility (XstDialog *main_dialog)
{
	switch (xst_dialog_get_complexity (main_dialog)) {
	case XST_DIALOG_ADVANCED:
		gtk_widget_show (xst_dialog_get_widget (main_dialog, "boot_add"));
		gtk_widget_show (xst_dialog_get_widget (main_dialog, "boot_delete"));
		break;
	case XST_DIALOG_BASIC:
		gtk_widget_hide (xst_dialog_get_widget (main_dialog, "boot_add"));
		gtk_widget_hide (xst_dialog_get_widget (main_dialog, "boot_delete"));
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
	
	g_return_if_fail (xst_tool_get_access (tool));

	count = boot_image_count (xst_xml_doc_get_root (tool->config));
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
	
	label = xst_xml_get_child_content (node, "label");
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

	xst_xml_element_destroy (node);
	boot_table_update ();
	xst_dialog_modify (tool->main_dialog);
	callbacks_actions_set_sensitive (FALSE);
}

void
on_boot_prompt_toggled (GtkToggleButton *toggle, gpointer data)
{
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_timeout"),
						 gtk_toggle_button_get_active (toggle));

	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_timeout_label"),
						 gtk_toggle_button_get_active (toggle));

	xst_dialog_modify (tool->main_dialog);
}

void
on_main_dialog_update_complexity (XstDialog *main_dialog, gpointer data)
{
	XstDialogComplexity complexity = xst_dialog_get_complexity (tool->main_dialog);
	
	boot_table_update_state (complexity);
	
	callbacks_buttons_set_visibility (tool->main_dialog);
}

/* Hooks */

gboolean
callbacks_conf_read_failed_hook (XstTool *tool, XstReportLine *rline, gpointer data)
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

