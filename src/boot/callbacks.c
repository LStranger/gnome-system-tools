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

#include "global.h"
#include "callbacks.h"
#include "transfer.h"
#include "e-table.h"
#include "boot-settings.h"

XstTool *tool;

static int reply;

static void reply_cb (gint val, gpointer data);

/* Main window callbacks */

extern void
on_boot_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;
	gchar *label, *buf;
	GtkWindow *parent;
	GnomeDialog *dialog;
	
	g_return_if_fail (xst_tool_get_access (tool));
	g_return_if_fail (node = get_selected_node ());

	label = xst_xml_get_child_content (node, "label");
	buf = g_strdup_printf (_("Are you sure you want to delete %s?"), label);

	parent = GTK_WINDOW (tool->main_dialog);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (buf, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (label);
	g_free (buf);
	
	if (reply)
		return;
	
	boot_table_delete ();
	xst_dialog_modify (tool->main_dialog);
	actions_set_sensitive (FALSE);
}

extern void
on_boot_default_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;
	gchar *label;

	node = get_selected_node ();
	label = xst_xml_get_child_content (node, "label");

	xst_xml_set_child_content (xst_xml_doc_get_root (tool->config), "default", label);
	boot_table_update ();
	xst_dialog_modify (tool->main_dialog);
}

extern void
on_boot_prompt_toggled (GtkToggleButton *toggle, gpointer user_data)
{
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_timeout"),
						 gtk_toggle_button_get_active (toggle));

	xst_dialog_modify (tool->main_dialog);
}

/* Helpers */

void
actions_set_sensitive (gboolean state)
{
	XstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;

	if (xst_tool_get_access (tool) && complexity == XST_DIALOG_ADVANCED)
	{
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_add"), TRUE);
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_delete"),
							 state);
	}
	
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_settings"), state);
    	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "boot_default"),
						 state);
}

void
boot_settings_dialog_complexity (gboolean state)
{
	GtkRequisition req;
	GtkWidget *win;
	gint i;
	gchar *widgets[] = { "boot_settings_append", "boot_settings_root",
					 "boot_settings_append_label", "boot_settings_root_label",
					 NULL };

	if (!state)
	{
		for (i = 0; widgets[i]; i++)
			gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, widgets[i]));
	}

	else
	{
		for (i = 0; widgets[i]; i++)
			gtk_widget_show (xst_dialog_get_widget (tool->main_dialog, widgets[i]));
	}

	/* Resize to minimum size */

	win = xst_dialog_get_widget (tool->main_dialog, "boot_settings_dialog");
	gtk_widget_size_request (win, &req);
	gtk_window_set_default_size (GTK_WINDOW (win), req.width, req.height);
}

static void
reply_cb (gint val, gpointer data)
{
	reply = val;
	gtk_main_quit ();
}
