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


static int reply;

/* Prototypes */
static void reply_cb (gint val, gpointer data);

/* Main button callbacks */

static void 
do_quit (void)
{
	/* TODO: Check for changes and optionally ask for confirmation */
	 gtk_main_quit ();
}

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
	GtkWidget *w0, *menu;
	GList *g;
	group *current_g;
	GtkWidget *menu_item;
	guint num_row;
	guint index = 0;
	

	w0 = tool_widget_get ("user_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), current_user->login);

	w0 = tool_widget_get ("user_settings_comment");
	gtk_entry_set_text (GTK_ENTRY (w0), current_user->comment);
	
	/* Build groups menu */

	menu = gtk_menu_new ();
	w0 = tool_widget_get ("user_settings_group");

	for (g = g_list_first (group_list), num_row = 0; g; g = g_list_next (g), num_row++)
	{
		current_g = (group *)g->data;

/*
		if (current_g->gid >= logindefs.new_group_min_id &&
				current_g->gid <= logindefs.new_group_max_id) */
		{
			menu_item = gtk_menu_item_new_with_label (current_g->name);
			gtk_menu_append (GTK_MENU (menu), menu_item);
			gtk_widget_show (menu_item);

			if (current_user->gid == current_g->gid)
				index = num_row;
		}
	}

	gtk_menu_set_active (GTK_MENU (menu), index);
	
	gtk_option_menu_set_menu (GTK_OPTION_MENU (w0), menu);
	
	
	w0 = tool_widget_get ("user_settings_dialog");
	gtk_widget_show (w0);
}

extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	gchar *msg;

	w0 = tool_widget_get ("user_passwd_label");
	msg = g_strdup_printf ("Changing password for %s", current_user->login);
	gtk_label_set_text (GTK_LABEL (w0), msg);
	g_free (msg);
	
	w0 = tool_widget_get ("user_passwd_dialog");
	gtk_widget_show (w0);
}

extern void
on_new_user_clicked (GtkButton *button, gpointer user_data)
{
}

extern void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	gchar *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;
	GtkList *list;

	txt = g_strdup_printf ("Are You sure You want to delete %s?", current_user->login);
	parent = GTK_WINDOW (tool_widget_get ("users_admin"));
	
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);


	if (reply)
		return;
	else
	{
		list = GTK_LIST (tool_widget_get ("user_list"));
		user_list = g_list_remove (user_list, current_user);
		gtk_list_remove_items (list, list->selection);
/*		current_user = user_list; */
		
	}

}

extern void
on_user_list_selection_changed (GtkWidget *list, gpointer user_data)
{
	GList *current;
	GtkObject *list_item;

	current = GTK_LIST (list)->selection;

	if (!current)
		return;

	list_item = GTK_OBJECT (current->data);
	current_user = gtk_object_get_data (list_item, user_list_data_key);
}

/* Groups tab */

extern void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	GList *u, *rows = NULL;
	GtkList *list;
	user *current_u;
	GtkWidget *list_item;


	w0 = tool_widget_get ("group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), current_group->name);

	/* Fill all users list */
	
	list = GTK_LIST (tool_widget_get ("group_settings_all"));
	for (u = g_list_first (user_list); u; u = g_list_next (u))
	{
		current_u = (user *)u->data;
		list_item = gtk_list_item_new_with_label (current_u->login);
		gtk_widget_show (list_item);
		rows = g_list_append (rows, list_item);
	}

	gtk_list_append_items (list, rows);

	/* Fill group members */

	rows = NULL;

	list = GTK_LIST (tool_widget_get ("group_settings_members"));
	for (u = g_list_first (current_group->users); u; u = g_list_next (u))
	{
		list_item = gtk_list_item_new_with_label ((gchar *)u->data);
		gtk_widget_show (list_item);
		rows = g_list_append (rows, list_item);
	}

	gtk_list_append_items (list, rows);


	/* Show group settings dialog */
	
	w0 = tool_widget_get ("group_settings_dialog");
	gtk_widget_show (w0);
}

extern void
on_group_list_selection_changed (GtkWidget *list, gpointer user_data)
{
	GList *current;
	GtkObject *list_item;

	current = GTK_LIST (list)->selection;

	if (!current)
		return;

	list_item = GTK_OBJECT (current->data);
	current_group = gtk_object_get_data (list_item, group_list_data_key);
}



/* User settings callbacks */

extern void
on_user_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	GtkWidget *menu, *menu_item;
	GList *glist, *tmplist;

	/* Clear entries */

	w0 = tool_widget_get ("user_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	w0 = tool_widget_get ("user_settings_comment");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	/* Destroy menu & menuitems */
	
	w0 = tool_widget_get ("user_settings_group");
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (w0));
	glist = gtk_container_children (GTK_CONTAINER (menu));

	for (tmplist = g_list_first (glist); tmplist; tmplist = g_list_next (tmplist))
	{
		menu_item = (GtkWidget *)tmplist->data;
		gtk_widget_destroy (menu_item);
	}
	
	gtk_widget_destroy (menu);


	w0 = tool_widget_get ("user_settings_dialog");
	gtk_widget_hide (w0);
}

extern void
on_user_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	/* That #ifdef is just for commenting this code out, it crashes app and isn't ready.
	 * I'll finish it tomorrow
	 */
	
#ifdef TOMORROW
	GtkWidget *w0;
	gchar *txt;

	w0 = tool_widget_get ("user_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	/* If login name is changed and is at least 3 letters long (is that correct?) */
	if (strcmp (txt, current_user->login) && strlen (txt) >= 3)
	{
		g_free (current_user->login);
		current_user->login = txt;
	}
	else
		g_free (txt);


	w0 = tool_widget_get ("user_settings_comment");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	/* Should it check for size? */
	if (strcmp (txt, current_user->comment))
	{
		g_free (current_user->comment);
		current_user->comment = txt;
	}
	else
		g_free (txt);
	
#endif
	
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
	
	gtk_entry_set_text (entry1, "");
	gtk_entry_set_text (entry2, "");


	/* Check passwords */
	
	if (strcmp (new, confirm))
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented ("Passwords doesn't match!",
			GTK_WINDOW (win)));
		
		gnome_dialog_run (dialog);
	}
	else if (strlen (new) < logindefs.passwd_min_length)
	{
		msg = g_strdup_printf ("Password is too short!\n It must be at least %d "
			"characters long", logindefs.passwd_min_length);

		dialog = GNOME_DIALOG (gnome_error_dialog_parented (msg, GTK_WINDOW (win)));
		gnome_dialog_run (dialog);
		g_free (msg);
	}
	else		
	{
		/* FIXME crypt password!!! how? crypt? */
		g_free (current_user->password);

		current_user->password = g_strdup (new);

		msg = g_strdup_printf ("Password changed successfully for %s!",
				current_user->login);
		
		dialog = GNOME_DIALOG (gnome_ok_dialog_parented (msg, GTK_WINDOW (win)));
		
		gnome_dialog_run (dialog);
		gtk_widget_hide (win);
	}

}

/* Group settings dialog */

extern void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;

	/* Clear lists */

	w0 = tool_widget_get ("group_settings_all");
	gtk_list_clear_items (GTK_LIST (w0), 0, -1);

	w0 = tool_widget_get ("group_settings_members");
	gtk_list_clear_items (GTK_LIST (w0), 0, -1);

	w0 = tool_widget_get ("group_settings_dialog");
	gtk_widget_hide (w0);
}

extern void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
}


/* Helper functions */

static void
reply_cb (gint val, gpointer data)
{
	reply = val;
}

