/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user_group.c: this file is part of users-admin, a ximian-setup-tool frontend 
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

#include <stdlib.h>
#include <gnome.h>

#include "global.h"
#include "callbacks.h"
#include "user_group.h"
#include "transfer.h"
#include "e-table.h"
#include "user_settings.h"
#include "profile.h"

extern XstTool *tool;

/* Local globals */
static int reply;

login_defs logindefs;

/* Static prototypes */

static void group_settings_prepare (ug_data *ud);
static void user_update_xml (UserSettings *us);
static xmlNodePtr user_add_blank_xml (xmlNodePtr parent);
static void group_update_users (xmlNodePtr node, gchar *old_name, gchar *new_name);
static xmlNodePtr group_add_blank_xml (xmlNodePtr db_node);
static void group_add_from_user (UserSettings *us);
static void group_update_xml (xmlNodePtr node, gboolean adv);
static gboolean node_exsists (xmlNodePtr node, gchar *name, gchar *val);
static GList *get_group_users (xmlNodePtr group_node);
static GList *get_group_mainusers (xmlNodePtr group_node);
static GList *group_fill_members_list (xmlNodePtr node);
static void group_fill_all_users_list (xmlNodePtr node, GList *exclude);
static void del_group_users (xmlNodePtr group_node);
static void add_group_users (xmlNodePtr group_node, gchar *name);
static void del_user_groups (xmlNodePtr user_node);
static void add_user_groups (xmlNodePtr user_node, gchar *group_name);
static gboolean is_valid_name (gchar *str);
static void reply_cb (gint val, gpointer data);
static gchar *parse_home (UserSettings *us);
static gchar *parse_group (UserSettings *us);

/* Global functions */

xmlNodePtr
get_root_node (gint tbl)
{
	xmlNodePtr node, root;

	root = xst_xml_doc_get_root (tool->config);

	switch (tbl)
	{
	case TABLE_USER:
		node = xst_xml_element_find_first (root, "userdb");
		break;
	case TABLE_GROUP:
		node = xst_xml_element_find_first (root, "groupdb");
		break;
	case TABLE_NET_USER:
		node = xst_xml_element_find_first (root, "nis_userdb");
		break;
	case TABLE_NET_GROUP:
		node = xst_xml_element_find_first (root, "nis_groupdb");
		break;
	default:
		node = NULL;
		break;
	}

	return node;
}

xmlNodePtr
get_user_root_node (void)
{
	return xst_xml_element_find_first (xst_xml_doc_get_root (tool->config), "userdb");
}

xmlNodePtr
get_group_root_node (void)
{
	return xst_xml_element_find_first (xst_xml_doc_get_root (tool->config), "groupdb");
}

xmlNodePtr
get_nis_group_root_node (void)
{
	return xst_xml_element_find_first (xst_xml_doc_get_root (tool->config), "nis_groupdb");
}

xmlNodePtr
get_nis_user_root_node (void)
{
	return xst_xml_element_find_first (xst_xml_doc_get_root (tool->config), "nis_userdb");
}

xmlNodePtr
get_db_node (xmlNodePtr node)
{
	while (node)
	{
		if (strstr (node->name, "db"))
			return node;

		node = node->parent;
	}

	return NULL;
}

static gboolean
user_filter (xmlNodePtr node)
{
	gchar *buf;
	gchar **ar;
	gboolean ret;

	ret = FALSE;
	buf = user_query_string_get ();
	ar = g_strsplit (buf, " ", 3);
	
	g_free (buf);
	
	if (!strcmp (ar[0], "all"))
		ret = TRUE;

	else
	{
		buf = xst_xml_get_child_content (node, ar[1]);
		if (buf && strstr (buf, ar[2]))
			ret = TRUE;

		g_free (buf);
	}
	
	g_strfreev (ar);
	
	return ret;
}

