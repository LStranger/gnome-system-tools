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

/* Local globals */
static int reply;

/* Prototypes */
static void reply_cb (gint val, gpointer data);
static void do_quit (void);
static void my_gtk_entry_set_text (void *entry, gchar *str);


/* Main button callbacks */

extern void 
on_close_clicked(GtkButton *button, gpointer data)
{
	do_quit();
}

void
on_apply_clicked (GtkButton *button, gpointer user_data)
{
	transfer_gui_to_xml (xml_doc_get_root (tool_config_get_xml()));
 	tool_config_save();
	tool_set_modified(FALSE);
}

/* Main window callbacks */
/* Users tab */

extern void 
on_users_admin_delete_event (GtkWidget * widget, GdkEvent * event, gpointer gdata)
{
	 do_quit ();
}

extern void
on_user_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	GList *tmp_list;
	group *g = NULL;
	gchar *txt;
	gboolean found = FALSE;
	
	g_return_if_fail (current_user != NULL);

	w0 = tool_widget_get ("user_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, current_user->login);

	w0 = tool_widget_get ("user_settings_group");
	gtk_widget_set_sensitive (w0, tool_get_access());
	user_fill_settings_group (GTK_COMBO (w0));

	tmp_list = group_list;
	while (tmp_list)
	{
		g = tmp_list->data;
		tmp_list = tmp_list->next;
		
		if (g->gid == current_user->gid)
		{
			found = TRUE;
			break;
		}
	}
	
	if (!found)
		g_warning ("The GID for the main user's group was not found in group_list.");
	else
		my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), g->name);
	
	w0 = tool_widget_get ("user_settings_comment");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, current_user->comment);
	
	w0 = tool_widget_get ("user_settings_dialog");
	txt = g_strdup_printf (_("Settings for User %s"), current_user->login);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_show (w0);
}

static void 
user_passwd_dialog_show (void)
{
	GtkWidget *w0;
	gchar *txt;
	
	g_return_if_fail (tool_get_access());

	w0 = tool_widget_get ("user_passwd_dialog");
	txt = g_strdup_printf (_("Password for User %s"), current_user->login);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_show (w0);
}

extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
{
	user_passwd_dialog_show ();
}

extern void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	
	g_return_if_fail (tool_get_access());
	
	current_user = NULL;

	w0 = tool_widget_get ("user_settings_group");
	user_fill_settings_group (GTK_COMBO (w0));
	my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), "");

	w0 = tool_widget_get ("user_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), "Create New User");
	gtk_widget_show (w0);
}

extern void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_if_fail (tool_get_access());
	
	txt = g_strdup_printf (_("Are you sure you want to delete user %s?"), current_user->login);
	parent = GTK_WINDOW (tool_widget_get ("users_admin"));
	
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);

	if (reply)
		return;
	else
	{
		e_table_del (USER);
		tool_set_modified (TRUE);
	}

}


/* Groups tab */

extern void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	GList *member_rows;
	gchar *txt;
	
	g_return_if_fail (current_group != NULL);

	w0 = tool_widget_get ("group_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, current_group->name);

	/* Fill group members */
	member_rows = group_fill_members_list ();

	/* Fill all users list */
	group_fill_all_users_list (member_rows);

	/* Show group settings dialog */
	
	w0 = tool_widget_get ("group_settings_dialog");
	txt = g_strdup_printf (_("Settings for Group %s"), current_group->name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_show (w0);
}

extern void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;

	/* Clear current group */

	current_group = NULL;

	/* Fill all users list */
	group_fill_all_users_list (NULL);

	/* Show group settings dialog */
	
	w0 = tool_widget_get ("group_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), _("Create New Group"));
	gtk_widget_show (w0);
}

extern void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;

	txt = g_strdup_printf (_("Are you sure you want to delete group %s?"), current_group->name);
	parent = GTK_WINDOW (tool_widget_get ("users_admin"));
	
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);

	if (reply)
		return;
	else
	{
		e_table_del (GROUP);
		tool_set_modified (TRUE);
	}
}

/* User settings callbacks */

static void
user_settings_dialog_close (void)
{
	GtkWidget *w0;
	GList *list;
	GtkCList *clist;
	gint row;

	/* set current current user if it's not set */
	if (!current_user)
	{
		clist = GTK_CLIST (tool_widget_get ("user_list"));
		row = GPOINTER_TO_INT (clist->selection->data);
		
		current_user = gtk_clist_get_row_data (clist, row);
	}

	/* Clean up entries */

	w0 = tool_widget_get ("user_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("user_settings_comment");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("user_settings_group");
	list = gtk_container_children (GTK_CONTAINER (w0));
	g_list_free (list);

	w0 = tool_widget_get ("user_settings_dialog");
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
	
	/* if current_user == NULL we are adding new user,
	 * else we are modifying user settigns */
	
	if (!current_user)
	{
		if ((retval = user_add ()))
			user_passwd_dialog_show ();
	}
	else
		retval = user_update (current_user);

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
	
	entry1 = GTK_ENTRY (tool_widget_get ("user_passwd_new"));
	entry2 = GTK_ENTRY (tool_widget_get ("user_passwd_confirmation"));
	quality = GTK_TOGGLE_BUTTON (tool_widget_get ("user_passwd_quality"));
	win = tool_widget_get ("user_passwd_dialog");

	new_passwd = gtk_entry_get_text (entry1);
	confirm = gtk_entry_get_text (entry2);

	/* Empty old contnents */
	
	err = passwd_set (current_user, new_passwd, confirm, gtk_toggle_button_get_active (quality));
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

static void
group_settings_dialog_close (void)
{
	GtkWidget *w0;
	GtkCList *clist;
	gint row;

	/* set current current group if it's not set */
	if (!current_group)
	{
		clist = GTK_CLIST (tool_widget_get ("group_list"));
		row = GPOINTER_TO_INT (clist->selection->data);
		
		current_group = gtk_clist_get_row_data (clist, row);
	}
	/* Clear group name */

	w0 = tool_widget_get ("group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	/* Clear both lists. Do we have to free some memory also? */

	w0 = tool_widget_get ("group_settings_all");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = tool_widget_get ("group_settings_members");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = tool_widget_get ("group_settings_dialog");
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
	
	/* if current_group == NULL we are adding new group,
	 * else we are modifying group settigns */
	
	if (!current_group)
		retval = group_add ();
	else
		retval = group_update (current_group);

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


/* Helper functions */

static void
reply_cb (gint val, gpointer data)
{
	reply = val;
}

static void 
do_quit (void)
{
	if (GTK_WIDGET_IS_SENSITIVE (tool_widget_get ("apply")))
	{
		/* Changes have been made. */
		gchar *txt = _("There are changes which haven't been applyed.\nApply now?");
		GtkWindow *parent;
		GnomeDialog *dialog;
		
		parent = GTK_WINDOW (tool_widget_get ("users_admin"));
		dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL,
				parent));

		gnome_dialog_run (dialog);
		
		if (!reply)
			tool_config_save();
	}

	gtk_main_quit ();
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

