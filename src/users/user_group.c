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
#include "callbacks.h"
#include "e-table.h"


static gboolean is_valid_name (gchar *str);
static gint char_sort_func (gconstpointer a, gconstpointer b);

login_defs logindefs;

extern gboolean
user_add (gchar type)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_user_name, *new_group_name, *new_comment, *tmp;
	gchar *name, *gid;
	GList *tmp_list;
	gboolean found = FALSE;
	gboolean user_exists = FALSE;
	xmlNodePtr node;

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

		if (!user_exists && !strcmp (name, new_user_name))
		{
			tmp = g_strdup_printf (_("User %s already exists."), new_user_name);
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (tmp, win));
			gnome_dialog_run (dialog);
			g_free (tmp);

			gtk_widget_grab_focus (w0);
			gtk_editable_select_region (GTK_EDITABLE (w0), 0, -1);
			user_exists = TRUE;
		}
		g_free (tmp_list->data);
		tmp_list = tmp_list->next;
	}
	g_list_free (tmp_list);

	if (user_exists)
		return FALSE;

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

		if (!found && !strcmp (name, new_group_name))
			found = TRUE;

		g_free (tmp_list->data);
		tmp_list = tmp_list->next;

	}
	g_list_free (tmp_list);

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

		node = group_add_to_xml (new_group_name, LOCAL);
		gid = my_xml_get_content (node, "gid");
		e_table_add_group ();
	}
 	else
	{
		gid = get_group_by_data ("name", new_group_name, "gid");
	}

	/* Everything should be ok, let's create a new user */

	node = user_add_to_xml (new_user_name, type);
	if (type == LOCAL)
		e_table_add_user ();

	my_xml_set_child_content (node, "gid", gid);
	g_free (gid);

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
		adv_user_settings_update (node, new_user_name);

	w0 = tool_widget_get ("user_settings_comment");
	new_comment = gtk_entry_get_text (GTK_ENTRY (w0));
	if (strlen (new_comment) > 0)
	{
		my_xml_set_child_content (node, "comment", new_comment);
		e_table_change_user ();
	}

	/* Ask for password */
	user_passwd_dialog_show (node);

	return TRUE;
}

extern gboolean
user_update (xmlNodePtr node)
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
	xmlNodePtr g_node;

	g_return_val_if_fail (login = my_xml_get_content (node, "login"), FALSE);

	w0 = tool_widget_get ("user_settings_name");
	new_login = gtk_entry_get_text (GTK_ENTRY (w0));

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
			g_free (login);
			return FALSE;
		}

		if (!is_valid_name (new_login))
		{
			dialog = GNOME_DIALOG (gnome_error_dialog_parented (
			("Please set a valid username, using only lower-case letters."), win));
			gnome_dialog_run (dialog);
			g_free (login);
			return FALSE;
		}

		my_xml_set_child_content (node, "login", new_login);
		e_table_change_user ();
	}
	g_free (login);

	/* Change comment if comment is changed. */
	w0 = tool_widget_get ("user_settings_comment");
	new_comment = gtk_entry_get_text (GTK_ENTRY (w0));
	comment = my_xml_get_content (node, "comment");
	
	if (comment == NULL)
	{
		if (strlen (new_comment) > 0)
		{
			my_xml_set_child_content (node, "comment", new_comment);
			e_table_change_user ();
		}
	}
	else if (strcmp (new_comment, comment))
	{
		my_xml_set_child_content (node, "comment", new_comment);
		e_table_change_user ();
	}

	g_free (comment);

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
		adv_user_settings_update (node, new_login);

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

		if (!found && !strcmp (name, new_group_name))
			found = TRUE;

		g_free (tmp_list->data);
		tmp_list = tmp_list->next;
	}
	g_free (tmp_list);
	
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

		g_node = group_add_to_xml (new_group_name, LOCAL);
		gid = my_xml_get_content (g_node, "gid");
		e_table_add_group ();
	}

	else
		gid = get_group_by_data ("name", new_group_name, "gid");

	my_xml_set_child_content (node, "gid", gid);
	g_free (gid);

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
	g_list_free (tmp_list);

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}
		
extern gboolean
group_add (gchar type)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *new_group_name, *tmp, *name;
	GList *tmp_list;
	GtkCList *clist;
	gint row;
	xmlNodePtr node;

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

	node = group_add_to_xml (new_group_name, type);
	if (type == LOCAL)
		e_table_add_group ();
	
	/* Add group members */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (gtk_clist_get_text (clist, row++, 0, &tmp))
		add_group_users (node, tmp);

	return TRUE;
}