gboolean
check_node_complexity (xmlNodePtr node)
{
	xmlNodePtr db_node;
	gchar *field, *content;
	gint min, max, val;
	static GtkToggleButton *toggle;
	XstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;

	if (!toggle)
		toggle = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (tool->main_dialog, "showall"));

	db_node = get_db_node (node);
	get_min_max (db_node, &min, &max);

	if (!strcmp (db_node->name, "userdb"))
	{
		if (!user_filter (node))
			return FALSE;
		
		field = g_strdup ("uid");
	}

	else if (!strcmp (db_node->name, "groupdb"))
		field = g_strdup ("gid");

	else
		return TRUE;

	node = xst_xml_element_find_first (node, field);
	g_free (field);
	content = xst_xml_element_get_content (node);
	val = atoi (content);
	g_free (content);

	if (val >= min && val <= max)
		return TRUE;

	else if (complexity == XST_DIALOG_ADVANCED && gtk_toggle_button_get_active (toggle))
		return TRUE;

	else
		return FALSE;
}

gboolean
check_user_login (xmlNodePtr node, gchar *login)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (login != NULL, FALSE);

	/* If !empty. */
	if (strlen (login) < 1)
		buf = g_strdup (_("The username is empty."));

	/* if valid. */
	else if (!is_valid_name (login))
		buf = g_strdup (_("Please set a valid username, using only lower-case letters."));

	/* if !exsist. */
	else if (node_exsists (node, "login", login))
		buf = g_strdup (_("Username already exsists."));

	/* If anything is wrong. */
	if (buf)
	{
		GtkWindow *win;
		GnomeDialog *dialog;

		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, win));
		gnome_dialog_run (dialog);
		g_free (buf);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_name"));

		return FALSE;
	}

	else
		return TRUE;
}

gboolean
check_user_uid (xmlNodePtr node, gchar *val)
{
	gboolean retval = TRUE;
	GtkWindow *win;
	GnomeDialog *dialog;

	/* Check if uid is available */

	if (node_exsists (node, "uid", val))
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Such user id already exsists."), win));

		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_uid"));
		retval = FALSE;
	}

	return retval;
}

gboolean
check_user_comment (xmlNodePtr node, gchar *val)
{
	/* What could be wrong with comment? */
	return TRUE;
}

gboolean
check_user_home (xmlNodePtr node, gchar *val)
{
	GtkWindow *win;
	GnomeDialog *dialog;

	/* I think every user has to have home dir... FIXME, if I'm wrong. */

	if (strlen (val) < 1)
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Home directory must not be empty."), win));

		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_settings_home"));
		return FALSE;
	}

	return TRUE;
}

gboolean
check_user_shell (xmlNodePtr node, gchar *val)
{
	/* TODO: check shell. */

	return TRUE;
}

gint
check_user_group (UserSettings *us)
{
	gchar *buf;
	GnomeDialog *dialog;
	xmlNodePtr group_node;

	g_return_val_if_fail (us != NULL, -1);

	buf = parse_group (us);
	group_node = get_corresp_field (us->node);

	if (node_exsists (group_node, "name", buf))
		return 0;

	if (!is_valid_name (buf))
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented(_(
			"Please set a valid main group name, with only lower-case letters,"
			"\nor select one from pull-down menu."),
								   GTK_WINDOW (us->dialog)));

		gnome_dialog_run (dialog);
		gtk_widget_grab_focus (GTK_WIDGET (us->group->main));

		return -1;
	}

	/* Group not found, but name is valid. */
	return 1;
}

gboolean
check_group_name (xmlNodePtr node, gchar *name)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	/* If !empty. */
	if (strlen (name) < 1)
		buf = g_strdup (_("Group name is empty."));

	/* if valid. */
	else if (!is_valid_name (name))
		buf = g_strdup (_("Please set a valid group name, using only lower-case letters."));

	/* if !exsist. */
	else if (node_exsists (node, "name", name))
		buf = g_strdup (_("Group already exsists."));

	/* If anything is wrong. */
	if (buf)
	{
		GtkWindow *win;
		GnomeDialog *dialog;

		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, win));
		gnome_dialog_run (dialog);
		g_free (buf);
		gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "group_settings_name"));

		return FALSE;
	}

	else
		return TRUE;
}

gboolean
check_group_gid (xmlNodePtr node, gchar *val)
{
	gboolean retval = TRUE;
	GtkWindow *win;
	GnomeDialog *dialog;

	/* Check if gid is available */

	if (node_exsists (node, "gid", val))
	{
		win = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog"));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Such group id already exsists."), win));

		gnome_dialog_run (dialog);
		retval = FALSE;
	}

	return retval;
}

