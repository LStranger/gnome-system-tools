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
#include "transfer.h"

/* All this for password generation and crypting. */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <crack.h>
#include <crypt.h>
#include "md5.h"

#define CRACK_DICT_PATH "/usr/lib/cracklib_dict"
#define RANDOM_PASSWD_SIZE 6

#define USER 1
#define GROUP 2
#define UID 1
#define GID 2

/* Local globals */
static int reply;
static gchar *pam_passwd_files[] = { "/etc/pam.d/passwd", NULL };


/* Prototypes */
static void reply_cb (gint val, gpointer data);
static GList *fill_group_members_list (void);
static void fill_all_users_list (GList *member_rows);
static void fill_user_settings_group (GtkCombo *combo);
static void do_quit (void);
static void user_actions_set_sensitive (gboolean state);
static void group_actions_set_sensitive (gboolean state);
static void my_gtk_entry_set_text (void *entry, gchar *str);
static gboolean update_user (void);
static gboolean add_user (void);
static gboolean update_group (void);
static gboolean add_group (void);
static group *make_default_group (void);
static user *make_default_user (gchar *name);
static guint find_new_id (gchar from, gchar what);
static gchar *find_new_key (gchar from);
static gboolean is_valid_name (gchar *str);
static gboolean passwd_uses_md5 (void);
static gchar *passwd_rand_str (gchar *str, gint len);
gint char_sort_func (gconstpointer a, gconstpointer b);


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
	fill_user_settings_group (GTK_COMBO (w0));

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

extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data)
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
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	
	g_return_if_fail (tool_get_access());
	
	current_user = NULL;

	w0 = tool_widget_get ("user_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());

	w0 = tool_widget_get ("user_settings_group");
	gtk_widget_set_sensitive (w0, tool_get_access());
	fill_user_settings_group (GTK_COMBO (w0));
	my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), "");


	w0 = tool_widget_get ("user_settings_comment");
	gtk_widget_set_sensitive (w0, tool_get_access());

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
	GtkCList *clist;
	gint row;

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
		clist = GTK_CLIST (tool_widget_get ("user_list"));
		row = GPOINTER_TO_INT (clist->selection->data);
		user_list = g_list_remove (user_list, current_user);
		gtk_clist_remove (clist, row);
		current_user = NULL;
		tool_set_modified (TRUE);
	}
}

extern void
on_user_list_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event, 
		gpointer user_data)
{
	if (!clist->selection) 
	{
		user_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("user_settings_frame")),
				_("Settings for the selected user"));
	} 
	else 
	{
		gchar *label;
		
		current_user = gtk_clist_get_row_data (clist, row);
		
		user_actions_set_sensitive (TRUE);
		label = g_strdup_printf (_("Settings for user %s"), current_user->login);
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
	gchar *txt;
	
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
	fill_all_users_list (NULL);

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
	GtkCList *clist;
	gint row;
	

	txt = g_strdup_printf (_("Are you sure you want to delete group %s?"), current_group->name);
	parent = GTK_WINDOW (tool_widget_get ("users_admin"));
	
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);

	if (reply)
		return;
	else
	{
		clist = GTK_CLIST (tool_widget_get ("group_list"));
		row = GPOINTER_TO_INT (clist->selection->data), 
		group_list = g_list_remove (group_list, current_group);
		gtk_clist_remove (clist, row);
		current_group = NULL;
		tool_set_modified (TRUE);
	}
}

extern void
on_group_list_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event, 
		gpointer user_data)
{
	if (!clist->selection)
	{
		group_actions_set_sensitive (FALSE);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")),
				_("Settings for the selected group"));
		
	}
	else
	{
		gchar *label;
		
		current_group = gtk_clist_get_row_data (clist, row);
		
		group_actions_set_sensitive (TRUE);
		label = g_strdup_printf (_("Settings for group "), current_group->name);
		gtk_frame_set_label (GTK_FRAME (tool_widget_get ("group_settings_frame")), label);
		g_free (label);
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
		if ((retval = add_user ()))
			on_user_chpasswd_clicked (NULL, NULL);
	}
	else
		retval = update_user ();

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
	gchar random_passwd[RANDOM_PASSWD_SIZE + 1];
	gchar *txt;
	
	win = tool_widget_get ("user_passwd_dialog");
	entry1 = GTK_ENTRY (tool_widget_get ("user_passwd_new"));
	entry2 = GTK_ENTRY (tool_widget_get ("user_passwd_confirmation"));
	
	*random_passwd = 0;
	
	while (FascistCheck (random_passwd, CRACK_DICT_PATH))
		passwd_rand_str (random_passwd, RANDOM_PASSWD_SIZE);

	my_gtk_entry_set_text (entry1, random_passwd);
	my_gtk_entry_set_text (entry2, random_passwd);
	
	txt = g_strdup_printf (_("Password set to \"%s\"."), random_passwd);
	dialog = GNOME_DIALOG (gnome_ok_dialog_parented (txt, GTK_WINDOW (win)));
	gnome_dialog_run (dialog);
	g_free (txt);
}

