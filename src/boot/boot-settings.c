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

#include "gst.h"
#include "callbacks.h"
#include "boot-settings.h"
#include "table.h"

#include "boot-image-editor.h"
#include "boot-append-gui.h"
#include "boot-append-editor.h"
#include "boot-druid.h"

extern GstTool *tool;
extern GtkWidget *boot_table;

static GList *
settings_dev_list (GstBootImageType ctype)
{
	GList *list = NULL;

	xmlNodePtr  root;
	xmlNodePtr  part;
	xmlNodePtr  node;

	gchar *dev;
	gchar *type;

	root = gst_xml_doc_get_root (tool->config);
	part = gst_xml_element_find_first (root, "partitions");
	for (node = gst_xml_element_find_first (part, "partition");
	     node; node = gst_xml_element_find_next (node, "partition"))
	{
		dev = gst_xml_get_child_content (node, "dev");
		type = gst_xml_get_child_content (node, "type");
		if (ctype == label_to_type (type))
			list = g_list_append (list, g_strdup (dev));
	}

	return (list);
}

static void
on_type_entry_change (GtkWidget *w, gpointer data)
{
	BootSettingsGui *gui;
	const gchar *buf;
	GstBootImageType type;

	gui = (BootSettingsGui *) data;

	buf = gtk_entry_get_text (GTK_ENTRY (gui->type->entry));
	
	if (strlen (buf) > 0)
	{
		type = label_to_type (buf);
		if (type == TYPE_LINUX)
			gtk_combo_set_popdown_strings (gui->root, settings_dev_list (type));
		else if (type != TYPE_LINSWAP)
			gtk_combo_set_popdown_strings (gui->device, settings_dev_list (type));
	}	
	return;
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
	gui->xml = glade_xml_new (tool->glade_path, NULL, NULL);
	gui->top = parent;

	/* Basic frame */
	gui->basic_frame = glade_xml_get_widget (gui->xml, "settings_basic_frame");
	gui->name = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_name"));
	gui->type = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_type"));
	/*gui->type_label = GTK_LABEL (glade_xml_get_widget (gui->xml, "boot_type_label"));*/

	/* Image frame */
	gui->image_frame = glade_xml_get_widget (gui->xml, "settings_image_frame");
	gui->image_widget = glade_xml_get_widget (gui->xml, "settings_image");
	gui->image_entry = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_image_entry"));	
	gui->root = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_root_combo"));
	gui->append = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_append"));
	gui->append_browse = GTK_BUTTON (glade_xml_get_widget (gui->xml, "settings_append_browse"));

	/* Other frame */
	gui->other_frame = glade_xml_get_widget (gui->xml, "settings_other_frame");
	gui->device = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_device"));

	/* Connect signals */
	g_signal_connect (G_OBJECT (gui->name), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->type->entry);
	g_signal_connect (G_OBJECT (gui->type->entry), "changed",
	G_CALLBACK (on_type_entry_change), (gpointer) gui);
	g_signal_connect (G_OBJECT (gui->image_entry), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->root);
	g_signal_connect (G_OBJECT (gui->root->entry), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->append);
	g_signal_connect (G_OBJECT (gui->append_browse), "clicked",
	                  G_CALLBACK (on_boot_append_browse_clicked), (gpointer) gui);

	return gui;
}

static void
setup_advanced (BootSettingsGui *gui, GtkWidget *parent)
{
	if (gui->image->type == TYPE_LINUX)
	{
		gtk_widget_show (gui->image_frame);
		gtk_widget_hide (gui->other_frame);
	}
	else
	{
		gtk_widget_show (gui->other_frame);
		gtk_widget_hide (gui->image_frame);
	}
}

static void
setup_basic (BootSettingsGui *gui, GtkWidget *parent)
{
	if (gui->image->type == TYPE_LINUX)
	{
		gtk_widget_show (gui->image_frame);
		gtk_widget_hide (gui->other_frame);
	}
	else
	{
		gtk_widget_show (gui->other_frame);
		gtk_widget_hide (gui->image_frame);
	}
}

static gint *
compare (gconstpointer a, gconstpointer b)
{
	return (GINT_TO_POINTER(strcmp (a,b)));
}

GList *
settings_type_list (void)
{
	GList *list = NULL;

	xmlNodePtr  root;
	xmlNodePtr  part;
	xmlNodePtr  node;

	gchar *dev;
	gchar *type;
	
	root = gst_xml_doc_get_root (tool->config);
	part = gst_xml_element_find_first (root, "partitions");
	for (node = gst_xml_element_find_first (part, "partition");
	     node; node = gst_xml_element_find_next (node, "partition"))
	{
		dev = gst_xml_get_child_content (node, "dev");
		type = type_to_label (label_to_type (gst_xml_get_child_content (node, "type")));
		if ((g_list_find_custom (list, type, (GCompareFunc)compare) == NULL) &&
		    (label_to_type (type) != TYPE_LINSWAP))
			list = g_list_append (list, g_strdup (type));
	}

	return (list);
}


void
boot_settings_gui_setup (BootSettingsGui *gui, GtkWidget *top)
{
	BootImage *image = gui->image;
	GList *type_list;
	gchar *error;

	if (image->type == TYPE_LINUX)
	{
		gtk_combo_set_popdown_strings (gui->root, settings_dev_list (image->type));
		if (image->root)
		  gst_ui_entry_set_text (GTK_ENTRY (gui->root->entry), image->root);
	}
	else
	{
		gtk_combo_set_popdown_strings (gui->device, settings_dev_list (image->type));
		if (image->image)
		  gst_ui_entry_set_text (GTK_ENTRY (gui->device->entry), image->image);
	}
	
	gst_ui_entry_set_text (gui->type->entry, type_to_label (image->type));
	gst_ui_entry_set_text (gui->name, image->label);
	gst_ui_entry_set_text (gui->append, image->append);

	if (error = boot_image_valid_root (image))
		gst_ui_entry_set_text (gui->image_entry, "");
	else
		gst_ui_entry_set_text (gui->image_entry, image->image);
	gst_ui_entry_set_text (gui->device->entry, image->image);

	if (!image->new) {
		if (gst_dialog_get_complexity (tool->main_dialog) == GST_DIALOG_ADVANCED)
			setup_advanced (gui, top);
		if (gst_dialog_get_complexity (tool->main_dialog) == GST_DIALOG_BASIC)
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

	if (image->type == TYPE_LINUX)
	{
		image->root = g_strdup (gtk_entry_get_text (GTK_ENTRY (gui->root->entry)));
		image->append = g_strdup (gtk_entry_get_text (gui->append));
		image->image = g_strdup (gtk_entry_get_text (gui->image_entry));
	}
	else
	{
		image->root = g_strdup ("");
		image->append = g_strdup ("");
		image->image = g_strdup (gtk_entry_get_text (GTK_ENTRY (gui->device->entry)));
	}

	if (check) {
		gchar *error = boot_image_check (image);
		if (error) {
			boot_settings_gui_error (GTK_WINDOW (gui->top), error);
			return FALSE;
		}
	}
	
	boot_table_update ();
	gst_dialog_modify (tool->main_dialog);

	return TRUE;
}

void
boot_settings_gui_error (GtkWindow *parent, gchar *error)
{
	GtkWidget *d;

	d = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, error);

	g_free (error);
	gtk_dialog_run (GTK_DIALOG (d));
	gtk_widget_destroy (d);
}

void
boot_settings_gui_destroy (BootSettingsGui *gui)
{
	if (gui) {
		boot_image_destroy (gui->image);
		g_object_unref (G_OBJECT (gui->xml));
		g_free (gui);
	}
}
