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
#include "e-table.h"

static gboolean is_valid_name (gchar *str);
static gint char_sort_func (gconstpointer a, gconstpointer b);

login_defs logindefs;

extern gboolean
user_add (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_user_name, *new_group_name, *new_comment, *tmp;
	gchar *name, *gid;
	GList *tmp_list;
	gboolean found = FALSE;

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

	tmp_list = get_user_list ("name", FALSE);
	while (tmp_list)
	{
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (name, new_user_name))
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

	tmp_list = get_group_list ("name", FALSE);
	while (tmp_list)
	{
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (name, new_group_name))
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

		e_table_add_group (new_group_name);
		gid = e_table_get_group_by_data ("name", new_group_name, "gid");
	}
 	else
	{
		gid = e_table_get_group_by_data ("name", new_group_name, "gid");
	}

	/* Everything should be ok, let's create a new user */

	e_table_add_user (new_user_name);
	e_table_change_user_full ("login", new_user_name, "gid", gid);

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
	{
		gchar *new_shell;

		new_shell = (gtk_entry_get_text (GTK_ENTRY (tool_widget_get
						("user_settings_shell"))));

		/* TODO Check if shell is valid */
		if (strlen (new_shell) > 0)
			e_table_change_user_full ("login", new_user_name, "shell", new_shell);
	}
	
	w0 = tool_widget_get ("user_settings_comment");
	new_comment = gtk_entry_get_text (GTK_ENTRY (w0));
	if (strlen (new_comment) > 0)
		e_table_change_user_full ("login", new_user_name, "comment", new_comment);

	return TRUE;
}

extern gboolean
user_update (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_login, *new_comment, *new_group_name;
	gchar *login, *comment, *gid;
	gchar *name = NULL;
	GList *tmp_list;
	gboolean found = FALSE;
	gboolean comp;

	w0 = tool_widget_get ("user_settings_name");
	new_login = gtk_entry_get_text (GTK_ENTRY (w0));
	login = e_table_get_user ("login");

	win = GTK_WINDOW (tool_widget_get ("user_settings_dialog"));

	/* If login name is changed and isn't empty */
	if (strcmp (new_login, login))
	{
		if (strlen (new_login) < 1)
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented 
					(_("The username is empty."), win));
			
			gnome_dialog_run (dialog);
			gtk_widget_grab_focus (w0);
			return FALSE;
		}

		if (!is_valid_name (new_login))
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (
			("Please set a valid username, using only lower-case letters."), win));
			gnome_dialog_run (dialog);
			return FALSE;
		}

		e_table_change_user ("login", new_login);
	}

	w0 = tool_widget_get ("user_settings_comment");
	new_comment = gtk_entry_get_text (GTK_ENTRY (w0));
	comment = e_table_get_user ("comment");
	
	if (comment == NULL)
	{
		if (strlen (new_comment) > 0)
			e_table_change_user ("comment", new_comment);
	}
	else if (strcmp (new_comment, comment))
		e_table_change_user ("comment", new_comment);

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
	{
		gchar *shell, *new_shell;

		shell = e_table_get_user ("shell");
		new_shell = (gtk_entry_get_text (GTK_ENTRY (tool_widget_get
						("user_settings_shell"))));

		/* TODO Check if shell is valid */
		if (shell == NULL)
		{
			if (strlen (new_shell) > 0)
				e_table_change_user ("shell", new_shell);
		}
		else if (strcmp (new_shell, shell))
			e_table_change_user ("shell", new_shell);

		g_free (shell);
	}

	/* Get selected group name */
	
	w0 = tool_widget_get ("user_settings_group");
	new_group_name = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (w0)->entry), 0, -1);
	
	/* Now find group's gid */
	
	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
		comp = TRUE;
	else
		comp = FALSE;

	tmp_list = get_group_list ("name", comp);
	while (tmp_list)
	{
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (name, new_group_name))
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
/*
		new_group = group_new ();
		new_group->ug.name = g_strdup (new_group_name);
		u->gid = new_group->ug.id;
		
		group_adv_list = g_list_append (group_adv_list, new_group);
		if (!user_group_is_system (&new_group->ug))
			group_basic_list = g_list_append (group_basic_list, new_group);
*/
	}
	
	else
	{
		gid = (e_table_get_group_by_data ("name", name, "gid"));
		e_table_change_user ("gid", gid);
	}

	return TRUE;
}
		
extern void
user_fill_settings_group (GtkCombo *combo, gboolean adv)
{
	GList *tmp_list, *items;
	gchar *name;

	items = NULL;
	tmp_list = get_group_list ("name", adv);
	while (tmp_list)
	{
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		items = g_list_append (items, name);
	}

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}
		