extern void
on_user_passwd_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkEntry *entry1, *entry2;
	GtkToggleButton *quality;
	gchar *new_passwd, *confirm, salt[9];
	GtkWidget *win;
	GnomeDialog *dialog;
	gchar *msg, *check_err;
	
	entry1 = GTK_ENTRY (tool_widget_get ("user_passwd_new"));
	entry2 = GTK_ENTRY (tool_widget_get ("user_passwd_confirmation"));
	quality = GTK_TOGGLE_BUTTON (tool_widget_get ("user_passwd_quality"));
	win = tool_widget_get ("user_passwd_dialog");

	new_passwd = gtk_entry_get_text (entry1);
	confirm = gtk_entry_get_text (entry2);

	/* Empty old contnents */
	
	my_gtk_entry_set_text (entry1, "");
	my_gtk_entry_set_text (entry2, "");


	/* Check passwords */
	
	if (strcmp (new_passwd, confirm))
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (_("The password and its confirmation\nmust match."),
			GTK_WINDOW (win)));
		
		gnome_dialog_run (dialog);
	}
	else if (gtk_toggle_button_get_active (quality) &&
					 (check_err = FascistCheck (new_passwd, CRACK_DICT_PATH)))
	{
		msg = g_strdup_printf (_("Bad password: %s.\nPlease try with a new password."), check_err);
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (msg, GTK_WINDOW (win)));
		g_free (msg);
		gnome_dialog_run (dialog);
	}
	else
	{
		g_free (current_user->password);
		if (passwd_uses_md5 ()) 
			current_user->password = g_strdup (crypt_md5 (new_passwd, passwd_rand_str (salt, 8)));
		else
			current_user->password = g_strdup (crypt (new_passwd, passwd_rand_str (salt, 2)));

		gtk_widget_hide (win);
		tool_set_modified (TRUE);
	}
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
		retval = add_group ();
	else
		retval = update_group ();

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

static GList *
fill_group_members_list (void)
{
	GList *tmp_list;
	GList *member_rows = NULL;
	GtkCList *clist;
	gint row;
	gchar *entry[2];

	entry[1] = NULL;

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_freeze (clist);

	tmp_list = current_group->users;

	while (tmp_list)
	{
		entry[0] = tmp_list->data;
		tmp_list = tmp_list->next;
		
		row = gtk_clist_append (clist, entry);
		member_rows = g_list_append (member_rows, entry[0]);
	}

	gtk_clist_thaw (clist);
	return member_rows;
}

static void
fill_all_users_list (GList *member_rows)
{
	GList *tmp_list, *member;
	GtkCList *clist;
	user *current_u;
	gchar *name;
	gboolean found;
	gchar *entry[2];

	entry[1] = NULL;

	clist = GTK_CLIST (tool_widget_get ("group_settings_all"));
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_freeze (clist);

	tmp_list = user_list;
	while (tmp_list)
	{
		current_u = tmp_list->data;
		tmp_list = tmp_list->next;

		found = FALSE;

		member = member_rows;
		while (member)
		{
			name = member->data;
			member = member->next;

			if (!strcmp (name, current_u->login))
			{
				found = TRUE;
				break;
			}
		}

		if (!found)
		{
			entry[0] = current_u-> login;
			gtk_clist_append (clist, entry);
		}
	}

	gtk_clist_thaw (clist);
}

