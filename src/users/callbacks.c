/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
#include "user_settings.h"
#include "profile.h"

XstTool *tool;

/* Main window callbacks */
/* "global" callbacks :) */

void
on_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
			 guint page_num, gpointer user_dat)
{
	set_active_table (page_num);
}

void
on_showall_toggled (GtkToggleButton *toggle, gpointer user_data)
{
	tables_update_content ();
}

void
on_settings_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	ud = g_new (ug_data, 1);

	ud->new = FALSE;
	ud->table = GPOINTER_TO_INT (user_data);
	ud->node = get_selected_node ();

	settings_prepare (ud);
}

/* Users tab */

void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));
	g_return_if_fail (node = get_selected_node ());

	user_passwd_dialog_prepare (node);
}

void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	user_settings_prepare (get_root_node (TABLE_USER));
}

void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));
	g_return_if_fail (node = get_selected_node ());

	if (check_login_delete (node))
	{
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (TABLE_USER))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_USER, FALSE);
	}
}

/* Groups tab */

void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	ud = g_new (ug_data, 1);

	ud->new = TRUE;
	ud->table = TABLE_GROUP;
	ud->node = get_root_node (ud->table);

	group_new_prepare (ud);
}

void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));

	node = get_selected_node ();

	if (check_group_delete (node))
	{
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (TABLE_GROUP))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_GROUP, FALSE);
	}
}

/* Network tab. */

void
on_network_delete_clicked (GtkWidget *button, gpointer user_data)
{
	xmlNodePtr node;
	gboolean delete = FALSE;
	gint tbl = -1;

	g_return_if_fail (xst_tool_get_access (tool));

	node = get_selected_node ();

	/* Is it user or group? */

	if (!strcmp (node->name, "user"))
	{
		delete = check_login_delete (node);
		tbl = TABLE_NET_USER;
	}
	else if (!strcmp (node->name, "group"))
	{
		delete = check_group_delete (node);
		tbl = TABLE_NET_GROUP;
	}

	if (delete)
	{
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (tbl))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_GROUP, FALSE);
	}
}

void
on_network_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	user_settings_prepare (get_root_node (TABLE_NET_USER));
}

void
on_network_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	ud = g_new (ug_data, 1);

	ud->new = TRUE;
	ud->table = TABLE_NET_GROUP;
	ud->node = get_root_node (ud->table);

	group_new_prepare (ud);
}

/* Profiles tab */

void
on_pro_name_changed (GtkEditable *editable, gpointer user_data)
{
	gchar *buf;

	buf = gtk_editable_get_chars (editable, 0, -1);
	profile_table_set_selected (buf);
}

void
on_pro_del_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));
	
	if (profile_table_del_profile (NULL))
		xst_dialog_modify (tool->main_dialog);
}

void
on_pro_save_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));
	
	profile_save (NULL);
	xst_dialog_modify (tool->main_dialog);
}

enum
{
	PROFILE_ERROR,
	PROFILE_NEW,
	PROFILE_COPY,
};

static void
pro_ask_name (gchar *string, gpointer user_data)
{
	Profile *pf;
	gint action;

	if (!string)
		return;

	pf = NULL;
	action = GPOINTER_TO_INT (user_data);

	switch (action)
	{
	case PROFILE_NEW:
		break;
		
	case PROFILE_COPY:
		pf = profile_table_get_profile (NULL);
		break;
		
	case PROFILE_ERROR:
	default:
		g_warning ("pro_ask_name: Shouldn't be here");
		return;
	}

	if (profile_add (pf, string, TRUE))
		xst_dialog_modify (tool->main_dialog);
	
	g_free (string);
}

void
on_pro_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;

	g_return_if_fail (xst_tool_get_access (tool));

	d = gnome_request_dialog (FALSE,
				  N_("Name of new profile"),
				  NULL,
				  15,
				  pro_ask_name,
				  GINT_TO_POINTER (PROFILE_NEW),
				  GTK_WINDOW (tool->main_dialog));

	gnome_dialog_run (GNOME_DIALOG (d));
}

