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

XstTool *tool;

static int reply;

const gchar *boot_types[] = { "Windows NT", "Windows 9x", "dos", "Linux", NULL };

static void boot_settings_dialog_clean (void);
static void boot_settings_dialog_prepare (void);
static gboolean boot_settings_dialog_affect (void);

static void my_gtk_entry_set_text (void *entry, gchar *str);
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

	label = xml_get_child_content (node, "label");
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
on_boot_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;
	gint res;

	d = xst_dialog_get_widget (tool->main_dialog, "boot_settings_dialog");

	boot_settings_dialog_prepare ();
	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));

	if (res)
		return;

	/* affect */
	boot_settings_dialog_affect ();
	boot_table_update ();
	xst_dialog_modify (tool->main_dialog);
}

extern void
on_boot_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;
	gint res;

	d = xst_dialog_get_widget (tool->main_dialog, "boot_settings_dialog");

	boot_settings_dialog_clean ();
	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));

	if (res)
		return;

	/* affect */
	boot_table_add ();
	boot_settings_dialog_affect ();
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

static void
boot_settings_dialog_clean (void)
{
	gint i;
	gchar *widget[] = { "boot_settings_label", "boot_settings_image", "boot_settings_append",
					"boot_settings_root", NULL };

	for (i = 0; widget[i]; i++)
		gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, widget[i])),"");

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xst_dialog_get_widget
										    (tool->main_dialog,
											"boot_settings_default")), FALSE);
}

static void
boot_settings_dialog_prepare (void)
{
	xmlNodePtr node;
	GList *list = NULL;
	gchar *buf, *image_label;
	gint i;

	node = get_selected_node ();

	/* Label */
	my_gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
											    "boot_settings_label")),
					(gchar *)boot_value_label (node));

	/* Image */

	buf = (gchar *)boot_value_image (node);

	if (!buf)
	{
		buf = (gchar *)boot_value_dev (node);
		image_label = g_strdup (_("Device:"));
		boot_settings_dialog_complexity (FALSE); /* To hide append and root */
	}

	else
	{
		image_label = g_strdup (_("Image:"));
		boot_settings_dialog_complexity (tool->main_dialog->complexity == XST_DIALOG_ADVANCED);
	}

	gtk_label_set_text (GTK_LABEL (xst_dialog_get_widget (tool->main_dialog,
											    "boot_settings_image_label")),
					image_label);

	my_gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												  "boot_settings_image")), buf);

	/* Type combo */
	
	for (i = 0; boot_types[i]; i++)
		list = g_list_prepend (list, (void *)boot_types[i]);

	gtk_combo_set_popdown_strings (GTK_COMBO (xst_dialog_get_widget (tool->main_dialog,
														"boot_settings_type")),
							 list);
	
	/* Default toggle */

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xst_dialog_get_widget
										    (tool->main_dialog,
											"boot_settings_default")),
							(gboolean)boot_value_default (node));

}

static gboolean
boot_settings_dialog_affect (void)
{
	xmlNodePtr node;

	node = get_selected_node ();

	boot_value_set_label (node, gtk_entry_get_text
					  (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												  "boot_settings_label"))));
	
	boot_value_set_image (node, gtk_entry_get_text
					  (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												  "boot_settings_image"))));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xst_dialog_get_widget
											   (tool->main_dialog,
											    "boot_settings_default"))))
	{
		boot_value_set_default (node);
	}
	
	return TRUE;
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
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}


static void
reply_cb (gint val, gpointer data)
{
	reply = val;
	gtk_main_quit ();
}