static void
fill_user_settings_group (GtkCombo *combo)
{
	GList *u, *items;

	items = NULL;
	for (u = g_list_first (group_list); u; u = g_list_next (u))
		items = g_list_append (items, ((group *) u->data)->name);

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
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

static gboolean
update_user (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *txt, *new_group_name;
	GtkCList *list;
	GList *tmp_list;
	group *g = NULL;
	group *new_group;
	gint row;
	gboolean found = FALSE;
	gchar *entry[2];

	entry[1] = NULL;
	
	w0 = tool_widget_get ("user_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	win = GTK_WINDOW (tool_widget_get ("user_settings_dialog"));

	/* If login name is changed and isn't empty */
	if (strcmp (txt, current_user->login))
	{
		if (strlen (txt) < 1)
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					(_("Username is empty."), win));
			
			gnome_dialog_run (dialog);
			gtk_widget_grab_focus (w0);
			return FALSE;
		}

		if (!is_valid_name (txt))
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (
			("Please set a valid username, using only lower-case letters."), win));
			gnome_dialog_run (dialog);
			return FALSE;
		}

		g_free (current_user->login);
		current_user->login = g_strdup (txt);

		list = GTK_CLIST (tool_widget_get ("user_list"));
		row = gtk_clist_find_row_from_data (list, current_user);
		gtk_clist_set_text (list, row, 0, txt);
	}

	w0 = tool_widget_get ("user_settings_comment");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	if (current_user->comment == NULL)
	{
		if (strlen (txt) > 0)
			current_user->comment = g_strdup (txt);
	}
	else if (strcmp (txt, current_user->comment))
	{
		g_free (current_user->comment);
		current_user->comment = g_strdup (txt);
	}

	/* Get selected group name */

	w0 = tool_widget_get ("user_settings_group");
	new_group_name = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (w0)->entry), 0, -1);

	/* Now find group's gid */

	tmp_list = group_list;
	while (tmp_list)
	{
		g = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (g->name, new_group_name))
		{
			found = TRUE;
			break;
		}
	}
	
	if (!found)
	{
		/* New group: check that it is a valid group name */

		if (!is_valid_name (new_group_name))
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (
				"Please set a valid main group name, with only lower-case letters,"
				"\nor select one from pull-down menu.", win));

			gnome_dialog_run (dialog);
			return FALSE;
		}

		/* Cool: create new group */

		new_group = make_default_group ();
		new_group->name = g_strdup (new_group_name);
		group_list = g_list_append (group_list, new_group);
		current_user->gid = new_group->gid;

		/* Add to CList */

		list = GTK_CLIST (tool_widget_get ("group_list"));

		entry[0] = new_group_name;
		row = gtk_clist_append (list, entry);
		gtk_clist_set_row_data (list, row, new_group);
	}
	
	else
		current_user->gid = g->gid;

	return TRUE;
}

static gboolean
add_user (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_user_name, *new_group_name, *new_comment, *tmp;
	GtkCList *clist;
	GList *tmp_list;
	group *g = NULL;
	group *new_group;
	user *new_user, *current_u;
	gchar *entry[2];
	gboolean found = FALSE;
	gint row;

	entry[1] = NULL;

	w0 = tool_widget_get ("user_settings_name");
	new_user_name = gtk_entry_get_text (GTK_ENTRY (w0));

	win = GTK_WINDOW (tool_widget_get ("user_settings_dialog"));

	/* If login name isn't empty */
	if (strlen (new_user_name) < 1)
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented 
				(_("Username is empty."), win));
		
		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (w0);
		return FALSE;
	}

	/* Check if user exists */

	tmp_list = user_list;
	while (tmp_list)
	{
		current_u = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (current_u->login, new_user_name))
		{
			tmp = g_strdup_printf (_("User %s already exists."), new_user_name);
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (tmp, win));
			gnome_dialog_run (dialog);
			g_free (tmp);
			
			gtk_widget_grab_focus (w0);
			gtk_editable_select_region (GTK_EDITABLE (w0), 0, -1);
			
			return FALSE;
		}
	}
	
	if (!is_valid_name (new_user_name))
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (
			_("Please set a valid username, using only lower-case letters."), win));
		
		gnome_dialog_run (dialog);
		return FALSE;
	}


	/* Get group name */

	w0 = tool_widget_get ("user_settings_group");
	new_group_name = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (w0)->entry), 0, -1);

	/* Now find new group's gid, if it exists */

	tmp_list = group_list;
	while (tmp_list)
	{
		g = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (g->name, new_group_name))
		{
			found = TRUE;
			break;
		}
	}
	
	if (!found)
	{
		/* New group: check that it is a valid group name */
		
		if (!is_valid_name (new_group_name))
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (_("Please set a valid "
					"main group name, with only lower-case letters,\n"
					"or select one from the pull-down menu."), win));
			
			gnome_dialog_run (dialog);
			return FALSE;
		}
		
		/* Cool: create group. */
		
		new_group = make_default_group ();
		new_group->name = g_strdup (new_group_name);
		group_list = g_list_append (group_list, new_group);
		
		/* Add to the GtkCList. */
		
		clist = GTK_CLIST (tool_widget_get ("group_list"));

		entry[0] = new_group_name;
		row =  gtk_clist_append (clist, entry);
		gtk_clist_set_row_data (clist, row, new_group);
	}
	else
		new_group = g;

	/* Everything should be ok, let's create a new user */

	new_user = make_default_user (new_user_name);
	new_user->gid = new_group->gid;
	
	w0 = tool_widget_get ("user_settings_comment");
	new_comment = gtk_entry_get_text (GTK_ENTRY (w0));
	if (strlen (new_comment) > 0)
		new_user->comment = g_strdup (new_comment);
	user_list = g_list_append (user_list, new_user);
	
	/* Add to the user_list GtkCList */
	
	clist = GTK_CLIST (tool_widget_get ("user_list"));
	
	entry[0] = new_user_name;
	row = gtk_clist_append (clist, entry);
	gtk_clist_set_row_data (clist, row, new_user);
	
	/* Select current user in users list (last one)*/

	gtk_clist_select_row (clist, row, 0);
	current_user = new_user;

	return TRUE;
}

