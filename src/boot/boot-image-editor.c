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

static void
editor_response (GtkDialog *dialog, gint response, gpointer data)
{
	BootImageEditor *editor = data;

	switch (response)
	{
	case GTK_RESPONSE_ACCEPT:
		if (boot_settings_gui_save (editor->gui, TRUE))
			boot_image_save (editor->gui->image);
		/*else
		  return;*/
		break;
	default:
		break;
	}

	boot_table_update ();
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
	gtk_widget_reparent (w, GTK_DIALOG (editor)->vbox);

	/* give our dialog an OK button and title */
	gtk_window_set_title (GTK_WINDOW (editor), _("Boot Image Editor"));
	gtk_window_set_policy (GTK_WINDOW (editor), FALSE, TRUE, TRUE);
	gtk_window_set_modal (GTK_WINDOW (editor), TRUE);
	gtk_dialog_set_has_separator (GTK_DIALOG (editor), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (editor)), 6);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (editor)->vbox), 12);
	/*gtk_dialog_add_buttons (GTK_DIALOG (editor),
				GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				NULL);*/

	/*g_signal_connect (G_OBJECT (editor), "response",
	  G_CALLBACK (editor_response), editor);*/

	boot_settings_gui_setup (editor->gui, GTK_DIALOG (editor)->vbox);

	return TRUE;
}

BootImageEditor *
boot_image_editor_new (BootImage *image)
{
	BootImageEditor *new;

	if (!image)
		return NULL;
	
	new = (BootImageEditor *) g_type_create_instance (boot_image_editor_get_type ());
	if (construct (new, image))
		return new;
	else {
		gtk_widget_destroy (GTK_WIDGET (new));
		return NULL;
	}
}
