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

#include "user-account-editor.h"

static void user_account_editor_class_init (UserAccountEditorClass *class);
static void user_account_editor_finalize   (GtkObject *obj);

static GnomeDialogClass *parent_class;


GtkType
user_account_editor_get_type ()
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo type_info = {
			"UserAccountEditor",
			sizeof (UserAccountEditor),
			sizeof (UserAccountEditorClass),
			(GtkClassInitFunc) user_account_editor_class_init,
			(GtkObjectInitFunc) NULL,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		type = gtk_type_unique (gnome_dialog_get_type (), &type_info);
	}

	return type;
}

static void
user_account_editor_class_init (UserAccountEditorClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gnome_dialog_get_type ());

	object_class->finalize = user_account_editor_finalize;
}

static void
user_account_editor_finalize (GtkObject *obj)
{
	UserAccountEditor *editor = (UserAccountEditor *) obj;

	user_account_gui_destroy (editor->gui);
        ((GtkObjectClass *)(parent_class))->finalize (obj);
}

static void
ok_clicked (GtkWidget *widget, gpointer data)
{
	UserAccountEditor *editor = data;

	if (user_account_gui_save (editor->gui)) {
		user_account_save (editor->gui->account);
		gtk_widget_destroy (GTK_WIDGET (editor));
	}
}

static void
cancel_clicked (GtkWidget *widget, gpointer data)
{
	UserAccountEditor *editor = data;

	gtk_widget_destroy (GTK_WIDGET (editor));
}

static void
construct (UserAccountEditor *editor, UserAccount *account)
{
	editor->gui = user_account_gui_new (account, GTK_WIDGET (editor));

	/* give our dialog an OK button and title */
	gtk_window_set_title (GTK_WINDOW (editor), _("User Account Editor"));
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

	user_account_gui_setup (editor->gui, GNOME_DIALOG (editor)->vbox);
}

UserAccountEditor *
user_account_editor_new (UserAccount *account)
{
	UserAccountEditor *new;

	new = (UserAccountEditor *) gtk_type_new (user_account_editor_get_type ());
	construct (new, account);

	return new;
}
