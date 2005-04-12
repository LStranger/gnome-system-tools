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

#include "boot-append-gui.h"
#include "boot-druid.h"

extern GstTool *tool;
extern GtkWidget *boot_table;

gchar *initrd_label_str = N_("_Initrd image path:");
gchar *module_label_str = N_("_Module path:");

static void
settings_dev_list (GtkComboBox *combo, GstBootImageType ctype)
{
	xmlNodePtr    root;
	xmlNodePtr    part;
	xmlNodePtr    node;
	gchar        *type;
	gchar        *dev;
	GtkTreeModel *model;

	root = gst_xml_doc_get_root (tool->config);
	part = gst_xml_element_find_first (root, "partitions");

	model = gtk_combo_box_get_model (combo);
	gtk_list_store_clear (GTK_LIST_STORE (model));

	for (node = gst_xml_element_find_first (part, "partition");
	     node; node = gst_xml_element_find_next (node, "partition"))
	{
		dev = gst_xml_get_child_content (node, "dev");
		type = gst_xml_get_child_content (node, "type");
		if (ctype == label_to_type (type))
			gtk_combo_box_append_text (combo, dev);
	}
}

static void
on_type_entry_change (GtkWidget *w, gpointer data)
{
	BootSettingsGui  *gui;
	GstBootImageType  type;
	BootImage        *image;
	const gchar      *buf;

	gui = (BootSettingsGui *) data;

	buf = boot_settings_get_type (gui);
	
	if (strlen (buf) > 0)
	{
		type = label_to_type (buf);
		if ((type == TYPE_LINUX) || (type == TYPE_HURD))
		{
			settings_dev_list (GTK_COMBO_BOX (gui->root), type);
			image = gui->image;

			if (image->root)
				gst_ui_entry_set_text (GTK_ENTRY (GTK_BIN (gui->root)->child), image->root);
		}
		else if (type != TYPE_LINSWAP)
			settings_dev_list (GTK_COMBO_BOX (gui->device), type);
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
	gui->xml = glade_xml_new (tool->glade_path, NULL, NULL);
	gui->top = parent;

	/* Basic frame */
	gui->basic_frame = glade_xml_get_widget (gui->xml, "settings_basic_frame");
	gui->name = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_name"));
	gui->type = glade_xml_get_widget (gui->xml, "settings_type");

	/* Image frame */
	gui->image_frame = glade_xml_get_widget (gui->xml, "settings_image_frame");
	gui->image_widget = glade_xml_get_widget (gui->xml, "settings_image");
	gui->root = glade_xml_get_widget (gui->xml, "settings_root");
	gui->initrd_label = glade_xml_get_widget (gui->xml, "settings_initrd_label");
	gui->initrd_widget = glade_xml_get_widget (gui->xml, "settings_initrd");
	gui->append = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_append"));
	gui->append_browse = GTK_BUTTON (glade_xml_get_widget (gui->xml, "settings_append_browse"));

	/* Other frame */
	gui->other_frame = glade_xml_get_widget (gui->xml, "settings_other_frame");
	gui->device = glade_xml_get_widget (gui->xml, "settings_device");

	/* Security frame */
	gui->use_password = GTK_CHECK_BUTTON (glade_xml_get_widget (gui->xml, "settings_use_password"));
	gui->pass_label = GTK_LABEL (glade_xml_get_widget (gui->xml, "settings_pass_label"));
	gui->confirm_label = GTK_LABEL (glade_xml_get_widget (gui->xml, "settings_confirm_label"));
	gui->password = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_password"));
	gui->password_confirm = GTK_ENTRY (glade_xml_get_widget (gui->xml, "settings_password_confirm"));

	boot_settings_fill_type_list (gui);

	/* Connect signals */
	g_signal_connect (G_OBJECT (gui->type), "changed",
			  G_CALLBACK (on_type_entry_change), (gpointer) gui);
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

void
boot_settings_fill_type_list (BootSettingsGui *gui)
{
	GtkWidget    *combo;
	GtkTreeModel *model;
	GList *list = NULL;

	xmlNodePtr  root;
	xmlNodePtr  part;
	xmlNodePtr  node;

	gchar *dev;
	gchar *type;

	combo = gui->type;
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	root  = gst_xml_doc_get_root (tool->config);
	part  = gst_xml_element_find_first (root, "partitions");

	for (node = gst_xml_element_find_first (part, "partition");
	     node; node = gst_xml_element_find_next (node, "partition"))
	{
		dev = gst_xml_get_child_content (node, "dev");
		type = type_to_label (label_to_type (gst_xml_get_child_content (node, "type")));
		if ((g_list_find_custom (list, type, (GCompareFunc)compare) == NULL)
		    && (label_to_type (type) != TYPE_LINSWAP))
			gtk_combo_box_append_text (GTK_COMBO_BOX (combo), type);
			list = g_list_append (list, type);
	}

	g_list_free (list);
}

const gchar*
boot_settings_get_type (BootSettingsGui *gui)
{
	GtkComboBox  *combo;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *type;

	combo = GTK_COMBO_BOX (gui->type);
	model = gtk_combo_box_get_model (combo);

	if (gtk_combo_box_get_active_iter (combo, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &type, -1);
		return type;
	}

	return NULL;
}

void
boot_settings_set_type (BootSettingsGui *gui, gchar *type)
{
	GtkComboBox  *combo;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid;
	gchar        *t;

	combo = GTK_COMBO_BOX (gui->type);
	model = gtk_combo_box_get_model (combo);

	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 0, &t, -1);

		if (strcmp (t, type) == 0) {
			gtk_combo_box_set_active_iter (combo, &iter);
			valid = FALSE;
		} else
			valid = gtk_tree_model_iter_next (model, &iter);
	}
}