static gboolean
check_passwd (xmlNodePtr user_node)
{
	/* Only checks if empty */
	gchar *buf;
	gboolean ret;

	ret = FALSE;
	buf = xst_xml_get_child_content (user_node, "password");
	if (strlen (buf) > 0)
		ret = TRUE;

	g_free (buf);

	return ret;
}

gboolean
get_min_max (xmlNodePtr db_node, gint *min, gint *max)
{
	Profile *pf;
	
	g_return_val_if_fail (db_node != NULL, FALSE);

	pf = profile_table_get_profile (NULL);	
	if (!strcmp (db_node->name, "userdb"))
	{
		*min = pf->umin;
		*max = pf->umax;

		return TRUE;
	}

	if (!strcmp (db_node->name, "groupdb"))
	{
		*min = pf->gmin;
		*max = pf->gmax;

		return TRUE;
	}

	else
	{
		/* What is that? let's put min very small and max very BIG */
		*min = 0;
		*max = 100000;
		return TRUE;
	}

	return FALSE;
}

xmlNodePtr
get_corresp_field (xmlNodePtr node)
{
	xmlNodePtr root;

	g_return_val_if_fail (node != NULL, NULL);

	root = xst_xml_doc_get_root (tool->config);
	node = get_db_node (node);

	if (!strcmp (node->name, "userdb"))
		return xst_xml_element_find_first (root, "groupdb");

	if (!strcmp (node->name, "groupdb"))
		return xst_xml_element_find_first (root, "userdb");

	if (!strcmp (node->name, "nis_groupdb"))
		return xst_xml_element_find_first (root, "nis_userdb");

	if (!strcmp (node->name, "nis_userdb"))
		return xst_xml_element_find_first (root, "nis_groupdb");

	return NULL;
}

xmlNodePtr
get_node_by_data (xmlNodePtr dbnode, gchar *field, gchar *fdata)
{
	xmlNodePtr node;
	gchar *buf;

	g_return_val_if_fail (dbnode != NULL, NULL);
	g_return_val_if_fail (field != NULL, NULL);

	if (!fdata)
		return NULL;

	for (node = dbnode->childs; node; node = node->next)
	{
		buf = xst_xml_get_child_content (node, field);
		if (!buf)
			continue;
		
		if (!strcmp (buf, fdata))
		{
			g_free (buf);
			return node;
		}

		g_free (buf);
	}

	return NULL;
}

GList *
get_user_list (gchar *field, xmlNodePtr group_node)
{
	GList *list = NULL;
	xmlNodePtr node, u;

	node = get_corresp_field (group_node);

	for (u = xst_xml_element_find_first (node, "user");
	     u;
	     u = xst_xml_element_find_next (u, "user"))
	{

		if (check_node_complexity (u))
			list = g_list_prepend (list, xst_xml_get_child_content (u, field));
	}

	return list;
}

/* for GLists of strings only */
GList *
my_g_list_remove_duplicates (GList *list1, GList *list2)
{
	GList *new_list, *tmp_list;
	gboolean found;

	new_list = NULL;
	
	while (list1)
	{
		found = FALSE;
		tmp_list = list2;
		while (tmp_list)
		{
			if (!strcmp (list1->data, tmp_list->data))
				found = TRUE;

			tmp_list = tmp_list->next;
		}
		
		if (!found)
			new_list = g_list_prepend (new_list, list1->data);

		list1 = list1->next;
	}

	return new_list;
}

gchar *
find_new_id (xmlNodePtr parent)
{
	gchar *field, *buf;
	gint id;
	gint min, max;
	gint ret = 0;
	xmlNodePtr n0;
	Profile *pf;

	g_return_val_if_fail (parent != NULL, NULL);
	
	pf = profile_table_get_profile (NULL);
	if (!strcmp (parent->name, "userdb"))
	{
		field = g_strdup ("uid");
		min = pf->umin;
		max = pf->umax;
	}

	else if (!strcmp (parent->name, "groupdb"))
	{
		field = g_strdup ("gid");
		min = pf->gmin;
		max = pf->gmax;
	}

	else
		return NULL;

	for (n0 = parent->childs; n0; n0 = n0->next)
	{
		buf = xst_xml_get_child_content (n0, field);

		if (!buf)
			continue;

		id = atoi (buf);
		g_free (buf);

		if (ret < id)
			ret = id;
	}
	g_free (field);
	ret++;

	if (ret >= min && ret <= max)
		return g_strdup_printf ("%d", ret);

	return NULL;
}