extern gboolean
group_update (xmlNodePtr node)
{
	GtkWidget *w0;
	GtkWindow *win;
	GnomeDialog *dialog;
	gchar *txt, *name;
	GtkCList *clist;
	gint row;

	g_return_val_if_fail (name = my_xml_get_content (node, "name"), FALSE);

	win = GTK_WINDOW (tool_widget_get ("group_settings_dialog"));

	w0 = tool_widget_get ("group_settings_name");
	txt = gtk_entry_get_text (GTK_ENTRY (w0));

	if (strcmp (name, txt))
	{
		g_free (name);
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
			my_xml_set_child_content (node, "name", txt);
			e_table_change_group ();
		}
	}
	else
		g_free (name);

	/* Update group members also */
	/* First, free our old users list ... */

	del_group_users (node);

	/* ... and then, build new one */

	clist = GTK_CLIST (tool_widget_get ("group_settings_members"));

	row = 0;
	while (gtk_clist_get_text (clist, row++, 0, &txt))
		add_group_users (node, txt);

	return TRUE;
}

gchar *
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
			g_free (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= id)
				ret = id + 1;
		}
		g_list_free (tmp_list);
		
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
			g_free (tmp_list->data);
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
			g_free (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= key)
				ret = key + 1;
		}
		g_list_free (tmp_list);

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
			g_free (tmp_list->data);
			tmp_list = tmp_list->next;

			if (ret <= key)
				ret = key + 1;
		}
		g_list_free (tmp_list);

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

gboolean
is_free_uid (gint new_uid)
{
	GList *tmp_list;
	gint uid;
	gboolean found = FALSE;

	/* Check if uid does not exsist. */
	
	tmp_list = get_user_list ("uid", FALSE);
	while (tmp_list)
	{
		uid = atoi (tmp_list->data);
		g_free (tmp_list->data);
		tmp_list = tmp_list->next;

		if (found)
			continue;

		if (uid == new_uid)
			found = TRUE;
	}
	g_list_free (tmp_list);

	if (found)
		return FALSE;

	return TRUE;
}

static gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
}

extern GList *
group_fill_members_list (xmlNodePtr node)
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

	tmp_list = get_group_users (node);

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
	g_list_free (tmp_list);

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
			continue;

		txt = xml_element_get_content (node);
		gid = atoi (txt);

		if (!adv || (gid >= logindefs.new_group_min_id &&
					gid <= logindefs.new_group_max_id))

		{
			if (strcmp (field, "gid"))
			{
				node = xml_element_find_first (u, field);
				if (!node)
				{
					g_free (txt);
					continue;
				}
				g_free (txt);
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
			continue;

		txt = xml_element_get_content (node);
		id = atoi (txt);

		if (!adv || (id >= logindefs.new_user_min_id && id <= logindefs.new_user_max_id))
		{
			if (strcmp (field, "uid"))
			{
				node = xml_element_find_first (u, field);
				if (!node)
				{
					g_free (txt);
					continue;
				}
				g_free (txt);
				txt = xml_element_get_content (node);
			}

			list = g_list_prepend (list, txt);
		}
	}

	return list;
}


gchar *
get_group_by_data (gchar *field, gchar *fdata, gchar *data)
{
	xmlNodePtr root, node, u;
	gchar *content;

	root = e_table_get_table_data (GROUP);

	for (u = xml_element_find_first (root, "group"); u; u = xml_element_find_next (u, "group"))
	{
		node = xml_element_find_first (u, field);
		if (!node)
			break;

		content = xml_element_get_content (node);
		if (!strcmp (fdata, content))
		{
			g_free (content);
			node = xml_element_find_first (u, data);
			if (!node)
				break;

			return xml_element_get_content (node);
		}
		else
			g_free (content);
	}

	return NULL;
}

