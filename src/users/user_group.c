/* user_group.c: this file is part of users-admin, a helix-setup-tool frontend 
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

#include <stdlib.h>
#include <gnome.h>

#include "global.h"
#include "user_group.h"

#define USER 1
#define GROUP 2

static guint find_new_id (gchar from);
static gchar *find_new_key (gchar from);
static gboolean is_valid_name (gchar *str);
static gint char_sort_func (gconstpointer a, gconstpointer b);

GList *user_list = NULL;
user *current_user = NULL;

GList *group_list = NULL;
group *current_group = NULL;

login_defs logindefs;

extern user *
user_new (gchar *name)
{
	user *u;

	u = g_new0 (user, 1);
	u->key = find_new_key (USER);
	u->uid = find_new_id (USER);
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

extern gboolean
user_add (void)
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
		
		new_group = group_new ();
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

	new_user = user_new (new_user_name);
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

extern gboolean
user_update (user *u)
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
	if (strcmp (txt, u->login))
	{
		if (strlen (txt) < 1)
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					(_("The username is empty."), win));
			
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

		g_free (u->login);
		u->login = g_strdup (txt);

		list = GTK_CLIST (tool_widget_get ("user_list"));
		row = gtk_clist_find_row_from_data (list, u);
		gtk_clist_set_text (list, row, 0, txt);
	}

	w0 = tool_widget_get ("user_settings_comment");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	if (u->comment == NULL)
	{
		if (strlen (txt) > 0)
			u->comment = g_strdup (txt);
	}
	else if (strcmp (txt, u->comment))
	{
		g_free (u->comment);
		u->comment = g_strdup (txt);
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

		new_group = group_new ();
		new_group->name = g_strdup (new_group_name);
		group_list = g_list_append (group_list, new_group);
		u->gid = new_group->gid;

		/* Add to CList */

		list = GTK_CLIST (tool_widget_get ("group_list"));

		entry[0] = new_group_name;
		row = gtk_clist_append (list, entry);
		gtk_clist_set_row_data (list, row, new_group);
	}
	
	else
		u->gid = g->gid;

	return TRUE;
}

extern void
user_fill_settings_group (GtkCombo *combo)
{
	GList *u, *items;

	items = NULL;
	for (u = g_list_first (group_list); u; u = g_list_next (u))
		items = g_list_append (items, ((group *) u->data)->name);

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}

extern group *
group_new (void)
{
	group *gr;
	
	gr = g_new0 (group, 1);

	gr->users = NULL;
	gr->password = g_strdup ("x");
	gr->gid = find_new_id (GROUP);
	gr->key = find_new_key (GROUP);

	return gr;
}

extern gboolean
group_add (void)
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
				(_("The group name is empty."), win));
		
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
		  _("Please set a valid group name, with only lower-case letters."), win));
		gnome_dialog_run (dialog);
		return FALSE;
	}
	

	/* Everything should be ok and we can add new group */

	tmpgroup = group_new ();
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

extern gboolean
group_update (group *g)
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

	if (strcmp (g->name, txt))
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
			g_free (g->name);
			g->name = g_strdup (txt);

			clist = GTK_CLIST (tool_widget_get ("group_list"));
			row = gtk_clist_find_row_from_data (clist, g);
			gtk_clist_set_text (clist, row, 0, txt);
		}
	}

	/* Update group members also */
	/* First, free our old users list ... */

	selection = g->users;
	while (selection)
	{
		txt = selection->data;
		selection = selection->next;

		g_free (txt);
	}
	
	g_list_free (g->users);
	g->users = NULL;

	/* ... and then, build new one */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (gtk_clist_get_text (clist, row++, 0, &txt))
		g->users = g_list_append (g->users, g_strdup (txt));

	return TRUE;
}

static guint
find_new_id (gchar from)
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
	
	if (from == USER)
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

static gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
}

extern GList *
group_fill_members_list (void)
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

extern void
group_fill_all_users_list (GList *member_rows)
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