gchar *
find_new_key (xmlNodePtr parent)
{
	/* TODO: Possibily mix together find_new_id and find_new_key. */
	gchar *buf;
	gint id;
	gint ret = 0;
	xmlNodePtr n0;

	g_return_val_if_fail (parent != NULL, NULL);

	for (n0 = parent->childs; n0; n0 = n0->next)
	{
		buf = xst_xml_get_child_content (n0, "key");

		if (!buf)
			continue;

		id = atoi (buf);
		g_free (buf);

		if (ret < id)
			ret = id;
	}

	return g_strdup_printf ("%d", ++ret);
}

/* Extern functions */

/* User related */

extern void
settings_prepare (ug_data *ud)
{
	gchar *buf;
	
	g_return_if_fail (ud != NULL);

	buf = xst_xml_get_child_content (ud->node, "login");

	if (buf)
	{
		/* Has to be some kind of user */
		g_free (buf);

		/* old_user
		user_settings_prepare (ud);
		*/
		user_settings_prepare (ud->node);
		return;
	}

	buf = xst_xml_get_child_content (ud->node, "name");

	if (buf)
	{
		/* Has to be some kind of group */
		g_free (buf);

		group_settings_prepare (ud);
		return;
	}

	g_warning ("settings_prepare: shouldn't be here");
}

extern gboolean
user_update (UserSettings *us)
{
	gboolean ok = TRUE;
	gint group, i;
	gchar *buf;
	gchar *old_name, *new_name;

	new_name = gtk_entry_get_text (us->basic->name);
	old_name = xst_xml_get_child_content (us->node, "login");
	
	if (!check_user_login (us->node, new_name))
		ok = FALSE;

	buf = gtk_entry_get_text (us->basic->comment);
	if (!check_user_comment (us->node, buf))
		ok = FALSE;

	group = check_user_group (us);
	switch (group)
	{
		case 0:
			/* Group exsisted, everything is ok. */
			break;
		case 1:
			/* Make new group */
			if (ok)
				group_add_from_user (us);
			break;
		case -1:
		default:
			/* Error. */
			ok = FALSE;
			break;
	}

	i = gtk_spin_button_get_value_as_int (us->basic->uid);
	
	buf = g_strdup_printf ("%d", i);
	if (!check_user_uid (us->node, buf))
		ok = FALSE;

	g_free (buf);

	buf = gtk_entry_get_text (us->basic->home);
	if (!check_user_home (us->node, buf))
		ok = FALSE;

	buf = gtk_entry_get_text (us->basic->shell);
	if (!check_user_shell (us->node, buf))
		ok = FALSE;

	
	if (ok)
	{
		if (us->new)
		{
			/* Add new user, update table. */
			us->node = user_add_blank_xml (us->node);
			user_update_xml (us);
			current_table_new_row (us->node, us->table);

			/* Ask for password too */
			if (!check_passwd (us->node))
				user_password_change (us->node);

			return ok;
		}

		else
		{
			/* Entered data ok, not new: just update. */
			user_update_xml (us);
			group_update_users (us->node, old_name, new_name);
			current_table_update_row (us->table);

			g_free (new_name);
			
			return ok;
		}
	}

	return FALSE;
}

