/* boot_settings.c: this file is part of boot-admin, a ximian-setup-tool frontend 
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
#include "boot-settings.h"
#include "e-table.h"

XstTool *tool;

const gchar *boot_types[] = { "Windows NT", "Windows 9x", "dos", "Linux", "Unknown", NULL };

static void
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}

static void
boot_settings_clean (BootSettingsDialog *state)
{
}

static void
boot_settings_populate (BootSettingsDialog *state, xmlNodePtr node)
{
	gint i;
	GList *list = NULL;

	/* Type combo */
	for (i = 0; boot_types[i]; i++)
		list = g_list_prepend (list, (void *)boot_types[i]);

	gtk_combo_set_popdown_strings (state->type, list);

	/* All entries */
	my_gtk_entry_set_text (state->basic_name, boot_value_label (node));
	my_gtk_entry_set_text (state->adv_name, boot_value_label (node));
	my_gtk_entry_set_text (state->type->entry, boot_value_type (node, TRUE));

	if (xst_xml_element_find_first (node, "image"))
	{
		/* We have Linux, so hide device, show image and 'other' frame */
		my_gtk_entry_set_text (state->image_entry, boot_value_image (node, TRUE));

		gtk_widget_show (state->image_label);
		gtk_widget_show (GTK_WIDGET (state->image));

		gtk_widget_hide (state->device_label);
		gtk_widget_hide (GTK_WIDGET (state->device));

		gtk_widget_show (GTK_WIDGET (state->optional));
	}

	else
	{
		my_gtk_entry_set_text (state->device->entry, boot_value_dev (node, TRUE));
		
		gtk_widget_hide (GTK_WIDGET (state->image));
		gtk_widget_hide (state->image_label);
		
		gtk_widget_show (GTK_WIDGET (state->device));
		gtk_widget_show (state->device_label);

		gtk_widget_hide (GTK_WIDGET (state->optional));
	}
		
	if (state->complexity == XST_DIALOG_ADVANCED)
	{
		gtk_widget_hide (GTK_WIDGET (state->basic_name));
		gtk_widget_show (GTK_WIDGET (state->adv_name));
		gtk_widget_show (GTK_WIDGET (state->settings));		
	}

	else if (state->complexity == XST_DIALOG_BASIC)
	{
		gtk_widget_show (GTK_WIDGET (state->basic_name));
		gtk_widget_hide (GTK_WIDGET (state->adv_name));
		gtk_widget_hide (GTK_WIDGET (state->settings));
		gtk_widget_hide (GTK_WIDGET (state->optional));
	}
	
	my_gtk_entry_set_text (state->root, boot_value_root (node));
}

static gboolean
boot_settings_init (BootSettingsDialog *state)
{
	state->dialog = xst_dialog_get_widget (tool->main_dialog, "boot_settings_dialog");
	state->basic_name = GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
											    "settings_b_name"));
	
	state->type = GTK_COMBO (xst_dialog_get_widget (tool->main_dialog, "settings_type"));
	state->settings = xst_dialog_get_widget (tool->main_dialog, "settings_settings");
	state->adv_name = GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
											  "settings_a_name"));

	state->device = GTK_COMBO (xst_dialog_get_widget (tool->main_dialog, "settings_device"));
	state->device_label = xst_dialog_get_widget (tool->main_dialog, "settings_device_label");

	state->image_label = xst_dialog_get_widget (tool->main_dialog, "settings_image_label");
	state->image_entry = GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
												"settings_image_entry"));
	
	state->image = xst_dialog_get_widget (tool->main_dialog, "settings_image");
	state->optional = xst_dialog_get_widget (tool->main_dialog, "settings_optional");
	state->root = GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog, "settings_root"));
	state->complexity = tool->main_dialog->complexity;

	return TRUE;
}

BootSettingsDialog *
boot_settings_prepare (xmlNodePtr node)
{
	BootSettingsDialog *state;

	state = g_new (BootSettingsDialog, 1);
	if (!boot_settings_init (state))
	{
		g_warning ("boot_settings_prepare: can't init dialog");
		g_free (state);
		return NULL;
	}

	if (node)
		boot_settings_populate (state, node);
	else
		boot_settings_clean (state);

	return state;
}

void
boot_settings_affect (BootSettingsDialog *state)
{
	xmlNodePtr node;

	node = get_selected_node ();

	/* Type */
	xst_xml_set_child_content (node, "XstPartitionType",
						  gtk_entry_get_text (GTK_ENTRY (state->type->entry)));
	
	if (state->complexity == XST_DIALOG_ADVANCED)
	{
		boot_value_set_label (node, gtk_entry_get_text (state->adv_name));
		
		if (xst_xml_element_find_first (node, "image"))
			boot_value_set_image (node, gtk_entry_get_text (state->image_entry));
		else
			boot_value_set_dev (node, gtk_entry_get_text (GTK_ENTRY (state->device->entry)));

		boot_value_set_root (node, gtk_entry_get_text (state->root));
	}
	
	else if (state->complexity == XST_DIALOG_BASIC)
	{
		boot_value_set_label (node, gtk_entry_get_text (state->basic_name));
	}
}

extern void
on_boot_settings_clicked (GtkButton *button, gpointer user_data)
{
	BootSettingsDialog *state;
	GtkWidget *d;
	gint res;

	d = xst_dialog_get_widget (tool->main_dialog, "boot_settings_dialog");

	state = boot_settings_prepare (get_selected_node ());
	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));

	if (res)
		return;

	/* affect */
	boot_settings_affect (state);
	boot_table_update ();
	xst_dialog_modify (tool->main_dialog);
}

extern void
on_boot_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;

	d = gnome_error_dialog (_("Not Implemented yet, sorry."));
	gnome_dialog_run (GNOME_DIALOG(d));
	
/*	BootSettingsDialog *state;
	GtkWidget *d;
	xmlNodePtr node;
	gint res;

	d = xst_dialog_get_widget (tool->main_dialog, "boot_settings_dialog");

	state = boot_settings_prepare (NULL);
	res = gnome_dialog_run_and_close (GNOME_DIALOG (d));

	if (res)
		return;

	node = boot_table_add ();
	boot_settings_affect (node);
	boot_table_update ();
	xst_dialog_modify (tool->main_dialog); */
}