void
on_pro_copy_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;

	g_return_if_fail (xst_tool_get_access (tool));
	
	d = gnome_request_dialog (FALSE,
				  N_("Name of new profile"),
				  NULL,
				  15,
				  pro_ask_name,
				  GINT_TO_POINTER (PROFILE_COPY),
				  GTK_WINDOW (tool->main_dialog));

	gnome_dialog_run (GNOME_DIALOG (d));
}

/* User settings callbacks */

void
on_user_settings_dialog_show (GtkWidget *button, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_name"));
}

static void
user_settings_dialog_close (UserSettings *us)
{
	/* Clean up entries */
	
	gtk_entry_set_text (us->basic->name, "");
	gtk_entry_set_text (us->basic->comment, "");
	gtk_entry_set_text (us->basic->shell, "");
	gtk_entry_set_text (us->basic->home, "");
	gtk_spin_button_set_value (us->basic->uid, 0);
	gtk_widget_hide    (us->basic->advanced);
	gtk_clist_clear    (us->group->member);
	gtk_clist_clear    (us->group->all);
	gtk_widget_hide    (GTK_WIDGET (us->dialog));
    
	user_settings_destroy (us);
}

void
on_user_settings_dialog_delete_event (GnomeDialog *dialog, gpointer user_data)
{
	UserSettings *us;

	us = gtk_object_get_data (GTK_OBJECT (dialog), "UserSettings");
	user_settings_dialog_close (us);
}

void
on_user_settings_clicked (GnomeDialog *dialog, gint button_number, gpointer user_data)
{
	UserSettings *us;
	
	g_return_if_fail (xst_tool_get_access (tool));

	us = gtk_object_get_data (GTK_OBJECT (dialog), "UserSettings");

	switch (button_number)
	{
	case 0:
		user_settings_helper (us);
		if (user_update (us))
		{
			xst_dialog_modify (tool->main_dialog);
			user_settings_dialog_close (us);
		}
		break;
	default:
		user_settings_dialog_close (us);
		break;
	}
}

void
on_user_settings_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "user_settings_gall"));
	members = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "user_settings_gmember"));

	while (all->selection)
	{
		row = GPOINTER_TO_INT (all->selection->data);
		gtk_clist_get_text (all, row, 0, &name);
		my_gtk_clist_append (members, g_strdup (name));
		gtk_clist_remove (all, row);
	}
}

void
on_user_settings_remove_clicked (GtkButton *button, gpointer user_data)
{
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "user_settings_gall"));
	members = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "user_settings_gmember"));

	while (members->selection)
	{
		row = GPOINTER_TO_INT (members->selection->data);
		gtk_clist_get_text (members, row, 0, &name);
		my_gtk_clist_append (all, g_strdup (name));
		gtk_clist_remove (members, row);
	}
}

void
on_user_settings_gmember_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
				     gpointer user_data)
{
	GtkWidget *remove, *primary;

	g_return_if_fail (xst_tool_get_access (tool));

	remove = xst_dialog_get_widget (tool->main_dialog, "user_settings_remove");
	primary = xst_dialog_get_widget (tool->main_dialog, "user_settings_primary");

	if (!clist->selection)
	{
		gtk_widget_set_sensitive (remove,  FALSE);
		gtk_widget_set_sensitive (primary, FALSE);
	}
	
	else
	{
		gtk_widget_set_sensitive (remove,  TRUE);
		gtk_widget_set_sensitive (primary, TRUE);
	}
}

void
on_user_settings_gall_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
				     gpointer user_data)
{
	GtkWidget *w0;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "user_settings_add");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}

