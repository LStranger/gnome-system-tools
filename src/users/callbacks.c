/* callbacks.c: this file is part of users-admin, a helix-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee> and Arturo Espinosa <arturo@helixcode.com>.
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
#include "network.h"

/* Local globals */
static int reply;

/* Prototypes */
static void my_gtk_entry_set_text (void *entry, gchar *str);

/* Main window callbacks */
/* Users tab */

extern void
on_user_settings_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (node = e_table_get_current_user ());
	user_settings_prepare (node);
}

void 
user_passwd_dialog_show (xmlNodePtr node)
{
	GtkWidget *w0;
	gchar *txt, *name;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (node != NULL);
	g_return_if_fail (name = my_xml_get_content (node, "login"));

	w0 = tool_widget_get ("user_passwd_dialog");
	txt = g_strdup_printf (_("Password for User %s"), name);
	g_free (name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_show (w0);
	gtk_object_set_data (GTK_OBJECT (w0), "name", node);

#ifndef HAVE_LIBCRACK
	gtk_widget_hide (tool_widget_get ("user_passwd_quality"));
#endif

}

extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (node = e_table_get_current_user ());

	user_passwd_dialog_show (node);
}

extern void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (tool_get_access());
	user_new_prepare (g_strdup (""));
}

static void
reply_cb (gint val, gpointer data)
{
	reply = val;
	gtk_main_quit ();
}

extern void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt, *name;
	GtkWindow *parent;
	GnomeDialog *dialog;
	xmlNodePtr node;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (node = e_table_get_current_user ());
	g_return_if_fail (name = my_xml_get_content (node, "login"));

	parent = GTK_WINDOW (tool_get_top_window());

	if (!strcmp (name, "root"))
	{
		g_free (name);
		txt = g_strdup (_("The root user must not be deleted."));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
		g_free (txt);
		return;
	}

	txt = g_strdup_printf (_("Are you sure you want to delete user %s?"), name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (name);

	if (reply)
		return;
	else
	{
		e_table_del_user (node);
		tool_set_modified (TRUE);
		user_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")),
				_("Settings for the selected user"));
	}
}


/* Groups tab */

extern void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (node = e_table_get_current_group ());
	group_settings_prepare (node);
}

extern void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (tool_get_access());
	group_new_prepare ();
}

extern void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt, *name;
	GtkWindow *parent;
	GnomeDialog *dialog;
	xmlNodePtr node;

	g_return_if_fail (tool_get_access());
	g_return_if_fail (node = e_table_get_current_group ());
	g_return_if_fail (name = my_xml_get_content (node, "name"));

	parent = GTK_WINDOW (tool_get_top_window ());

	if (!strcmp (name, "root"))
	{
		g_free (name);
		txt = g_strdup ("You shouldn't delete root group!");
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
		g_free (txt);
		return;
	}

	txt = g_strdup_printf (_("Are you sure you want to delete group %s?"), name);
	g_free (name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);

	if (reply)
		return;
	else
	{
		e_table_del_group (node);
		tool_set_modified (TRUE);
		group_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")),
				"Settings for the selected group");
	}
}

/* Network tab. */

extern void
on_network_delete_clicked (GtkWidget *button, gpointer user_data)
{
	xmlNodePtr node;
	GtkWindow *parent;
	GnomeDialog *dialog;
	gchar *name, *buf = NULL;

	g_return_if_fail (node = network_current_node ());

	if (!strcmp (node->name, "group"))
	{
		node = xml_element_find_first (node, "name");
		name = xml_element_get_content (node);
		buf = g_strdup_printf ("Are You sure You want to delete group %s?", name);
		g_free (name);
	}

	if (!strcmp (node->name, "user"))
	{
		node = xml_element_find_first (node, "login");
		name = xml_element_get_content (node);
		buf = g_strdup_printf ("Are You sure You want to delete user %s?", name);
		g_free (name);
	}

	if (!buf)
		return;

	parent = GTK_WINDOW (tool_get_top_window ());
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (buf, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (buf);

	if (reply)
		return;
	else
		network_current_delete ();
}

extern void
on_network_settings_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (node = network_current_node ());

	if (!strcmp (node->name, "group"))
		group_settings_prepare (node);

	else if (!strcmp (node->name, "user"))
		user_settings_prepare (node);
	else
		g_warning ("unknown node.");

	return;
}

extern void
on_network_user_new_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;
	gchar *buf = NULL;

	g_return_if_fail (tool_get_access());
	node = network_current_node ();

	if (node)
	{
		if (!strcmp (node->name, "group"))
		{
			node = xml_element_find_first (node, "name");
			buf = xml_element_get_content (node);
		}
	}

	if (!buf)
		buf = g_strdup ("");

	user_new_prepare (buf);
}

extern void
on_network_group_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (tool_get_access());
	group_new_prepare ();
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
	gtk_object_remove_data (GTK_OBJECT (w0), "new");
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
	gboolean retval;
	GtkWidget *w0;
	guint new;
	
	w0 = tool_widget_get ("user_settings_dialog");
	new = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (w0), "new"));
	
	if (new)
		retval = user_add ();
	else
		retval = user_update ();

	if (retval)
	{
		tool_set_modified(TRUE);
		/* Clean up dialog, it's easiest to just call *_cancel_* function */
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
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (_("The password and its confirmation\nmust match."),
			GTK_WINDOW (win)));
		
		gnome_dialog_run (dialog);
		break;
	 default: /* Quality check problems, with err pointing to a string to the error */
		msg = g_strdup_printf (_("Bad password: %s.\nPlease try with a new password."), err);
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (msg, GTK_WINDOW (win)));
		g_free (msg);
		gnome_dialog_run (dialog);
		break;
	}
	
	my_gtk_entry_set_text (entry1, "");
	my_gtk_entry_set_text (entry2, "");
}

/* Group settings dialog */

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

	/* Clear both lists. Do we have to free some memory also? */

	w0 = tool_widget_get ("group_settings_all");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = tool_widget_get ("group_settings_members");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = tool_widget_get ("group_settings_dialog");
	gtk_object_remove_data (GTK_OBJECT (w0), "new");
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
	gboolean retval;
	GtkWidget *w0;
	guint new;

	w0 = tool_widget_get ("group_settings_dialog");
	new = GPOINTER_TO_UINT (gtk_object_get_data (GTK_OBJECT (w0), "new"));

	if (new)
		retval = group_add ();
	else
	{
		retval = group_update ();
	}

	if (retval)
	{
		tool_set_modified(TRUE);
		/* Clear list and hide dialog */
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
	GtkCList *all, *members;
	gchar *name;
	gchar *entry[2];
	gint row;

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

	if (tool_get_access())
	{
		w0 = tool_widget_get ("group_settings_add");

		if (!clist->selection)
			gtk_widget_set_sensitive (w0, FALSE);
		else
			gtk_widget_set_sensitive (w0, TRUE);
	}
}
	
extern void
on_group_settings_members_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	GtkWidget *w0;

	if (tool_get_access())
	{
		w0 = tool_widget_get ("group_settings_remove");

		if (!clist->selection)
			gtk_widget_set_sensitive (w0, FALSE);
		else
			gtk_widget_set_sensitive (w0, TRUE);
	}
}


void
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

void
group_actions_set_sensitive (gboolean state)
{
	if (tool_get_access())
	{
		gtk_widget_set_sensitive (tool_widget_get ("group_new"), TRUE);
		gtk_widget_set_sensitive (tool_widget_get ("group_delete"), state);
	}
	
	gtk_widget_set_sensitive (tool_widget_get ("group_settings"), state);
}

static void
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}


