/* callbacks.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <gnome.h>

#include "global.h"
#include "callbacks.h"
#include "user_group.h"
#include "transfer.h"
#include "passwd.h"
#include "e-table.h"


/* Main window callbacks */
/* Users tab */

extern void
on_user_settings_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (node = get_selected_node ());
	user_settings_prepare (node);
}

extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (node = get_selected_node ());

	user_passwd_dialog_prepare (node);
}

extern void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (tool_get_access ());

	node = get_current_root_node ();
	user_new_prepare (node);
}

extern void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (node = get_selected_node ());

	if (check_login_delete (node))
	{
		tool_set_modified (TRUE);
		if (delete_selected_node ())
			xml_element_destroy (node);

		user_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")),
				"Settings for the selected user");
	}
}

/* Groups tab */

extern void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (node = get_selected_node ());
	group_settings_prepare (node);
}

extern void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (tool_get_access ());

	node = get_current_root_node ();
	group_new_prepare (node);
}

extern void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (tool_get_access());

	node = get_selected_node ();

	if (check_group_delete (node))
	{
		tool_set_modified (TRUE);
		if (delete_selected_node ())
			xml_element_destroy (node);

		group_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")),
				"Settings for the selected group");
	}
}

/* Network tab. */

extern void
on_network_delete_clicked (GtkWidget *button, gpointer user_data)
{
}

extern void
on_network_settings_clicked (GtkButton *button, gpointer user_data)
{
}

extern void
on_network_user_new_clicked (GtkButton *button, gpointer user_data)
{
}

extern void
on_network_group_new_clicked (GtkButton *button, gpointer user_data)
{
}


/* User settings callbacks */

extern void
on_user_settings_dialog_show (GtkWidget *button, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (tool_widget_get ("user_settings_name"));
}

static void
user_settings_dialog_close (void)
{
	GtkWidget *w0;
	GList *list;

	/* Clean up entries */

	w0 = tool_widget_get ("user_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("user_settings_comment");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("user_settings_group");
	list = gtk_container_children (GTK_CONTAINER (w0));
	g_list_free (list);

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
		adv_user_settings (NULL, FALSE);

	w0 = tool_widget_get ("user_settings_dialog");
	gtk_object_remove_data (GTK_OBJECT (w0), "node");
	gtk_widget_hide (w0);
}

extern void     
on_user_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	user_settings_dialog_close ();
}

extern void
on_user_settings_dialog_delete_event (GtkWidget *button, gpointer user_data)
{
	user_settings_dialog_close ();
}

extern void
on_user_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	xmlNodePtr node;

	g_return_if_fail (tool_get_access ());

	w0 = tool_widget_get ("user_settings_dialog");
	node = gtk_object_get_data (GTK_OBJECT (w0), "node");

	if (user_update (node))
	{
		tool_set_modified (TRUE);
		user_settings_dialog_close ();
	}
}

/* Password settings callbacks */

static void
user_passwd_dialog_close (void)
{
	GtkWidget *w0;

	w0 = tool_widget_get ("user_passwd_dialog");
	gtk_widget_hide (w0);
	gtk_object_remove_data (GTK_OBJECT (w0), "name");
}

extern void
on_user_passwd_cancel_clicked (GtkButton *button, gpointer user_data)
{
	user_passwd_dialog_close ();
}

extern void
on_user_passwd_dialog_delete_event (GtkWidget *w, gpointer user_data)
{
	user_passwd_dialog_close ();
}

extern void
on_user_passwd_random_clicked (GtkButton *button, gpointer user_data)
{
	GtkEntry *entry1, *entry2;
	GtkWidget *win;
	GnomeDialog *dialog;
	gchar *txt, *random_passwd;

	win = tool_widget_get ("user_passwd_dialog");
	entry1 = GTK_ENTRY (tool_widget_get ("user_passwd_new"));
	entry2 = GTK_ENTRY (tool_widget_get ("user_passwd_confirmation"));

	random_passwd = passwd_get_random ();

	my_gtk_entry_set_text (entry1, random_passwd);
	my_gtk_entry_set_text (entry2, random_passwd);

	txt = g_strdup_printf (_("Password set to \"%s\"."), random_passwd);
	dialog = GNOME_DIALOG (gnome_ok_dialog_parented (txt, GTK_WINDOW (win)));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (random_passwd);
}