void
boot_settings_gui_setup (BootSettingsGui *gui, GtkWidget *top)
{
	BootImage *image = gui->image;
	GList *type_list;

	if (image->type == TYPE_LINUX)
	{
		settings_dev_list (GTK_COMBO_BOX (gui->root), image->type);

		if (image->root)
			gst_ui_entry_set_text (GTK_ENTRY (GTK_BIN (gui->root)->child), image->root);

		if (boot_image_valid_initrd (image))
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gui->initrd_widget), "/boot");
		else
			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (gui->initrd_widget), image->initrd);

		gst_ui_entry_set_text (gui->append, image->append);
		boot_settings_set_type (gui, type_to_label (image->type));
	}
	else if (image->type == TYPE_HURD)
	{
		settings_dev_list (GTK_COMBO_BOX (gui->root), image->type);

		if (image->root)
			gst_ui_entry_set_text (GTK_ENTRY (GTK_BIN (gui->root)->child), image->root);

		if (boot_image_valid_module (image))
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gui->initrd_widget), "/boot");
		else
			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (gui->initrd_widget), image->module);

		gst_ui_entry_set_text (gui->append, image->append);
		boot_settings_set_type (gui, type_to_label (image->type));
	}
	else
	{
		settings_dev_list (GTK_COMBO_BOX (gui->device), image->type);

		if (!image->new)
			boot_settings_set_type (gui, type_to_label (image->type));

		if (image->image)
			gst_ui_entry_set_text (GTK_ENTRY (GTK_BIN (gui->device)->child), image->image);
	}
	
	gst_ui_entry_set_text (gui->name, image->label);

	if (boot_image_valid_root (image) || !image->image)
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (gui->image_widget), "/boot");
	else
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (gui->image_widget), image->image);

	gst_ui_entry_set_text (GTK_BIN (gui->device)->child, image->image);

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

	/* only check if the image is new, if it isn't
	   then there's no possibility to change it */
	if (image->new)
		image->type = label_to_type (boot_settings_get_type (gui));

	image->label = g_strdup (gtk_entry_get_text (gui->name));

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gui->use_password)))
		image->password = g_strdup (gtk_entry_get_text (gui->password));
	else
		image->password = g_strdup ("");

	if (image->type == TYPE_LINUX)
	{
		image->root = g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_BIN (gui->root)->child)));
		image->append = g_strdup (gtk_entry_get_text (gui->append));
		image->image = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (gui->image_widget));
		image->initrd = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (gui->initrd_widget));
	}
	else if (image->type == TYPE_HURD)
	{
		image->root = g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_BIN (gui->root)->child)));
		image->append = g_strdup (gtk_entry_get_text (gui->append));
		image->image = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (gui->image_widget));
		image->module = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (gui->initrd_widget));
	}
	else
	{
		image->root = g_strdup ("");
		image->append = g_strdup ("");
		image->image = g_strdup (gtk_entry_get_text (GTK_ENTRY (GTK_BIN (gui->device)->child)));
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

	d = gtk_message_dialog_new (parent,
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_ERROR,
				    GTK_BUTTONS_CLOSE,
				    _("Error modifying boot image"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d), error);

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