static gboolean
update_group (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *txt;
	GList *selection;
	GtkCList *clist;
	gint row;
	
	win = GTK_WINDOW (tool_widget_get ("group_settings_dialog"));

	w0 = tool_widget_get ("group_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	if (strcmp (current_group->name, txt))
	{
		if (strlen (txt) < 1)
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					(_("Group name is empty."), win));
			
			gnome_dialog_run (dialog);
			gtk_widget_grab_focus (w0);
			return FALSE;
		}
		else
		{
			g_free (current_group->name);
			current_group->name = g_strdup (txt);

			clist = GTK_CLIST (tool_widget_get ("group_list"));
			row = gtk_clist_find_row_from_data (clist, current_group);
			gtk_clist_set_text (clist, row, 0, txt);
		}
	}

	/* Update group members also */
	/* First, free our old users list ... */

	selection = current_group->users;
	while (selection)
	{
		txt = selection->data;
		selection = selection->next;

		g_free (txt);
	}
	
	g_list_free (current_group->users);
	current_group->users = NULL;

	/* ... and then, build new one */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (TRUE)
	{
		if (!gtk_clist_get_text (clist, row++, 0, &txt))
			break;

		current_group->users = g_list_append (current_group->users, g_strdup (txt));
	}

	return TRUE;
}

static gboolean
add_group (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_group_name, *tmp;
	GList *selection = NULL;
	GtkCList *clist;
	group *tmpgroup, *current_g;
	gint row;
	gchar *entry[2];

	entry[1] = NULL;
	
	win = GTK_WINDOW (tool_widget_get ("group_settings_dialog"));

	w0 = tool_widget_get ("group_settings_name");
	new_group_name = gtk_entry_get_text (GTK_ENTRY (w0));

	if (strlen (new_group_name) < 1)
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented 
				(_("Group name is empty."), win));
		
		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (w0);
		return FALSE;
	}
	
	/* Find if group with given name already exists */

	selection = group_list;
	while (selection)
	{
		current_g = selection->data;
		selection = selection->next;

		if (!strcmp (current_g->name, new_group_name))
		{
			tmp = g_strdup_printf (_("Group %s already exists."), new_group_name);
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (tmp, win));
			gnome_dialog_run (dialog);
			g_free (tmp);
			
			gtk_widget_grab_focus (w0);
			gtk_editable_select_region (GTK_EDITABLE (w0), 0, -1);
			return FALSE;
		}
	}
	
	/* Is the group name valid? */
	
	if (!is_valid_name (new_group_name))
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (
		  _("Please set a valid gruop name, with only lower-case letters."), win));
		gnome_dialog_run (dialog);
		return FALSE;
	}
	

	/* Everything should be ok and we can add new group */

	tmpgroup = make_default_group ();
	tmpgroup->name = g_strdup (new_group_name);

	clist = GTK_CLIST (tool_widget_get ("group_list"));
	entry[0] = new_group_name;
	row = gtk_clist_append (clist, entry);
	gtk_clist_set_row_data (clist, row, tmpgroup);
	
	/* Add group members */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (TRUE)
	{
		if (!gtk_clist_get_text (clist, row++, 0, &tmp))
			break;

		tmpgroup->users = g_list_append (tmpgroup->users, g_strdup (tmp));
	}

	current_group = tmpgroup;
	group_list = g_list_append (group_list, current_group);

	/* Select current group in group list */

	row = gtk_clist_find_row_from_data (clist, current_group);
	gtk_clist_select_row (clist, row, 0);

	return TRUE;
}

