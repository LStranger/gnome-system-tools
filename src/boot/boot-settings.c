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
#include "gst-hig-dialog.h"
#include "callbacks.h"
#include "boot-settings.h"
#include "table.h"

#include "boot-append-gui.h"
#include "boot-druid.h"

extern GstTool *tool;
extern GtkWidget *boot_table;

gchar *initrd_label_str = N_("_Initrd image path:");
gchar *module_label_str = N_("_Module path:");

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
	BootImage *image;

	gui = (BootSettingsGui *) data;

	buf = gtk_entry_get_text (GTK_ENTRY (gui->type->entry));
	
	if (strlen (buf) > 0)
	{
		type = label_to_type (buf);
		if ((type == TYPE_LINUX) || (type == TYPE_HURD))
		{
			gtk_combo_set_popdown_strings (gui->root, settings_dev_list (type));
			image = gui->image;
			if (image->root)
				gst_ui_entry_set_text (GTK_ENTRY (gui->root->entry), image->root);
		}
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

	/* Image frame */
	gui->image_frame = glade_xml_get_widget (gui->xml, "settings_image_frame");
	gui->image_widget = glade_xml_get_widget (gui->xml, "settings_image");
	gui->image_entry = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_image_entry"));	
	gui->root = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_root_combo"));
	gui->initrd_label = glade_xml_get_widget (gui->xml, "settings_initrd_label");
	gui->initrd_widget = glade_xml_get_widget (gui->xml, "settings_initrd");
	gui->initrd_entry = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_initrd_entry"));
	gui->append = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_append"));
	gui->append_browse = GTK_BUTTON (glade_xml_get_widget (gui->xml, "settings_append_browse"));

	/* Other frame */
	gui->other_frame = glade_xml_get_widget (gui->xml, "settings_other_frame");
	gui->device = GTK_COMBO (glade_xml_get_widget (gui->xml, "settings_device"));

	/* Security frame */
	gui->use_password = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "settings_use_password"));
	gui->pass_label = GTK_LABEL (glade_xml_get_widget (gui->xml, "settings_pass_label"));
	gui->confirm_label = GTK_LABEL (glade_xml_get_widget (gui->xml, "settings_confirm_label"));
	gui->password = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_password"));
	gui->password_confirm = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_password_confirm"));

	/* Connect signals */
	g_signal_connect (G_OBJECT (gui->name), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->type->entry);
	g_signal_connect (G_OBJECT (gui->type->entry), "changed",
			  G_CALLBACK (on_type_entry_change), (gpointer) gui);
	g_signal_connect (G_OBJECT (gui->image_entry), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->root);
	g_signal_connect (G_OBJECT (gui->root->entry), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->initrd_widget);
	g_signal_connect (G_OBJECT (gui->initrd_entry), "activate",
			  G_CALLBACK (gui_grab_focus), (gpointer) gui->append);
	g_signal_connect (G_OBJECT (gui->append_browse), "clicked",
	                  G_CALLBACK (on_boot_append_browse_clicked), (gpointer) gui);
	g_signal_connect (G_OBJECT (gui->use_password), "clicked",
			  G_CALLBACK (on_use_password_clicked), (gpointer) gui);

	return gui;
}

