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

#include <gnome.h>
#include <sys/stat.h>
#include <unistd.h>
#include "global.h"

#include "callbacks.h"
#include "transfer.h"

/* Local globals */
static int reply;
static const gchar *GROUP_MEMBER_DATA_KEY = "group_member_name";


/* Prototypes */
static void reply_cb (gint val, gpointer data);
static GList *fill_group_members_list (void);
static void fill_all_users_list (GList *member_rows);
static void fill_user_settings_group (GtkCombo *combo);
static void do_quit (void);
static void user_actions_set_sensitive (gboolean state);
static void group_actions_set_sensitive (gboolean state);
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
	GList *i;
	group *g;
	
	g_return_if_fail (current_user != NULL);

	w0 = tool_widget_get ("user_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, current_user->login);

	/* TODO: fill the main group widget with available groups. */
	w0 = tool_widget_get ("user_settings_group");
	gtk_widget_set_sensitive (w0, tool_get_access());
	fill_user_settings_group (GTK_COMBO (w0));
	for (i = g_list_first (group_list); i; i = g_list_next (i)) 
	{
		g = (group *) i->data;
		if (g->gid == current_user->gid)
			break;
	}
	
	if (!i)
		g_warning ("The GID for the main user's group was not found in group_list.");
	else
		my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), g->name);
	
	w0 = tool_widget_get ("user_settings_comment");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, current_user->comment);
	
	w0 = tool_widget_get ("user_settings_dialog");
	gtk_widget_show (w0);
}

extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	
	g_return_if_fail (tool_get_access());
	
	w0 = tool_widget_get ("user_passwd_dialog");
	gtk_widget_show (w0);
}

extern void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (tool_get_access());
}

extern void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;
	GtkList *list;
	GList *selection;

	g_return_if_fail (tool_get_access());
	
	txt = g_strdup_printf ("Are you sure you want to delete user %s?", current_user->login);
	parent = GTK_WINDOW (tool_widget_get ("users_admin"));
	
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);

	if (reply)
		return;
	else
	{
		user_list = g_list_remove (user_list, current_user);
		
		list = GTK_LIST (tool_widget_get ("user_list"));
		selection = g_list_copy (list->selection);
		gtk_list_remove_items (list, selection);
		g_list_free (selection);
		current_user = NULL;
		tool_set_modified (TRUE);
	}
}

extern void
on_user_list_selection_changed (GtkWidget *list, gpointer user_data)
{
	GList *current;
	GtkObject *list_item;

	current = GTK_LIST (list)->selection;

	if (!current) 
	{
		user_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")), 
												 "Settings for the selected user");
	} 
	else 
	{
		gchar *label;
		
		list_item = GTK_OBJECT (current->data);
		current_user = gtk_object_get_data (list_item, user_list_data_key);
		
		user_actions_set_sensitive (TRUE);
		label = g_strconcat ("Settings for user ", current_user->login, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")), label);
		g_free (label);
	}
}


/* Groups tab */

extern void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	GList *member_rows;	
	
	g_return_if_fail (current_group != NULL);

	w0 = tool_widget_get ("group_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, current_group->name);

	/* Fill group members */
	member_rows = fill_group_members_list ();

	/* Fill all users list */
	fill_all_users_list (member_rows);

	/* Show group settings dialog */
	
	w0 = tool_widget_get ("group_settings_dialog");
	gtk_widget_show (w0);
}

extern void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (tool_get_access());
}

extern void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;
	GtkList *list;
	GList *selection;

	txt = g_strdup_printf ("Are you sure you want to delete group %s?", current_group->name);
	parent = GTK_WINDOW (tool_widget_get ("users_admin"));
	
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);

	if (reply)
		return;
	else
	{
		group_list = g_list_remove (group_list, current_group);
		
		list = GTK_LIST (tool_widget_get ("group_list"));
		selection = g_list_copy (list->selection);
		gtk_list_remove_items (list, selection);
		g_list_free (selection);
/*		current_user = user_list; */
		
	}
}

extern void
on_group_list_selection_changed (GtkWidget *list, gpointer user_data)
{
	GList *current;
	GtkObject *list_item;

	current = GTK_LIST (list)->selection;

	if (!current)
	{
		group_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), 
												 "Settings for the selected group");
	}
	else
	{
		gchar *label;
		
		list_item = GTK_OBJECT (current->data);
		current_group = gtk_object_get_data (list_item, group_list_data_key);
		
		group_actions_set_sensitive (TRUE);
		label = g_strconcat ("Settings for group ", current_group->name, NULL);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), label);
		g_free (label);
	}
}



/* User settings callbacks */