int
basic_user_count (xmlNodePtr parent)
{
	xmlNodePtr node, u;
	gint ret = 0;
	gint uid;
	gchar *content;

	g_return_val_if_fail (parent != NULL, 0);

	for (node = parent->childs; node;)
	{
		u = xml_element_find_first (node, "uid");
		node = node->next;

		if (!u)
			continue;

		content = xml_element_get_content (u);
		uid = atoi (content);
		g_free (content);
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
	gchar *content;

	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->childs; node; node = node->next)
	{
		if (strcmp ("user", node->name))
			continue;

		u = xml_element_find_first (node, "uid");
		if (!u)
			continue;

		content = xml_element_get_content (u);
		uid = atoi (content);
		g_free (content);
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
	gchar *content;

	g_return_val_if_fail (parent != NULL, 0);

	for (node = parent->childs; node;)
	{
		u = xml_element_find_first (node, "gid");
		node = node->next;

		if (!u)
			continue;

		content = xml_element_get_content (u);
		uid = atoi (content);
		g_free (content);
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
	gchar *content;

	g_return_val_if_fail (parent != NULL, NULL);

	for (node = parent->childs; node; node = node->next)
	{
		if (strcmp ("group", node->name))
			continue;

		u = xml_element_find_first (node, "gid");
		if (!u)
			continue;

		content = xml_element_get_content (u);
		uid = atoi (content);
		g_free (content);
		if (uid >= logindefs.new_group_min_id && uid <= logindefs.new_group_max_id)
			i++;

		if (i == n)
			break;
	}

	return node;
}

void
adv_user_settings (xmlNodePtr node, gboolean show)
{
	GtkWidget *win, *adv, *w0;
	gchar *content;

	win = tool_widget_get ("user_settings_dialog");
	adv = tool_widget_get ("user_settings_advanced");

	if (show)
	{
		g_return_if_fail (node != NULL);

		/* Shell. */
		w0 = tool_widget_get ("user_settings_shell");
		content = my_xml_get_content (node, "shell");
		gtk_entry_set_text (GTK_ENTRY (w0), content);
		g_free (content);
		gtk_widget_set_sensitive (w0, tool_get_access());

		/* Home dir. */
		w0 = tool_widget_get ("user_settings_home");
		content = my_xml_get_content (node, "home");
		gtk_entry_set_text (GTK_ENTRY (w0), content);
		g_free (content);
		gtk_widget_set_sensitive (w0, tool_get_access());

		/* User ID. */
		w0 = tool_widget_get ("user_settings_uid");
		content = my_xml_get_content (node, "uid");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), g_strtod (content, NULL));
		g_free (content);

		gtk_widget_set_sensitive (w0, tool_get_access());

		/* Show advanced frame. */
		gtk_widget_show (adv);
	}
	else
	{
		/* Clear home, shell and user id. */
		gtk_entry_set_text (GTK_ENTRY (tool_widget_get ("user_settings_shell")), "");
		gtk_entry_set_text (GTK_ENTRY (tool_widget_get ("user_settings_home")), "");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (tool_widget_get
				("user_settings_uid")), 0);

		/* Hide advanced frame. */
		gtk_widget_hide (adv);
	}
}

void
adv_user_settings_new (void)
{
	GtkWidget *w0;
	gfloat uid;

	/* Set new first available UID */
	w0 = tool_widget_get ("user_settings_uid");
	uid = g_strtod (find_new_id (USER), NULL);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w0), uid);
	
	w0 = tool_widget_get ("user_settings_advanced");
	gtk_widget_show (w0);
}

void
adv_user_settings_update (xmlNodePtr node, gchar *login)
{
	gchar *new_shell;
	gchar *new_home;
	gint new_uid;

	g_return_if_fail (node != NULL);

	/* Shell */
	new_shell = (gtk_entry_get_text (GTK_ENTRY (tool_widget_get ("user_settings_shell"))));

	/* TODO Check if shell is valid */
	if (strlen (new_shell) > 0)
	{
		my_xml_set_child_content (node, "shell", new_shell);
		e_table_change_user ();
	}

	/* Home */	
	new_home = (gtk_entry_get_text (GTK_ENTRY (tool_widget_get ("user_settings_home"))));
	if (strlen (new_home) > 0)
	{
		my_xml_set_child_content (node, "home", new_home);
		e_table_change_user ();
	}

	/* UID */
	new_uid = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (tool_widget_get
			("user_settings_uid")));

	if (is_free_uid (new_uid)) 
	{
		my_xml_set_child_content (node, "uid", g_strdup_printf ("%d", new_uid));
		e_table_change_user ();
	}
}

gchar *
my_xml_get_content (xmlNodePtr parent, gchar *name)
{
	xmlNodePtr node;

	node = xml_element_find_first (parent, name);
	if (!node)
		return NULL;

	return xml_element_get_content (node);
}