extern gboolean
group_add (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_group_name, *tmp, *name;
	GList *tmp_list;
	GtkCList *clist;
	gint row;

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

	tmp_list = get_group_list ("name", FALSE);
	while (tmp_list)
	{
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		if (!strcmp (name, new_group_name))
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

	e_table_add_group (new_group_name);
	
	/* Add group members */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (gtk_clist_get_text (clist, row++, 0, &tmp))
		e_table_add_group_users_full (new_group_name, tmp);

	return TRUE;
}

extern gboolean
group_update (void)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *txt, *name;
	GtkCList *clist;
	gint row;
	
	win = GTK_WINDOW (tool_widget_get ("group_settings_dialog"));

	w0 = tool_widget_get ("group_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));
	name = e_table_get_group ("name");

	if (strcmp (name, txt))
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
			e_table_change_group ("name", txt);
	}

	/* Update group members also */
	/* First, free our old users list ... */

	e_table_del_group_users ();
	
	/* ... and then, build new one */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (gtk_clist_get_text (clist, row++, 0, &txt))
		e_table_add_group_users (txt);

	return TRUE;
}

extern gchar *
find_new_id (gchar from)
{
	GList *tmp_list;
	guint ret = 0;
	gint id;
	
	if (from == GROUP)
	{
		ret = logindefs.new_group_min_id;

		tmp_list = get_group_list ("gid", FALSE);
		while (tmp_list)
		{
			id = atoi (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= id)
				ret = id + 1;
		}
		
		if (ret > logindefs.new_group_max_id)
		{
			g_warning ("new gid is out of bounds");
			return NULL;
		}
		
		return g_strdup_printf ("%d", ret);
	}
	
	if (from == USER)
	{
		ret = logindefs.new_user_min_id;

		tmp_list = get_user_list ("uid", FALSE);
		while (tmp_list)
		{
			id = atoi (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= id)
				ret = id + 1;
		}
		
		if (ret > logindefs.new_user_max_id)
		{
			g_warning ("new gid is out of bounds");
			return NULL;
		}
		
		return g_strdup_printf ("%d", ret);
	}

	/* Shouldn't reach here */
	
	g_warning ("find_last_id: wrong parameter");
	return NULL;
}