extern void
on_user_passwd_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkEntry *entry1, *entry2;
	GtkToggleButton *quality;
	gchar *new_passwd, *confirm;
	GtkWidget *win;
	GnomeDialog *dialog;
	gchar *msg, *err;
	xmlNodePtr node;

	entry1 = GTK_ENTRY (tool_widget_get ("user_passwd_new"));
	entry2 = GTK_ENTRY (tool_widget_get ("user_passwd_confirmation"));
	quality = GTK_TOGGLE_BUTTON (tool_widget_get ("user_passwd_quality"));
	win = tool_widget_get ("user_passwd_dialog");

	node = gtk_object_get_data (GTK_OBJECT (win), "name");

	new_passwd = gtk_entry_get_text (entry1);
	confirm = gtk_entry_get_text (entry2);

        /* Empty old contnents */

	err = passwd_set (node, new_passwd, confirm, gtk_toggle_button_get_active (quality));
	switch ((int) err)
	{
		case 0: /* The password is OK and has been set */
			gtk_widget_hide (win);
			tool_set_modified (TRUE);
			break;
		case -1: /* Bad confirmation */
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					(_("The password and its confirmation\nmust match."),
					 GTK_WINDOW (win)));

			gnome_dialog_run (dialog);
			break;
		default: /* Quality check problems, with err pointing to a string to the error */
			msg = g_strdup_printf (
					_("Bad password: %s.\nPlease try with a new password."),
					err);

			dialog = GNOME_DIALOG (gnome_error_dialog_parented (msg, GTK_WINDOW (win)));
			g_free (msg);
			gnome_dialog_run (dialog);
			break;
	}

	my_gtk_entry_set_text (entry1, "");
	my_gtk_entry_set_text (entry2, "");
}

/* Group settings callbacks */

extern void
on_group_settings_dialog_show (GtkWidget *widget, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (tool_widget_get ("group_settings_name"));
}

static void
group_settings_dialog_close (void)
{
	GtkWidget *w0;

	/* Clear group name */

	w0 = tool_widget_get ("group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	/* Clear both lists. Do we have to free some memory also? TODO: Yes, we do! */

	w0 = tool_widget_get ("group_settings_all");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = tool_widget_get ("group_settings_members");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = tool_widget_get ("group_settings_dialog");
	gtk_object_remove_data (GTK_OBJECT (w0), "node");
	gtk_widget_hide (w0);
}

extern void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	group_settings_dialog_close ();
}

extern void
on_group_settings_dialog_delete_event (GtkWidget *w, gpointer user_data)
{
	group_settings_dialog_close ();
}

extern void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	xmlNodePtr node;

	g_return_if_fail (tool_get_access ());

	w0 = tool_widget_get ("group_settings_dialog");
	node = gtk_object_get_data (GTK_OBJECT (w0), "node");

	if (group_update (node))
	{
		tool_set_modified (TRUE);
		group_settings_dialog_close ();
	}
}

extern void
on_group_settings_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkCList *all, *members;
	gchar *name;
	gchar *entry[2];
	gint row;

	g_return_if_fail (tool_get_access ());

	entry[1] = NULL;

	all = GTK_CLIST (tool_widget_get ("group_settings_all"));
	members = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = GPOINTER_TO_INT (all->selection->data);

	gtk_clist_get_text (all, row, 0, &name);
	entry[0] = g_strdup (name);
	gtk_clist_remove (all, row);
	gtk_clist_append (members, entry);
}

extern void
on_group_settings_remove_clicked (GtkButton *button, gpointer user_data)
{
	/* TODO: Maybe mix with previous func? */
	GtkCList *all, *members;
	gchar *name;
	gchar *entry[2];
	gint row;

	g_return_if_fail (tool_get_access ());

	entry[1] = NULL;

	all = GTK_CLIST (tool_widget_get ("group_settings_all"));
	members = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = GPOINTER_TO_INT (members->selection->data);

	gtk_clist_get_text (members, row, 0, &name);
	entry[0] = g_strdup (name);
	gtk_clist_remove (members, row);
	gtk_clist_append (all, entry);
}

extern void
on_group_settings_all_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	GtkWidget *w0;

	g_return_if_fail (tool_get_access ());

	w0 = tool_widget_get ("group_settings_add");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}

extern void
on_group_settings_members_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	/* TODO: maybe mix with previous? */
	GtkWidget *w0;

	g_return_if_fail (tool_get_access ());

	w0 = tool_widget_get ("group_settings_remove");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}


/* Helpers .*/

extern void
user_actions_set_sensitive (gboolean state)
{
	if (tool_get_access())
	{
		gtk_widget_set_sensitive (tool_widget_get ("user_new"), TRUE);
		gtk_widget_set_sensitive (tool_widget_get ("user_delete"), state);
		gtk_widget_set_sensitive (tool_widget_get ("user_chpasswd"), state);
	}

	gtk_widget_set_sensitive (tool_widget_get ("user_settings"), state);
}

extern void
group_actions_set_sensitive (gboolean state)
{
	if (tool_get_access())
	{
		gtk_widget_set_sensitive (tool_widget_get ("group_new"), TRUE);
		gtk_widget_set_sensitive (tool_widget_get ("group_delete"), state);
	}

	gtk_widget_set_sensitive (tool_widget_get ("group_settings"), state);
}

void
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}