extern void
on_user_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;

	w0 = tool_widget_get ("user_settings_dialog");
	gtk_widget_hide (w0);
}

extern void
on_user_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *txt;
	GtkWidget *label, *list_item;
	GtkList *list;
	GList *tmplist;
	group *g;
																			 
	w0 = tool_widget_get ("user_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	win = GTK_WINDOW (tool_widget_get ("user_settings_dialog"));

	/* If login name is changed and is at least 3 letters long (is that correct?) */
	if (strcmp (txt, current_user->login))
	{
		if (strlen (txt) < 1)
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					("Username is empty.", win));
			
			gnome_dialog_run (dialog);
		}
		else
		{
			g_free (current_user->login);
			current_user->login = g_strdup (txt);

			list = GTK_LIST (tool_widget_get ("user_list"));
			tmplist = g_list_copy (list->selection);
			list_item = (GtkWidget *)tmplist->data;
			label = GTK_BIN (list_item)->child;
			gtk_label_set_text (GTK_LABEL (label), txt);			
		}
	}

	w0 = tool_widget_get ("user_settings_comment");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	/* Should it check for size? */
	if (strcmp (txt, current_user->comment))
	{
		g_free (current_user->comment);
		current_user->comment = g_strdup (txt);
	}


	/* Get selected group name */

	w0 = tool_widget_get ("user_settings_group");
	txt = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (w0)->entry), 0, -1);

	/* Now find group's gid */

	for (tmplist = g_list_first (group_list); tmplist; tmplist = g_list_next (tmplist))
	{
		g = (group *)tmplist->data;
		if (!strcmp (g->name, txt))
			break;
	}
	
	if (!tmplist)
		g_warning ("Couldn't find the GID for the selected group.");
	else
		current_user->gid = g->gid;
	
	g_free (txt);
				
	tool_set_modified(TRUE);
	/* Clean up dialog, it's easiest to just call *_cancel_* function */
	on_user_settings_cancel_clicked (NULL, NULL);
}

/* Password settings callbacks */

extern void
on_user_passwd_cancel_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;

	w0 = tool_widget_get ("user_passwd_dialog");
	gtk_widget_hide (w0);
}

extern void
on_user_passwd_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkEntry *entry1, *entry2;
	gchar *new, *confirm;
	GtkWidget *win;
	GnomeDialog *dialog;
	gchar *msg;
	
	entry1 = GTK_ENTRY (tool_widget_get ("user_passwd_new"));
	entry2 = GTK_ENTRY (tool_widget_get ("user_passwd_confirmation"));
	win = tool_widget_get ("user_passwd_dialog");

	new = gtk_entry_get_text (entry1);
	confirm = gtk_entry_get_text (entry2);

	/* Empty old contnents */
	
	my_gtk_entry_set_text (entry1, "");
	my_gtk_entry_set_text (entry2, "");


	/* Check passwords */
	
	if (strcmp (new, confirm))
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented ("The password and its confirmation\nmust match.",
			GTK_WINDOW (win)));
		
		gnome_dialog_run (dialog);
	}
	else if (strlen (new) < logindefs.passwd_min_length)
	{
		msg = g_strdup_printf ("The password is too short:\nit must be at least %d "
			"characters long.", logindefs.passwd_min_length);

		dialog = GNOME_DIALOG (gnome_error_dialog_parented (msg, GTK_WINDOW (win)));
		gnome_dialog_run (dialog);
		g_free (msg);
	}
	else		
	{
		/* FIXME crypt password!!! how? crypt? */
		/* it depends on the crypt method of the system. We'll have to
		 * first identify the method, through magic, and then use crypt
		 * and steal some code for MD5 crypt */
		
		g_free (current_user->password);
		current_user->password = g_strdup (new);
		gtk_widget_hide (win);
		tool_set_modified (TRUE);
	}
}

/* Group settings dialog */

extern void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;

	w0 = tool_widget_get ("group_settings_dialog");
	gtk_widget_hide (w0);
}

extern void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0, *label, *list_item;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *txt;
	GList *selection;
	GtkList *list;
	
	win = GTK_WINDOW (tool_widget_get ("group_settings_dialog"));

	w0 = tool_widget_get ("group_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	if (strcmp (current_group->name, txt))
	{
		if (strlen (txt) < 1)
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					("Group name is empty.", win));
			
			gnome_dialog_run (dialog);
		}
		else
		{
			g_free (current_group->name);
			current_group->name = g_strdup (txt);

			list = GTK_LIST (tool_widget_get ("group_list"));
			selection = g_list_copy (list->selection);
			list_item = (GtkWidget *)selection->data;
			label = GTK_BIN (list_item)->child;
			gtk_label_set_text (GTK_LABEL (label), txt);
		}
	}

	/* TODO: update group members also */

	tool_set_modified(TRUE);
	/* Clear list and hide dialog */
	on_group_settings_cancel_clicked (NULL, NULL);
}