static void
passwd_change (gchar *string, gpointer user_data)
{
	GtkToggleButton *quality;
	gchar *err;
	xmlNodePtr node;
	
	if (!string)
		return; /* Cancel clicked */

	node = user_data;

	quality = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (tool->main_dialog, "user_passwd_quality"));
	
	err = passwd_set (node, string, gtk_toggle_button_get_active (quality));
	switch ((int) err)
	{
	case 0: /*	 The password is OK and has been set */
		xst_dialog_modify (tool->main_dialog);
		break;

	default: /* Quality check problems, with err pointing to a string to the error */
	{
		GtkWidget *parent, *d;
		gchar *msg;
		
		msg = g_strdup_printf (
			_("Bad password: %s.\nPlease try with a new password."),
			err);
		
		parent = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");
		d = gnome_error_dialog_parented (msg, GTK_WINDOW (parent));
		gnome_dialog_run (GNOME_DIALOG (d));
		g_free (msg);
		break;
	}
	}
}

void
on_user_passwd_change_clicked (GtkButton *button, gpointer user_data)
{
	user_password_change (get_selected_node ());
}

void
user_password_change (xmlNodePtr user_node)
{
	GtkWidget *d;
	GtkWidget *parent;

	parent = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");

	d = password_request_dialog (N_("Insert password"), 5,
				     passwd_change, user_node,
				     GTK_WINDOW (parent));

	gnome_dialog_run (GNOME_DIALOG (d));
}

void
on_user_passwd_random_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *win;
	GnomeDialog *dialog;
	gchar *txt, *random_passwd;

	win = xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog");

	random_passwd = passwd_get_random ();

	txt = g_strdup_printf (_("Password set to \"%s\"."), random_passwd);
	dialog = GNOME_DIALOG (gnome_ok_dialog_parented (txt, GTK_WINDOW (win)));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (random_passwd);

	xst_dialog_modify (tool->main_dialog);
}

/* Group settings callbacks */

void
on_group_settings_dialog_show (GtkWidget *widget, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "group_settings_name"));
}

static void
group_settings_dialog_close (void)
{
	GtkWidget *w0;
	ug_data *ud;

	/* Clear group name */

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	/* Clear both lists. */

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_all");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	ud = gtk_object_get_data (GTK_OBJECT (w0), "data");
	g_free (ud);
	gtk_object_remove_data (GTK_OBJECT (w0), "data");
	gtk_widget_hide (w0);
}

void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	group_settings_dialog_close ();
}

void
on_group_settings_dialog_delete_event (GtkWidget *w, gpointer user_data)
{
	group_settings_dialog_close ();
}

void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	ud = gtk_object_get_data (GTK_OBJECT (w0), "data");

	if (group_update (ud))
	{
		xst_dialog_modify (tool->main_dialog);
		group_settings_dialog_close ();
	}
}

void
on_group_settings_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_all"));
	members = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));

	while (all->selection)
	{
		row = GPOINTER_TO_INT (all->selection->data);
		gtk_clist_get_text (all, row, 0, &name);
		my_gtk_clist_append (members, g_strdup (name));
		gtk_clist_remove (all, row);
	}
}

void
on_group_settings_remove_clicked (GtkButton *button, gpointer user_data)
{
	/* TODO: Maybe mix with previous func? */
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_all"));
	members = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));

	while (members->selection)
	{
		row = GPOINTER_TO_INT (members->selection->data);
		gtk_clist_get_text (members, row, 0, &name);
		my_gtk_clist_append (all, g_strdup (name));
		gtk_clist_remove (members, row);
	}
}

void
on_group_settings_all_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	GtkWidget *w0;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_add");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}

void
on_group_settings_members_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	/* TODO: maybe mix with previous? */
	GtkWidget *w0;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_remove");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}


/* Helpers .*/

void
actions_set_sensitive (gint table, gboolean state)
{
	switch (table)
	{
	case TABLE_USER:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_new",      TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_delete",   state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_settings", state);
		break;
	case TABLE_GROUP:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_new",      TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_delete",   state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_settings", state);
		break;
	case TABLE_NET_USER:
	case TABLE_NET_GROUP:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_group_new", TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_user_new",  TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_delete",    state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_settings",  state);
		break;
	default:
		g_warning ("actions_set_sensitive: Shouldn't be here.");
		return;
	}
}

void
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}

