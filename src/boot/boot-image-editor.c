/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Tambet Ingo <tambet@ximian.com>
 *
 *  Copyright 2001 Ximian, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include "boot-image-editor.h"

static void boot_image_editor_class_init (BootImageEditorClass *class);
static void boot_image_editor_finalize   (GtkObject *obj);

static GnomeDialogClass *parent_class;


GtkType
boot_image_editor_get_type ()
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo type_info = {
			"BootImageEditor",
			sizeof (BootImageEditor),
			sizeof (BootImageEditorClass),
			(GtkClassInitFunc) boot_image_editor_class_init,
			(GtkObjectInitFunc) NULL,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		type = gtk_type_unique (gnome_dialog_get_type (), &type_info);
	}

	return type;
}

static void
boot_image_editor_class_init (BootImageEditorClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gnome_dialog_get_type ());

	object_class->finalize = boot_image_editor_finalize;
}

static void
boot_image_editor_finalize (GtkObject *obj)
{
	BootImageEditor *editor = (BootImageEditor *) obj;

	boot_settings_gui_destroy (editor->gui);
	((GtkObjectClass *)(parent_class))->finalize (obj);
}

static void
ok_clicked (GtkWidget *widget, gpointer data)
{
	BootImageEditor *editor = data;

	if (boot_settings_gui_save (editor->gui)) {
		boot_image_save (editor->gui->image);
		gtk_widget_destroy (GTK_WIDGET (editor));
	}
}

static void
cancel_clicked (GtkWidget *widget, gpointer data)
{
	BootImageEditor *editor = data;

	gtk_widget_destroy (GTK_WIDGET (editor));
}

static gboolean
construct (BootImageEditor *editor, BootImage *image)
{
	GtkWidget *w;
	
	editor->gui = boot_settings_gui_new (image, GTK_WIDGET (editor));

	if (!editor->gui)
		return FALSE;

	w = glade_xml_get_widget (editor->gui->xml, "boot_settings_editor");
	gtk_widget_reparent (w, GNOME_DIALOG (editor)->vbox);
	
	/* give our dialog an OK button and title */
	gtk_window_set_title (GTK_WINDOW (editor), _("Boot Image Editor"));
	gtk_window_set_policy (GTK_WINDOW (editor), FALSE, TRUE, TRUE);
	gtk_window_set_modal (GTK_WINDOW (editor), TRUE);
	gnome_dialog_append_buttons (GNOME_DIALOG (editor),
				     GNOME_STOCK_BUTTON_OK,
				     GNOME_STOCK_BUTTON_CANCEL,
				     NULL);

	gnome_dialog_button_connect (GNOME_DIALOG (editor), 0 /* OK */,
				     GTK_SIGNAL_FUNC (ok_clicked),
				     editor);
	gnome_dialog_button_connect (GNOME_DIALOG (editor), 1 /* CANCEL */,
				     GTK_SIGNAL_FUNC (cancel_clicked),
				     editor);

	boot_settings_gui_setup (editor->gui, GNOME_DIALOG (editor)->vbox);

	return TRUE;
}

BootImageEditor *
boot_image_editor_new (BootImage *image)
{
	BootImageEditor *new;

	if (!image)
		return NULL;
	
	new = (BootImageEditor *) gtk_type_new (boot_image_editor_get_type ());
	if (construct (new, image))
		return new;
	else {
		gtk_widget_destroy (GTK_WIDGET (new));
		return NULL;
	}
}
