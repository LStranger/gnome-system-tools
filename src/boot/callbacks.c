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
#include "e-table.h"

XstTool *tool;

static int reply;

static void
reply_cb (gint val, gpointer data)
{
	reply = val;
	gtk_main_quit ();
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
    	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_default"),
						 state);
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
	xmlNodePtr   node;
	gchar       *label, *buf;
	gint         count;
	GtkWidget   *d;
	
	g_return_if_fail (xst_tool_get_access (tool));

	count = boot_image_count (xst_xml_doc_get_root (tool->config));
	if (count <= 1) {
		d = gnome_error_dialog_parented (_("Without at least one boot image,\n"
						   "your system will not start.\n"),
						 GTK_WINDOW (tool->main_dialog));
		
		gnome_dialog_run_and_close (GNOME_DIALOG (d));
		return;
	}

	g_return_if_fail (node = get_selected_node ());

	label = xst_xml_get_child_content (node, "label");
	buf = g_strdup_printf (_("Are you sure you want to delete %s?"), label);

	d = gnome_question_dialog_parented (buf, reply_cb, NULL,
					    GTK_WINDOW (tool->main_dialog));
	gnome_dialog_run_and_close (GNOME_DIALOG (d));
	g_free (label);
	g_free (buf);
	
	if (reply)
		return;
	
	boot_table_delete ();
	xst_dialog_modify (tool->main_dialog);
	callbacks_actions_set_sensitive (FALSE);
}

void
on_boot_default_clicked (GtkButton *button, gpointer data)
{
	xmlNodePtr node;
	gchar *label;

	node = get_selected_node ();
	label = xst_xml_get_child_content (node, "label");

	xst_xml_set_child_content (xst_xml_doc_get_root (tool->config), "default", label);
	boot_table_update ();
	xst_dialog_modify (tool->main_dialog);
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
on_main_dialog_update_complexity (GtkWidget *main_dialog, gpointer data)
{
	XstTool *tool;
	XstDialogComplexity complexity;

	tool = XST_TOOL (data);

	complexity = XST_DIALOG (main_dialog)->complexity;

	boot_table_update_state ();
	callbacks_buttons_set_visibility (XST_DIALOG (main_dialog));
}

/* Hooks */

gboolean
callbacks_conf_read_failed_hook (XstTool *tool, XstReportLine *rline, gpointer data)
{
	GtkWidget *dialog;
	gchar *txt;

	txt = g_strdup_printf (_("The file ``%s'' is missing or could not be read:\n"
				 "The configuration will show empty."), rline->argv[0]);
	
	dialog = gnome_error_dialog_parented (txt, GTK_WINDOW (tool->main_dialog));
	gnome_dialog_run_and_close (GNOME_DIALOG (dialog));

	g_free (txt);

	/* Handled, don't go looking for more hooks to run */
	return TRUE;
}