GList *
get_group_users (xmlNodePtr node)
{
	GList *userlist = NULL;
	xmlNodePtr users, u;

	g_return_val_if_fail (node != NULL, NULL);

	users = xml_element_find_first (node, "users");
	if (!users)
	{
		g_warning ("get_group_users: can't get current group's users node.");
		return NULL; 
	}

	for (u = xml_element_find_first (users, "user"); u; u = xml_element_find_next (u, "user"))
		userlist = g_list_prepend (userlist, xml_element_get_content (u));

	return userlist;
}

void
del_group_users (xmlNodePtr node)
{
	xmlNodePtr users;

	g_return_if_fail (node != NULL);

	users = xml_element_find_first (node, "users");
	if (!users)
	{
		g_warning ("e_table_del_group_users: can't get current group's users node.");
		return; 
	}

	xml_element_destroy_children (users);
}

void
add_group_users (xmlNodePtr node, gchar *name)
{
	xmlNodePtr users, u;

	g_return_if_fail (node != NULL);

	users = xml_element_find_first (node, "users");

	if (!users)
	{
		g_warning ("add_group_users: can't get current group's users node.");
		return; 
	}

	u = xml_element_add (users, "user");
	xml_element_set_content (u, name);
}

static void
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}


extern void
group_settings_prepare (xmlNodePtr node)
{
	GtkWidget *w0;
	GList *member_rows;
	gchar *txt, *name;

	g_return_if_fail (node != NULL);
	g_return_if_fail (name = my_xml_get_content (node, "name"));

	w0 = tool_widget_get ("group_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, name);

	/* Fill group members */
	member_rows = group_fill_members_list (node);

	/* Fill all users list */
	group_fill_all_users_list (member_rows);

	while (member_rows)
	{
		g_free (member_rows->data);
		member_rows = member_rows->next;
	}
	g_list_free (member_rows);

	/* Show group settings dialog */
	
	w0 = tool_widget_get ("group_settings_dialog");
	txt = g_strdup_printf (_("Settings for Group %s"), name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (name);
	g_free (txt);
	gtk_widget_show (w0);
}

extern void
user_settings_prepare (xmlNodePtr node)
{
	GtkWidget *w0;
	GList *tmp_list;
	gchar *txt;
	gboolean found = FALSE;
	gchar *login, *comment, *name = NULL;
	gint gid, id = 0;
	gint new_id = 0;
	gboolean comp = FALSE;
	GtkRequisition req;

	g_return_if_fail (node != NULL);
	g_return_if_fail (login = my_xml_get_content (node, "login"));

	/* Get tool state (advanced/basic */

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
		comp = TRUE;

	/* Fill login name entry */	
	w0 = tool_widget_get ("user_settings_name");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, login);
	g_free (login);

	/* Fill groups combo */
	w0 = tool_widget_get ("user_settings_group");
	gtk_widget_set_sensitive (w0, tool_get_access());
	user_fill_settings_group (GTK_COMBO (w0), comp);

	txt = my_xml_get_content (node, "gid");
	gid = atoi (txt);
	g_free (txt); 

	tmp_list = get_group_list ("gid", comp);
	while (tmp_list)
	{
		id = atoi (tmp_list->data);
		g_free (tmp_list->data); 
		tmp_list = tmp_list->next;

		if (!found && id == gid)
		{
			new_id = id;
			found = TRUE;
		}
	}
	g_list_free (tmp_list);

	if (!found) 
	{
		g_warning ("The GID for the main user's group was not found.");
		name = g_strdup (_("Unkown User"));
	}
	else
	{
		txt = g_strdup_printf ("%d", new_id);
		name = get_group_by_data ("gid", txt, "name");
		my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), name);
		g_free (txt);
	}       

	/* Fill comment entry */
	comment = my_xml_get_content (node, "comment");
	w0 = tool_widget_get ("user_settings_comment");
	gtk_widget_set_sensitive (w0, tool_get_access());
	my_gtk_entry_set_text (w0, comment);
	g_free (comment);

	/* If state == advanced, fill advanced settings too. */
	if (!comp)
		adv_user_settings (node, TRUE);

	/* Set dialog's title and show it */
	w0 = tool_widget_get ("user_settings_dialog");
	txt = g_strdup_printf (_("Settings for User %s"), name);
	g_free (name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);

	/* Resize it to minimum */
	gtk_widget_size_request (w0, &req);
	gtk_window_set_default_size (GTK_WINDOW (w0), req.width, req.height);

	/* Add 0 to windows data refering that we are not making new user */
	gtk_object_set_data (GTK_OBJECT (w0), "new", GUINT_TO_POINTER (0));
	gtk_widget_show (w0);
}