static group *
make_default_group (void)
{
	group *gr;
	
	gr = g_new0 (group, 1);

	gr->users = NULL;
	gr->password = g_strdup ("x");
	gr->gid = find_new_id (GROUP, 0);
	gr->key = find_new_key (GROUP);

	return gr;
}

static user *
make_default_user (gchar *name)
{
	user *u;

	u = g_new0 (user, 1);
	u->key = find_new_key (USER);
	u->uid = find_new_id (USER, UID);
	u->login = g_strdup (name);
	u->passwd_min_life = logindefs.passwd_min_day_use;
	u->passwd_max_life = logindefs.passwd_max_day_use;
	u->passwd_exp_warn = logindefs.passwd_warning_advance_days;
	u->is_shadow = TRUE;	/* hardcoded, since we don't have it in ui yet */

	if (logindefs.create_home)
		u->home = g_strdup_printf ("/home/%s", name);

	u->shell = g_strdup ("/bin/bash"); /* hardcoded, since we don't have it in ui yet */

	return u;
}
	

static guint
find_new_id (gchar from, gchar what)
{
	GList *tmp_list;
	group *current_g;
	user *current_u;
	guint ret = 0;
	
	if (from == GROUP)
	{
		ret = logindefs.new_group_min_id;

		tmp_list = group_list;
		while (tmp_list)
		{
			current_g = tmp_list->data;
			tmp_list = tmp_list->next;

			if (ret <= current_g->gid)
				ret = current_g->gid + 1;
		}
		
		if (ret > logindefs.new_group_max_id)
		{
			g_warning ("new gid is out of bounds");
			return -1;
		}
		
		return ret;
	}

	else if (from == USER && what == UID)
	{
		ret = logindefs.new_user_min_id;

		tmp_list = user_list;
		while (tmp_list)
		{
			current_u = tmp_list->data;
			tmp_list = tmp_list->next;

			if (ret <= current_u->uid)
				ret = current_u->uid + 1;
		}
		
		if (ret > logindefs.new_user_max_id)
		{
			g_warning ("new gid is out of bounds");
			return -1;
		}
		
		return ret;
	}


	/* Shouldn't reach here */
	
	g_warning ("find_last_id: wrong parameter");
	return -1;
}

static gchar *
find_new_key (gchar from)
{
	GList *tmp_list;
	group *current_g;
	user *current_u;
	guint ret = 0;
	guint tmp;
	gchar *buf;

	if (from == GROUP)
	{
		tmp_list = group_list;
		while (tmp_list)
		{
			current_g = tmp_list->data;
			tmp_list = tmp_list->next;

			tmp = atoi (current_g->key);
			if (ret <= tmp)
				ret = tmp + 1;
		}

		buf = g_strdup_printf ("%06d", ret);
		
		return buf;
	}

	if (from == USER)
	{
		tmp_list = user_list;
		while (tmp_list)
		{
			current_u = tmp_list->data;
			tmp_list = tmp_list->next;

			tmp = atoi (current_u->key);
			if (ret <= tmp)
				ret = tmp + 1;
		}

		buf = g_strdup_printf ("%06d", ret);

		return buf;
	}

	/* We shouldn't get here */

	g_warning ("find_new_key: wrong params");
	return NULL;
}

static gboolean 
is_valid_name (gchar *str)
{
	if (!str || !*str)
		return FALSE;

	for (;*str; str++)
	{
		if (((*str < 'a') || (*str > 'z')) &&
				((*str < '0') || (*str > '9')))
			return FALSE;
	}

	return TRUE;
}

