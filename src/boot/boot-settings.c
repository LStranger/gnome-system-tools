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
#include "boot-druid.h"

XstTool *tool;

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
	BootDruid *druid;

	if (xst_tool_get_access (tool)) {
		druid = boot_druid_new ();

		if (druid)
			gtk_widget_show (GTK_WIDGET (druid));
		else {
			gchar *error = g_strdup ("Can't add more images, maximum count reached.");
			
			boot_settings_gui_error (GTK_WINDOW (tool->main_dialog),
						 error);
		}		
	}
}

static void
gui_grab_focus (GtkWidget *w, gpointer data)
{
	GtkWidget *widget = GTK_WIDGET (data);

	gtk_widget_grab_focus (widget);
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

	/* Basic frame */
	gui->basic_frame = glade_xml_get_widget (gui->xml, "settings_basic_frame");
	gui->name = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_name"));
	gui->type = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_type"));

	/* Image frame */
	gui->image_frame = glade_xml_get_widget (gui->xml, "settings_image_frame");
	gui->image_widget = glade_xml_get_widget (gui->xml, "settings_image");
	gui->image_entry = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_image_entry"));	
	gui->root = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_root"));
	gui->append = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_append"));

	/* Other frame */
	gui->other_frame = glade_xml_get_widget (gui->xml, "settings_other_frame");
	gui->device = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_device"));

	/* Connect signals */
	gtk_signal_connect (GTK_OBJECT (gui->name), "activate",
			    GTK_SIGNAL_FUNC (gui_grab_focus), (gpointer) gui->type->entry);
	gtk_signal_connect (GTK_OBJECT (gui->image_entry), "activate",
			    GTK_SIGNAL_FUNC (gui_grab_focus), (gpointer) gui->root);
	gtk_signal_connect (GTK_OBJECT (gui->root), "activate",
			    GTK_SIGNAL_FUNC (gui_grab_focus), (gpointer) gui->append);

	return gui;
}

static void
setup_advanced (BootSettingsGui *gui, GtkWidget *parent)
{
	if (gui->image->type == TYPE_LINUX) {
		gtk_widget_show (gui->image_frame);
		gtk_widget_hide (gui->other_frame);
	} else {
		gtk_widget_hide (gui->image_frame);
		gtk_widget_show (gui->other_frame);
	}
}

static void
setup_basic (BootSettingsGui *gui, GtkWidget *parent)
{
	gtk_widget_hide (gui->other_frame);
	gtk_widget_hide (gui->image_frame);
}

void
boot_settings_gui_setup (BootSettingsGui *gui, GtkWidget *top)
{
	BootImage *image = gui->image;

	gtk_combo_set_popdown_strings (gui->type, type_labels_list ());
	
	xst_ui_entry_set_text (gui->name, image->label);
	xst_ui_entry_set_text (gui->type->entry, _(type_to_label (image->type)));
	xst_ui_entry_set_text (gui->root, image->root);
	xst_ui_entry_set_text (gui->append, image->append);

	xst_ui_entry_set_text (gui->image_entry, image->image);
	xst_ui_entry_set_text (gui->device->entry, image->image);

	if (!image->new) {
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
			setup_advanced (gui, top);
		if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_BASIC)
			setup_basic (gui, top);
	}
}

gboolean
boot_settings_gui_save (BootSettingsGui *gui, gboolean check)
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

	if (check) {
		gchar *error = boot_image_check (image);
		if (error) {
			boot_settings_gui_error (GTK_WINDOW (gui->top), error);
			return FALSE;
		}
	}
	
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