extern void
user_passwd_dialog_prepare (xmlNodePtr node)
{
	GtkWidget *w0;
	gchar *txt, *name;

	name = xst_xml_get_child_content (node, "login");

	w0 = xst_dialog_get_widget (tool->main_dialog, "user_passwd_dialog");
	txt = g_strdup_printf (_("Password for User %s"), name);
	g_free (name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (txt);
	gtk_widget_show (w0);
	gtk_object_set_data (GTK_OBJECT (w0), "name", node);

#ifndef HAVE_LIBCRACK
	gtk_widget_hide (xst_dialog_get_widget (tool->main_dialog, "user_passwd_quality"));
#endif
}

extern gboolean
check_login_delete (xmlNodePtr node)
{
	gchar *name, *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	name = xst_xml_get_child_content (node, "login");

	if (!strcmp (name, "root"))
	{
		g_free (name);
		txt = g_strdup (_("The root user must not be deleted."));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
		g_free (txt);
		return FALSE;
	}

	txt = g_strdup_printf (_("Are you sure you want to delete user %s?"), name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (name);

	if (reply)
		return FALSE;
        else
		return TRUE;
}

/* Group related externs. */

extern void
group_new_prepare (ug_data *ud)
{
	GtkWidget *w0;

	/* Fill all users list, don't exclude anything */
	group_fill_all_users_list (ud->node, NULL);

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	gtk_window_set_title (GTK_WINDOW (w0), _("Create New Group"));
	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);
	gtk_widget_show (w0);
}

extern gboolean
group_update (ug_data *ud)
{
	gboolean ok = TRUE;
	gboolean adv;
	gchar *buf;

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);

	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
								    "group_settings_name")));
	if (!check_group_name (ud->node, buf))
		ok = FALSE;

	if (ok)
	{
		if (ud->new)
		{
			/* Add new group, update table. */
			ud->node = group_add_blank_xml (ud->node);
			group_update_xml (ud->node, adv);
			current_table_new_row (ud->node, ud->table);

			return ok;
		}

		else
		{
			/* Entered data ok, not new: just update */
			group_update_xml (ud->node, adv);
			current_table_update_row (ud->table);
			return ok;
		}
	}

	return FALSE;
}

extern gboolean
check_group_delete (xmlNodePtr node)
{
	gchar *name, *txt;
	GtkWindow *parent;
	GnomeDialog *dialog;

	g_return_val_if_fail (node != NULL, FALSE);

	parent = GTK_WINDOW (tool->main_dialog);
	name = xst_xml_get_child_content (node, "name");

	if (!name)
	{
		g_warning ("check_group_delete: Can't get group's name");
		return FALSE;
	}

	if (!strcmp (name, "root"))
	{
		g_free (name);
		txt = g_strdup (_("The root group must not be deleted."));
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (txt, parent));
		gnome_dialog_run (dialog);
		g_free (txt);
		return FALSE;
	}

	txt = g_strdup_printf (_("Are you sure you want to delete group %s?"), name);
	dialog = GNOME_DIALOG (gnome_question_dialog_parented (txt, reply_cb, NULL, parent));
	gnome_dialog_run (dialog);
	g_free (txt);
	g_free (name);

	if (reply)
		return FALSE;
        else
		return TRUE;
}


/* Static functions */

static void
group_settings_prepare (ug_data *ud)
{
	GtkWidget *w0;
	GList *member_rows;
	gchar *txt, *name;

	g_return_if_fail (ud != NULL);
	g_return_if_fail (name = xst_xml_get_child_content (ud->node, "name"));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_widget_set_sensitive (w0, xst_tool_get_access (tool));
	my_gtk_entry_set_text (w0, name);

	/* Fill group members */
	member_rows = group_fill_members_list (ud->node);

	/* Fill all users list */
	group_fill_all_users_list (ud->node, member_rows);

	while (member_rows)
	{
		g_free (member_rows->data);
		member_rows = member_rows->next;
	}
	g_list_free (member_rows);

	/* Show group settings dialog */

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	txt = g_strdup_printf (_("Settings for Group %s"), name);
	gtk_window_set_title (GTK_WINDOW (w0), txt);
	g_free (name);
	g_free (txt);

	gtk_object_set_data (GTK_OBJECT (w0), "data", ud);
	gtk_widget_show (w0);
}