extern void
user_new_prepare (gchar *group_name)
{
	GtkWidget *w0;
	gboolean comp = FALSE;

	if (tool_get_complexity () == TOOL_COMPLEXITY_BASIC)
		comp = TRUE;

	w0 = tool_widget_get ("user_settings_group");
	user_fill_settings_group (GTK_COMBO (w0), comp);
	my_gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (w0)->entry), group_name);

	if (tool_get_complexity () == TOOL_COMPLEXITY_ADVANCED)
		adv_user_settings_new ();

	w0 = tool_widget_get ("user_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), "Create New User");
	gtk_object_set_data (GTK_OBJECT (w0), "new", GUINT_TO_POINTER (1));
	gtk_widget_show (w0);
}

extern void
group_new_prepare (void)
{
	GtkWidget *w0;

	/* Fill all users list */
	group_fill_all_users_list (NULL);

	/* Show group settings dialog */

	w0 = tool_widget_get ("group_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), _("Create New Group"));
	gtk_object_set_data (GTK_OBJECT (w0), "new", GUINT_TO_POINTER (1));
	gtk_widget_show (w0);
}

void
my_xml_set_child_content (xmlNodePtr parent, gchar *name, gchar *val)
{
	xmlNodePtr node;

	g_return_if_fail (parent != NULL);
	g_return_if_fail (name != NULL);
	g_return_if_fail (val != NULL);

	node = xml_element_find_first (parent, name);
	if (!node)
	{
		g_warning ("my_xml_set_child: can't get field %s.", name);
		return;
	}

	xml_element_set_content (node, val);
}

xmlNodePtr
group_add_to_xml (gchar *name, gchar type)
{
	xmlNodePtr root, group;

	root = xml_doc_get_root (tool_config_get_xml());

	switch (type)
	{
		case LOCAL:
			root = xml_element_find_first (root, "groupdb");
			break;
		default:
			g_warning ("group_add_to_xml: wrong type of group");
			return NULL;
	}

	group = xml_element_add (root, "group");

	xml_element_add_with_content (group, "key", find_new_key (GROUP));
	xml_element_add_with_content (group, "name", name);
	xml_element_add_with_content (group, "password", "x");
	xml_element_add_with_content (group, "gid", find_new_id (GROUP));
	xml_element_add (group, "users");

	return group;
}

xmlNodePtr
user_add_to_xml (gchar *name, gchar type)
{
	xmlNodePtr root, user;

	root = xml_doc_get_root (tool_config_get_xml());

	switch (type)
	{
		case LOCAL:
			root = xml_element_find_first (root, "userdb");
			break;
		default:
			g_warning ("user_add_to_xml: wrong type of group");
			return NULL;
	}

	user = xml_element_add (root, "user");

	xml_element_add_with_content (user, "key", find_new_key (USER));
	xml_element_add_with_content (user, "login", name);
	xml_element_add (user, "password");
	xml_element_add_with_content (user, "uid", find_new_id (USER));
	xml_element_add (user, "gid");
	xml_element_add (user, "comment");

	if (logindefs.create_home)
		xml_element_add_with_content (user, "home", g_strdup_printf ("/home/%s", name));
	xml_element_add_with_content (user, "shell", g_strdup ("/bin/bash"));
	xml_element_add (user, "last_mod");

	xml_element_add_with_content (user, "passwd_min_life",
			g_strdup_printf ("%d", logindefs.passwd_min_day_use));

	xml_element_add_with_content (user, "passwd_max_life",
			g_strdup_printf ("%d", logindefs.passwd_max_day_use));

	xml_element_add_with_content (user, "passwd_exp_warn",
			g_strdup_printf ("%d", logindefs.passwd_warning_advance_days));
	xml_element_add (user, "passwd_exp_disable");
	xml_element_add (user, "passwd_disable");
	xml_element_add (user, "reserved");
	xml_element_add_with_content (user, "is_shadow", g_strdup ("1"));

	return user;
}

