/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

#include "xst.h"
#include "callbacks.h"
#include "boot-settings.h"
#include "e-table.h"

#include "boot-image-editor.h"

XstTool *tool;

/* TODO: use struct from e-table.c */
const gchar *boot_types[] = { "Windows NT", "Windows 9x", "Dos", "Linux", "Unknown", NULL };

void
on_boot_settings_clicked (GtkButton *button, gpointer data)
{
	BootImage *image;
	BootImageEditor *editor;
	xmlNodePtr node = get_selected_node ();

	if (xst_tool_get_access (tool)) {
		image = boot_image_get_by_node (node);
		editor = boot_image_editor_new (image);

		gtk_widget_show (GTK_WIDGET (editor));
	}
}

void
on_boot_add_clicked (GtkButton *button, gpointer data)
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

BootSettingsGui *
boot_settings_gui_new (BootImage *image, GtkWidget *parent)
{
	BootSettingsGui *gui;

	if (!image)
		return NULL;

	gui = g_new0 (BootSettingsGui, 1);
	gui->image = image;
	gui->xml = glade_xml_new (tool->glade_path, NULL);
	gui->top = parent;

	gui->name = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_name"));
	gui->type = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_type"));

	gui->device = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_device"));
	gui->device_label = glade_xml_get_widget (gui->xml, "settings_device_label");

	gui->image_label = glade_xml_get_widget (gui->xml, "settings_image_label");
	gui->image_entry = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_image_entry"));	
	gui->image_widget = glade_xml_get_widget (gui->xml, "settings_image");
	
	gui->optional = glade_xml_get_widget (gui->xml, "settings_optional_frame");
	gui->root = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_root"));
	gui->append = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_append"));

	/* TODO: connect signals for checks, keyboard nav, etc */

	return gui;
}

static void
gui_setup_boot_types (BootSettingsGui *gui)
{
	gint i;
	GList *list = NULL;

	for (i = 0; boot_types[i]; i++)
		list = g_list_prepend (list, (void *)boot_types[i]);

	gtk_combo_set_popdown_strings (gui->type, list);
}

static void
setup_advanced_add (BootSettingsGui *gui, GtkWidget *parent)
{
}

static void
setup_advanced (BootSettingsGui *gui, GtkWidget *parent)
{
	if (gui->image->type == TYPE_LINUX) {
		gtk_widget_hide (gui->device_label);
		gtk_widget_hide (GTK_WIDGET (gui->device));

		gtk_widget_show (GTK_WIDGET (gui->optional));
		gtk_widget_show (gui->image_label);
		gtk_widget_show (GTK_WIDGET (gui->image_widget));
	} else {
		gtk_widget_hide (gui->image_label);
		gtk_widget_hide (GTK_WIDGET (gui->image_widget));
		gtk_widget_hide (GTK_WIDGET (gui->optional));

		gtk_widget_show (gui->device_label);
		gtk_widget_show (GTK_WIDGET (gui->device));
	}
}

static void
setup_basic (BootSettingsGui *gui, GtkWidget *parent)
{
	gtk_widget_hide (GTK_WIDGET (gui->optional));
	gtk_widget_hide (gui->image_label);		
	gtk_widget_hide (GTK_WIDGET (gui->image_widget));
	gtk_widget_hide (gui->device_label);
	gtk_widget_hide (GTK_WIDGET (gui->device));
}

void
boot_settings_gui_setup (BootSettingsGui *gui, GtkWidget *top)
{
	BootImage *image = gui->image;

	gui_setup_boot_types (gui);
	
	xst_ui_entry_set_text (gui->name, image->label);
	xst_ui_entry_set_text (gui->type->entry, type_to_label (image->type));
	xst_ui_entry_set_text (gui->root, image->root);
	xst_ui_entry_set_text (gui->append, image->append);

	xst_ui_entry_set_text (gui->image_entry, image->image);
	xst_ui_entry_set_text (gui->device->entry, image->image);

	if (image->new) {
		setup_advanced_add (gui, top);
	} else {
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
			setup_advanced (gui, top);
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_BASIC)
			setup_basic (gui, top);
	}
}

gboolean
boot_settings_gui_save (BootSettingsGui *gui)
{
	BootImage *image = gui->image;

	if (image->label) g_free (image->label);
	if (image->image) g_free (image->image);
	if (image->root) g_free (image->root);
	if (image->append) g_free (image->append);

	image->type = label_to_type (gtk_entry_get_text (GTK_ENTRY (gui->type->entry)));
	image->label = g_strdup (gtk_entry_get_text (gui->name));	
	image->root = g_strdup (gtk_entry_get_text (gui->root));
	image->append = g_strdup (gtk_entry_get_text (gui->append));

	if (image->type == TYPE_LINUX)
		image->image = g_strdup (gtk_entry_get_text (gui->image_entry));
	else
		image->image = g_strdup (gtk_entry_get_text (GTK_ENTRY (gui->device->entry)));

	boot_table_update ();
	xst_dialog_modify (tool->main_dialog);

	return TRUE;
}

void
boot_settings_gui_error (GtkWindow *parent, gchar *error)
{
	GtkWidget *d;

	if (parent)
		d = gnome_error_dialog_parented (error, parent);
	else
		d = gnome_error_dialog (error);

	gnome_dialog_run (GNOME_DIALOG (d));
	g_free (error);
}

void
boot_settings_gui_destroy (BootSettingsGui *gui)
{
	if (gui) {
		boot_image_destroy (gui->image);
		gtk_object_unref (GTK_OBJECT (gui->xml));
		g_free (gui);
	}
}