static gboolean
passwd_uses_md5 (void)
{
	gint i;
	gint fd = -1;
	gint last_line = 1;
	static gboolean been_here = FALSE;
	static gboolean found = FALSE;
	GScanner *scanner;
	GScannerConfig scanner_config =
	{
		(
		 " \t\r\n"
		 )			/* cset_skip_characters */,
		(
		 G_CSET_a_2_z
		 "_/.="
		 G_CSET_A_2_Z
		 )			/* cset_identifier_first */,
		(
		 G_CSET_a_2_z
		 "_/.="
		 G_CSET_A_2_Z
		 "1234567890"
		 G_CSET_LATINS
		 G_CSET_LATINC
		 )			/* cset_identifier_nth */,
		( "#\n" )		/* cpair_comment_single */,
		
		FALSE			/* case_sensitive */,
		
		TRUE			/* skip_comment_multi */,
		TRUE			/* skip_comment_single */,
		TRUE			/* scan_comment_multi */,
		TRUE			/* scan_identifier */,
		FALSE			/* scan_identifier_1char */,
		FALSE			/* scan_identifier_NULL */,
		FALSE			/* scan_symbols */,
		FALSE			/* scan_binary */,
		FALSE			/* scan_octal */,
		FALSE			/* scan_float */,
		FALSE			/* scan_hex */,
		FALSE			/* scan_hex_dollar */,
		FALSE			/* scan_string_sq */,
		FALSE			/* scan_string_dq */,
		FALSE			/* numbers_2_int */,
		FALSE			/* int_2_float */,
		FALSE			/* identifier_2_string */,
		FALSE			/* char_2_token */,
		FALSE			/* symbol_2_token */,
		FALSE			/* scope_0_fallback */,
	};
	
	if (been_here)
		return found;
	
	for (i = 0; pam_passwd_files[i]; i++)
		if ((fd = open (pam_passwd_files[i], O_RDONLY)) != -1)
			break;
	
	if (fd == -1)
		return FALSE;
	
	found = FALSE;
	scanner = g_scanner_new (&scanner_config);
	g_scanner_input_file (scanner, fd);
	
	/* Scan the file, until the md5 argument for /lib/security/pam_pwdb.so
	 * in the module-type password is found, or EOF */
	while ((g_scanner_get_next_token (scanner) != G_TOKEN_EOF) && !found)
	{
		
		/* Has a password module directive been found? */
		if ((scanner->token == G_TOKEN_IDENTIFIER) &&
				(scanner->position == 8) &&
				(!strcmp (scanner->value.v_identifier, "password")))
		{
			last_line = scanner->line;
			g_scanner_get_next_token (scanner);
			g_scanner_get_next_token (scanner);
			
			/* Check that the following arguments are for /lib/security/pam_pwdb.so. */
			if ((scanner->token == G_TOKEN_IDENTIFIER) &&
					(!strcmp (scanner->value.v_identifier, "/lib/security/pam_pwdb.so")))
				
				/* Cool: search all identifiers on the same line */
				while ((g_scanner_peek_next_token (scanner) != G_TOKEN_EOF) && 
							 (scanner->next_line == last_line) &&
							 !found)
			    {
						g_scanner_get_next_token (scanner);
				
						/* Is this the md5 argument? */
						if ((scanner->token == G_TOKEN_IDENTIFIER) &&
								(!strcmp (scanner->value.v_identifier, "md5")))
						{
							found = TRUE;
							break;
						}
					}
		}
	}
	
	g_scanner_destroy (scanner);
	close (fd);
	
	return found;
}

/* str must be a string of len + 1 allocated gchars */
static gchar *
passwd_rand_str (gchar *str, gint len)
{
 	gchar alphanum[] = "0ab1cd2ef3gh4ij5kl6mn7op8qr9st0uvwxyz0AB1CD2EF3GH4IJ5KL6MN7OP8QR9ST0UVWXYZ";
	gint i, alnum_len;
	
	alnum_len = strlen (alphanum);
	str[len] = 0;
	
	for (i = 0; i < len; i++) 
		str[i] = alphanum [(gint) ((((float) alnum_len) * rand () / (RAND_MAX + 1.0)))];
	
	return str;
}

gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
}