static void
user_update_xml (UserSettings *us)
{
	gchar *buf;
	gint i, row;
	xmlNodePtr group_node;
	gboolean adv;
	
	/* Login */
	buf = gtk_entry_get_text  (us->basic->name);
	xst_xml_set_child_content (us->node, "login", buf);

	/* Comment */
	buf = gtk_entry_get_text  (us->basic->comment);
	xst_xml_set_child_content (us->node, "comment", buf);

	/* Main group */
	buf = parse_group (us);

	group_node = get_corresp_field (get_db_node (us->node));
	group_node = get_node_by_data (group_node, "name", buf);
	buf = xst_xml_get_child_content (group_node, "gid");
	xst_xml_set_child_content (us->node, "gid", buf);
	
	/* Secondary groups */
	del_user_groups (us->node);

	row = 0;
	while (gtk_clist_get_text (us->group->member, row++, 0, &buf))
		add_user_groups (us->node, buf);

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);
	
	/* Home */
	buf = parse_home (us);
	xst_xml_set_child_content (us->node, "home", buf);
	g_free (buf);
	
	/* Shell */
	xst_xml_set_child_content (us->node, "shell", gtk_entry_get_text (us->basic->shell));

	/* UID */
	if (adv)
	{
		i = gtk_spin_button_get_value_as_int (us->basic->uid);
		xst_xml_set_child_content (us->node, "uid", g_strdup_printf ("%d", i));
	}

	/* Passwd stuff */
	i = gtk_spin_button_get_value_as_int (us->pwd->min);
	xst_xml_set_child_content (us->node, "passwd_min_life", g_strdup_printf ("%d", i));

	i = gtk_spin_button_get_value_as_int (us->pwd->max);
	xst_xml_set_child_content (us->node, "passwd_max_life", g_strdup_printf ("%d", i));

	i = gtk_spin_button_get_value_as_int (us->pwd->days);
	xst_xml_set_child_content (us->node, "passwd_exp_warn", g_strdup_printf ("%d", i));
}

static xmlNodePtr
user_add_blank_xml (xmlNodePtr parent)
{
	xmlNodePtr user;
	Profile *pf;

	g_return_val_if_fail (parent != NULL, NULL);

	user = xst_xml_element_add (parent, "user");
	pf = profile_table_get_profile (NULL);

	xst_xml_element_add_with_content (user, "key", find_new_key (parent));
	xst_xml_element_add (user, "login");
	xst_xml_element_add (user, "password");
	xst_xml_element_add_with_content (user, "uid", find_new_id (parent));
	xst_xml_element_add (user, "gid");
	xst_xml_element_add (user, "comment");
	xst_xml_element_add (user, "home");
	xst_xml_element_add (user, "shell");
	xst_xml_element_add (user, "last_mod");

	xst_xml_element_add_with_content (user, "passwd_min_life",
			g_strdup_printf ("%d", pf->pwd_mindays));

	xst_xml_element_add_with_content (user, "passwd_max_life",
			g_strdup_printf ("%d", pf->pwd_maxdays));

	xst_xml_element_add_with_content (user, "passwd_exp_warn",
			g_strdup_printf ("%d", pf->pwd_warndays));

	xst_xml_element_add (user, "passwd_exp_disable");
	xst_xml_element_add (user, "passwd_disable");
	xst_xml_element_add (user, "reserved");
	xst_xml_element_add_with_content (user, "is_shadow", g_strdup ("1"));

	return user;
}

static xmlNodePtr
group_add_blank_xml (xmlNodePtr db_node)
{
	xmlNodePtr group;

	g_return_val_if_fail (db_node != NULL, NULL);

	group = xst_xml_element_add (db_node, "group");

	xst_xml_element_add_with_content (group, "key", find_new_key (db_node));
	xst_xml_element_add (group, "name");
	xst_xml_element_add (group, "password");
	xst_xml_element_add_with_content (group, "gid", find_new_id (db_node));
	xst_xml_element_add (group, "users");

	return group;
}

static void
group_add_from_user (UserSettings *us)
{
	xmlNodePtr node;
	gchar *buf;
	ug_data *group_data;

	node = get_corresp_field (us->node);
	group_data = g_new (ug_data, 1);
	group_data->node = group_add_blank_xml (node);

	buf = parse_group (us);
	xst_xml_set_child_content (group_data->node, "name", buf);

	group_data->table = TABLE_GROUP;

#ifdef NIS
	/* Add it to e-table. */
	if (us->table == TABLE_USER)
		group_data->table = TABLE_GROUP;
	else if (ud->table == TABLE_NET_USER)
		group_data->table = TABLE_NET_GROUP;
	else
	{
		g_warning ("group_add_from_user: shouldn't be here");
		g_free (group_data);
		return;
	}
#endif	
	current_table_new_row (group_data->node, group_data->table);
	g_free (group_data);
}

static void
group_update_xml (xmlNodePtr node, gboolean adv)
{
	GtkCList *clist;
	gchar *buf;
	gint row = 0;

	/* Name */
	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
								    "group_settings_name")));
	xst_xml_set_child_content (node, "name", buf);

	/* Users */
	del_group_users (node);
	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));

	while (gtk_clist_get_text (clist, row++, 0, &buf))
		add_group_users (node, buf);
}