extern gchar *
find_new_key (gchar from)
{
	GList *tmp_list;
	guint ret = 0;
	gint key;
	gchar *buf;

	if (from == GROUP)
	{
		/* FALSE, cause we want ALL keys */
		tmp_list = get_group_list ("key", FALSE);
		while (tmp_list)
		{
			key = atoi (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= key)
				ret = key + 1;
		}

		buf = g_strdup_printf ("%06d", ret);
		
		return buf;
	}

	if (from == USER)
	{
		/* FALSE, cause we want ALL keys */
		tmp_list = get_user_list ("key", FALSE);
		while (tmp_list)
		{
			key = atoi (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= key)
				ret = key + 1;
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

	tmp_list = e_table_get_group_users ();

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
	gchar *name, *gname;
	gboolean found;
	gchar *entry[2];
	gboolean comp = FALSE;

	entry[1] = NULL;

	clist = GTK_CLIST (tool_widget_get ("group_settings_all"));
	gtk_clist_set_auto_sort (clist, TRUE);
	gtk_clist_freeze (clist);

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
		comp = TRUE;

	tmp_list = get_user_list ("login", comp);
	while (tmp_list)
	{
		gname = tmp_list->data;
		tmp_list = tmp_list->next;

		found = FALSE;

		member = member_rows;
		while (member)
		{
			name = member->data;
			member = member->next;

			if (!strcmp (name, gname))
			{
				found = TRUE;
				break;
			}
		}

		if (!found)
		{
			entry[0] = gname;
			gtk_clist_append (clist, entry);
		}
	}

	gtk_clist_thaw (clist);
}


extern GList *
get_group_list (gchar *field, gboolean adv)
{
	GList *list = NULL;
	xmlNodePtr root, node, u;
	gint gid;
	gchar *txt;

	root = e_table_get_table_data (GROUP);

	for (u = xml_element_find_first (root, "group"); u; u = xml_element_find_next (u, "group"))
	{
		node = xml_element_find_first (u, "gid");
		if (!node)
			break;

		txt = (xml_element_get_content (node));
		gid = atoi (txt);

		if (!adv || (gid >= logindefs.new_group_min_id &&
					gid <= logindefs.new_group_max_id))

		{
			if (strcmp (field, "gid"))
			{
				node = xml_element_find_first (u, field);
				if (!node)
					break;
				txt = xml_element_get_content (node);
			}
				
			list = g_list_prepend (list, txt);
		}
	}

	return list;
}

extern GList *
get_user_list (gchar *field, gboolean adv)
{
	GList *list = NULL;
	xmlNodePtr root, node, u;
	gint id;
	gchar *txt;

	root = e_table_get_table_data (USER);

	for (u = xml_element_find_first (root, "user"); u; u = xml_element_find_next (u, "user"))
	{
		node = xml_element_find_first (u, "uid");
		if (!node)
			break;

		txt = (xml_element_get_content (node));
		id = atoi (txt);

		if (!adv || (id >= logindefs.new_user_min_id && id <= logindefs.new_user_max_id))
		{
			if (strcmp (field, "uid"))
			{
				node = xml_element_find_first (u, field);
				if (!node)
					break;
				txt = xml_element_get_content (node);
			}

			list = g_list_prepend (list, txt);
		}
	}

	return list;
}


extern gchar *
get_group_by_data (gchar *field, gchar *fdata, gchar *data)
{
	xmlNodePtr root, node, u;

	root = e_table_get_table_data (GROUP);

	for (u = xml_element_find_first (root, "group"); u; u = xml_element_find_next (u, "group"))
	{
		node = xml_element_find_first (u, field);
		if (!node)
			break;

		if (!strcmp (fdata, xml_element_get_content (node)))
		{
			node = xml_element_find_first (u, data);
			if (!node)
				break;

			return xml_element_get_content (node);
		}
	}

	return NULL;
}

int
basic_user_count (xmlNodePtr parent)
{
	xmlNodePtr node, u;
	gint ret = 0;
	gint uid;

	g_return_val_if_fail (parent != NULL, 0);

	for (node = parent->childs; node;)
	{
		u = xml_element_find_first (node, "uid");
		node = node->next;

		if (!u)
			continue;

		uid = atoi (xml_element_get_content (u));
		if (uid >= logindefs.new_user_min_id && uid <= logindefs.new_user_max_id)
			ret++;
	}

	return ret;
}

xmlNodePtr
basic_user_find_nth (xmlNodePtr parent, int n)
{
	xmlNodePtr node, u;
	gint i = -1;
	gint uid;

	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->childs; node; node = node->next)
	{
		if (strcmp ("user", node->name))
			continue;

		u = xml_element_find_first (node, "uid");
		if (!u)
			continue;

		uid = atoi (xml_element_get_content (u));
		if (uid >= logindefs.new_user_min_id && uid <= logindefs.new_user_max_id)
			i++;

		if (i == n)
			break;
	}

	return node;
}

int
basic_group_count (xmlNodePtr parent)
{
	xmlNodePtr node, u;
	gint ret = 0;
	gint uid;

	g_return_val_if_fail (parent != NULL, 0);

	for (node = parent->childs; node;)
	{
		u = xml_element_find_first (node, "gid");
		node = node->next;

		if (!u)
			continue;

		uid = atoi (xml_element_get_content (u));
		if (uid >= logindefs.new_group_min_id && uid <= logindefs.new_group_max_id)
			ret++;
	}

	return ret;
}

xmlNodePtr
basic_group_find_nth (xmlNodePtr parent, int n)
{
	xmlNodePtr node, u;
	gint i = -1;
	gint uid;

	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->childs; node; node = node->next)
	{
		if (strcmp ("group", node->name))
			continue;

		u = xml_element_find_first (node, "gid");
		if (!u)
			continue;

		uid = atoi (xml_element_get_content (u));
		if (uid >= logindefs.new_group_min_id && uid <= logindefs.new_group_max_id)
			i++;

		if (i == n)
			break;
	}

	return node;
}

void
adv_user_settings (gboolean show)
{
	GtkWidget *win, *w0;

	win = tool_widget_get ("user_settings_dialog");
	w0 = tool_widget_get ("user_settings_advanced");

	if (show)
	{	
		gtk_entry_set_text (GTK_ENTRY (tool_widget_get ("user_settings_shell")),
				e_table_get_user ("shell"));

		gtk_entry_set_text (GTK_ENTRY (tool_widget_get ("user_settings_home")),
				e_table_get_user ("home"));

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool_widget_get ("user_settings_uid")),
				g_strtod (e_table_get_user ("uid"), NULL));

		gtk_widget_show (w0);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY (tool_widget_get ("user_settings_shell")), "");
		gtk_entry_set_text (GTK_ENTRY (tool_widget_get ("user_settings_home")), "");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool_widget_get
				("user_settings_uid")), 0);

		gtk_widget_hide (w0);
	}
}

void
adv_user_settings_new (void)
{
	GtkWidget *w0;

	w0 = tool_widget_get ("user_settings_advanced");
	gtk_widget_show (w0);
}