extern void
on_group_settings_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkList *all, *members;
	GList *selection;
	GtkWidget *list_item;

	all = GTK_LIST (tool_widget_get ("group_settings_all"));
	members = GTK_LIST (tool_widget_get ("group_settings_members"));

	selection = g_list_copy (all->selection);
	list_item = (GtkWidget *)selection->data;
	gtk_widget_reparent (list_item, GTK_WIDGET (members));
	g_list_free (selection);
}

extern void
on_group_settings_remove_clicked (GtkButton *button, gpointer user_data)
{
	GtkList *all, *members;
	GList *selection;
	GtkWidget *list_item;

	all = GTK_LIST (tool_widget_get ("group_settings_all"));
	members = GTK_LIST (tool_widget_get ("group_settings_members"));

	selection = g_list_copy (members->selection);
	list_item = (GtkWidget *)selection->data;
	gtk_widget_reparent (list_item, GTK_WIDGET (all));
	g_list_free (selection);
}


extern void
on_group_settings_all_selection_changed (GtkWidget *list, gpointer user_data)
{
	GList *current;
	GtkWidget *w0;

	current = GTK_LIST (list)->selection;
	
	if (tool_get_access())
	{
		w0 = tool_widget_get ("group_settings_add");

		if (!current)
			gtk_widget_set_sensitive (w0, FALSE);
		else
			gtk_widget_set_sensitive (w0, TRUE);
	}
}
	
extern void
on_group_settings_members_selection_changed (GtkWidget *list, gpointer user_data)
{
	GList *current;
	GtkWidget *w0;

	current = GTK_LIST (list)->selection;

	if (tool_get_access())
	{
		w0 = tool_widget_get ("group_settings_remove");

		if (!current)
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

static GList *
fill_group_members_list (void)
{
	GList *u;
	GList *member_rows = NULL;
	GtkList *list;
	GtkWidget *list_item;


	list = GTK_LIST (tool_widget_get ("group_settings_members"));

	for (u = g_list_first (current_group->users); u; u = g_list_next (u))
	{
		list_item = gtk_list_item_new_with_label ((gchar *)u->data);
		gtk_widget_show (list_item);
		gtk_object_set_data (GTK_OBJECT (list_item), GROUP_MEMBER_DATA_KEY,
				((gchar *)u->data));
		
		member_rows = g_list_append (member_rows, list_item);
		
	}

	gtk_list_append_items (list, member_rows);

	return member_rows;
}

static void
fill_all_users_list (GList *member_rows)
{
	GList *u;
	GList *all_rows = NULL;
	GtkList *list;
	user *current_u;
	GtkWidget *list_item;
	GList *tmplist;
	gchar *name;
	GtkObject *item;
	gboolean found;

	
	list = GTK_LIST (tool_widget_get ("group_settings_all"));
	for (u = g_list_first (user_list); u; u = g_list_next (u))
	{
		current_u = (user *)u->data;
		found = FALSE;

		/* Find, if not in members */

		for (tmplist = g_list_first (member_rows); tmplist; tmplist = g_list_next (tmplist))
		{
			item = GTK_OBJECT (tmplist->data);
			name = gtk_object_get_data (item, GROUP_MEMBER_DATA_KEY);

			if (!strcmp (name, current_u->login))
			{
				found = TRUE;
				break;
			}
		}

		if (!found)
		{
			list_item = gtk_list_item_new_with_label (current_u->login);
			gtk_widget_show (list_item);
			all_rows = g_list_append (all_rows, list_item);
		}
	}

	gtk_list_append_items (list, all_rows);
}

static void
fill_user_settings_group (GtkCombo *combo)
{
	GList *u, *items;
	
	items = NULL;
	for (u = g_list_first (group_list); u; u = g_list_next (u))
		items = g_list_append (items, ((group *) u->data)->name);
	
	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}

static void 
do_quit (void)
{
	/* TODO: Check for changes and optionally ask for confirmation */
	
	if (GTK_WIDGET_IS_SENSITIVE (tool_widget_get ("apply")))
	{
		/* Changes have been made. */
		gchar *txt = "There are changes which haven't been applyed.\nApply now?";
		GtkWindow *parent;
		GnomeDialog *dialog;
		
		parent = GTK_WINDOW (tool_widget_get ("users_admin"));
		dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
		gnome_dialog_run (dialog);
		
		if (!reply)
			tool_config_save();
	}

	gtk_main_quit ();
}

static void
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

static void
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