static void
group_update_users (xmlNodePtr node, gchar *old_name, gchar *new_name)
{
	xmlNodePtr dbnode, gnode;
	gchar *buf;

	g_return_if_fail (node != NULL);
	g_return_if_fail (old_name != NULL);
	g_return_if_fail (new_name != NULL);

	if (!strcmp (old_name, new_name))
		return;
	
	dbnode = get_db_node (node);
	dbnode = get_corresp_field (dbnode);

	for (dbnode = xst_xml_element_find_first (dbnode, "group");
		dbnode;
		dbnode = dbnode->next)
	{
		gnode = xst_xml_element_find_first (dbnode, "users");

		if (!gnode)
			continue;

		for (gnode = gnode->childs; gnode; gnode = gnode->next)
		{
			buf = xst_xml_element_get_content (gnode);

			if (!buf)
				continue;

			if (!strcmp (buf, old_name))
				xst_xml_element_set_content (gnode, new_name);

			g_free (buf);
		}
	}
}

static gboolean
node_exsists (xmlNodePtr node, gchar *name, gchar *val)
{
	xmlNodePtr n0;
	gchar *buf;
	xmlNodePtr parent;
	gboolean self;

	parent = get_db_node (node);
	if (parent == node)
		self = FALSE;
	else
		self = TRUE;

	for (n0 = parent->childs; n0; n0 = n0->next)
	{
		if (self && n0 == node)
			continue;  /* Self */

		buf = xst_xml_get_child_content (n0, name);

		if (!buf)
			continue;  /* No content */

		if (!strcmp (val, buf))
		{
			g_free (buf);  /* Woohoo! found! */
			return TRUE;
		}

		g_free (buf);
	}

	return FALSE;
}

static GList *
get_group_users (xmlNodePtr group_node)
{
	GList *userlist = NULL;
	xmlNodePtr node, u;

	g_return_val_if_fail (group_node != NULL, NULL);

	node = xst_xml_element_find_first (group_node, "users");
	if (!node)
	{
		g_warning ("get_group_users: can't get current group's users node.");
		return NULL;
	}

	for (u = xst_xml_element_find_first (node, "user"); u; u = xst_xml_element_find_next (u, "user"))
		userlist = g_list_prepend (userlist, xst_xml_element_get_content (u));

	return userlist;
}

static GList *
get_group_mainusers (xmlNodePtr group_node)
{
	xmlNodePtr user_node, node;
	gchar *gid, *buf;
	GList *userlist = NULL;
	
	g_return_val_if_fail (group_node != NULL, NULL);

	user_node = get_corresp_field (group_node);
	gid = xst_xml_get_child_content (group_node, "gid");
	
	for (node = xst_xml_element_find_first (user_node, "user");
	     node;
	     node = xst_xml_element_find_next (node, "user"))
	{

		buf = xst_xml_get_child_content (node, "gid");
		if (!buf)
			continue;

		if (!strcmp (buf, gid))
			userlist = g_list_prepend (userlist,
						   xst_xml_get_child_content (node, "login"));

		g_free (buf);
	}

	g_free (gid);
	return userlist;
}

static GList *
group_fill_members_list (xmlNodePtr node)
{
	GList *items;
	GtkCList *clist;

	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));
	items = get_group_users (node);
	
	gtk_clist_set_auto_sort (clist, TRUE);
	my_gtk_clist_append_items (clist, items);
	return items;
}

static void
group_fill_all_users_list (xmlNodePtr node, GList *exclude)
{
	GList *users, *items;
	GtkCList *clist;

	clist = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_all"));
	users = get_user_list ("login", node);
	items = my_g_list_remove_duplicates (users, exclude);
	
	gtk_clist_set_auto_sort (clist, TRUE);
	my_gtk_clist_append_items (clist, items);
}

static void
del_group_users (xmlNodePtr group_node)
{
	xmlNodePtr node;
	
	g_return_if_fail (group_node != NULL);

	node = xst_xml_element_find_first (group_node, "users");
	if (!node)
		return;

	xst_xml_element_destroy_children (node);
}

static void
add_group_users (xmlNodePtr group_node, gchar *name)
{
	xmlNodePtr user;

	g_return_if_fail (group_node != NULL);

	user = xst_xml_element_find_first (group_node, "users");
	if (!user)
		user = xst_xml_element_add (group_node, "users");

	user = xst_xml_element_add (user, "user");
	xst_xml_element_set_content (user, name);
}