static void
setup_gui (BootSettingsGui *gui, GtkWidget *parent)
{
	if (gui->image->type == TYPE_LINUX)
	{
		gtk_widget_show (gui->image_frame);
		gtk_widget_hide (gui->other_frame);

		gtk_label_set_text (GTK_LABEL (gui->initrd_label), _(initrd_label_str));
		gtk_label_set_use_underline (GTK_LABEL (gui->initrd_label), TRUE);
	}
	else if (gui->image->type == TYPE_HURD)
	{
		gtk_widget_show (gui->image_frame);
		gtk_widget_hide (gui->other_frame);

		gtk_label_set_text (GTK_LABEL (gui->initrd_label), _(module_label_str));
		gtk_label_set_use_underline (GTK_LABEL (gui->initrd_label), TRUE);
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

		if (error = boot_image_valid_initrd (image))
			gst_ui_entry_set_text (gui->initrd_entry, "");
		else
			gst_ui_entry_set_text (gui->initrd_entry, image->initrd);

		gst_ui_entry_set_text (gui->append, image->append);
	}
	else if (image->type == TYPE_HURD)
	{
		gtk_combo_set_popdown_strings (gui->root, settings_dev_list (image->type));
		if (image->root)
			gst_ui_entry_set_text (GTK_ENTRY (gui->root->entry), image->root);

		if (error = boot_image_valid_module (image))
			gst_ui_entry_set_text (gui->initrd_entry, "");
		else
			gst_ui_entry_set_text (gui->initrd_entry, image->module);

		gst_ui_entry_set_text (gui->append, image->append);
	}
	else
	{
		gtk_combo_set_popdown_strings (gui->device, settings_dev_list (image->type));
		if (image->image)
		  gst_ui_entry_set_text (GTK_ENTRY (gui->device->entry), image->image);
	}
	
	gst_ui_entry_set_text (gui->type->entry, type_to_label (image->type));
	gst_ui_entry_set_text (gui->name, image->label);

	if (error = boot_image_valid_root (image))
		gst_ui_entry_set_text (gui->image_entry, "");
	else
		gst_ui_entry_set_text (gui->image_entry, image->image);

	gst_ui_entry_set_text (gui->device->entry, image->image);

	if (image->password != NULL)
	{
		gst_ui_entry_set_text (gui->password, image->password);
		gst_ui_entry_set_text (gui->password_confirm, image->password);
	}
	
	if ((image->password == NULL) || (strlen (image->password) <= 0))
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->use_password),
					      FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->password), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->password_confirm), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->pass_label), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->confirm_label), FALSE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->use_password),
					      TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->password), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->password_confirm), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->pass_label), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->confirm_label), TRUE);
	}
	
	if (!image->new)
		setup_gui (gui, top);
}

gboolean
boot_settings_gui_save (BootSettingsGui *gui, gboolean check)
{
	gchar *msg_error;
	BootImage *image = gui->image;

	if (image->label)    g_free (image->label);
	if (image->image)    g_free (image->image);
	if (image->root)     g_free (image->root);
	if (image->initrd)   g_free (image->initrd);
	if (image->append)   g_free (image->append);
	if (image->password) g_free (image->password);

	image->type = label_to_type (gtk_entry_get_text (GTK_ENTRY (gui->type->entry)));
	image->label = g_strdup (gtk_entry_get_text (gui->name));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->use_password)))
		image->password = g_strdup (gtk_entry_get_text (gui->password));
	else
		image->password = g_strdup ("");

	if (image->type == TYPE_LINUX)
	{
		image->root = g_strdup (gtk_entry_get_text (GTK_ENTRY (gui->root->entry)));
		image->append = g_strdup (gtk_entry_get_text (gui->append));
		image->image = g_strdup (gtk_entry_get_text (gui->image_entry));
		image->initrd = g_strdup (gtk_entry_get_text (gui->initrd_entry));
	}
	else if (image->type == TYPE_HURD)
	{
		image->root = g_strdup (gtk_entry_get_text (GTK_ENTRY (gui->root->entry)));
		image->append = g_strdup (gtk_entry_get_text (gui->append));
		image->image = g_strdup (gtk_entry_get_text (gui->image_entry));
		image->module = g_strdup (gtk_entry_get_text (gui->initrd_entry));
	}
	else
	{
		image->root = g_strdup ("");
		image->append = g_strdup ("");
		image->image = g_strdup (gtk_entry_get_text (GTK_ENTRY (gui->device->entry)));
		image->initrd = g_strdup ("");
	}

	if (check) {
		msg_error = boot_image_check (image);

		if ((!msg_error) && (strcmp (gtk_entry_get_text (gui->password),
					     gtk_entry_get_text (gui->password_confirm)) != 0))
			msg_error = g_strdup (_("Password confirmation is not correct"));

		if (msg_error) {
			boot_settings_gui_error (GTK_WINDOW (gui->top), msg_error);
			g_free (msg_error);

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

	d = gst_hig_dialog_new (parent,
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GST_HIG_MESSAGE_ERROR,
				_("Error modifying boot image"),
				error,
				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				NULL);
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