static void
del_user_groups (xmlNodePtr user_node)
{
	xmlNodePtr group_node, g, group_users, tmp_node;
	gchar *user_name, *buf;
	gboolean found;

	g_return_if_fail (user_node != NULL);

	group_node = get_corresp_field (user_node);
	user_name = xst_xml_get_child_content (user_node, "login");

	for (g = xst_xml_element_find_first (group_node, "group");
	     g;
	     g = xst_xml_element_find_next (g, "group"))
	{
		
		group_users = xst_xml_element_find_first (g, "users");

		group_users = xst_xml_element_find_first (group_users, "user");
		while (group_users)
		{
			found = FALSE;
			buf = xst_xml_element_get_content (group_users);
			if (buf)
			{
				if (!strcmp (user_name, buf))
					found = TRUE;
				
				g_free (buf);
			}

			tmp_node = group_users;
			group_users = xst_xml_element_find_next (group_users, "user");
			if (found)
				xst_xml_element_destroy (tmp_node);
		}
	}

	g_free (user_name);
}

static void
add_user_groups (xmlNodePtr user_node, gchar *group_name)
{
	xmlNodePtr group_node, g, group_users;
	gchar *user_name, *buf;

	g_return_if_fail (user_node != NULL);

	group_node = get_corresp_field (user_node);
	user_name = xst_xml_get_child_content (user_node, "login");

	for (g = xst_xml_element_find_first (group_node, "group");
	     g;
	     g = xst_xml_element_find_next (g, "group"))
	{

		buf = xst_xml_get_child_content (g, "name");
		if (strcmp (buf, group_name))
		{
			g_free (buf);
			continue;
		}

		g_free (buf);
		group_users = xst_xml_element_find_first (g, "users");
		if (!group_users)
			group_users = xst_xml_element_add (g, "users");

		group_users = xst_xml_element_add (group_users, "user");
		xst_xml_element_set_content (group_users, user_name);
		break;
	}

	g_free (user_name);
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

static void
reply_cb (gint val, gpointer data)
{
        reply = val;
        gtk_main_quit ();
}

void
my_gtk_clist_append_items (GtkCList *list, GList *items)
{
	gchar *entry[2];

	g_return_if_fail (list != NULL);
	g_return_if_fail (GTK_IS_CLIST (list));

	entry[1] = NULL;

	gtk_clist_freeze (list);
	while (items)
	{
		entry[0] = items->data;
		items = items->next;

		gtk_clist_append (list, entry);
	}
	gtk_clist_thaw (list);
}

gint
my_gtk_clist_append (GtkCList *list, gchar *text)
{
	gchar *entry[2];
	
	g_return_val_if_fail (list != NULL, -1);
	g_return_val_if_fail (GTK_IS_CLIST (list), -1);

	entry[0] = text;
	entry[1] = NULL;

	return gtk_clist_append (list, entry);
}


static gchar *user_search_string;

void
user_query_string_set (gchar *str)
{
	if (user_search_string)
		g_free (user_search_string);

	user_search_string = g_strdup (str);
}

gchar *
user_query_string_get (void)
{
	if (!user_search_string)
		user_search_string = g_strdup ("all");
	
	return g_strdup (user_search_string);
}

static gchar *
parse_home (UserSettings *us)
{
	const guint max_tokens = 8;
	gchar *home;
	gchar **buf;
	gint i;

	buf = g_strsplit (gtk_entry_get_text (us->basic->home), "/", max_tokens);

	i = 0;
	while (buf[i])
	{
		if (!strcmp (buf[i], "$user"))
		{
			g_free (buf[i]);
			buf[i] = g_strdup (gtk_entry_get_text (us->basic->name));
		}
		i++;
	}
	
	home = g_strjoinv ("/", buf);
	
	g_strfreev (buf);
	
	return home;
}

static gchar *
parse_group (UserSettings *us)
{
	gchar *buf;

	buf = gtk_editable_get_chars (GTK_EDITABLE (us->group->main->entry), 0, -1);

	if (!strcmp (buf, "$user"))
	{
		g_free (buf);
		buf = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (us->basic->name), 0, -1));
	}

	return buf;
}
