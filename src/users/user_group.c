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
#include <ctype.h>
#include <gnome.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "callbacks.h"
#include "user_group.h"
#include "e-table.h"
#include "user_settings.h"
#include "profile.h"
#include "user-group-xml.h"
#include "passwd.h"

extern XstTool *tool;

/* Local globals */
static int reply;

/* Static prototypes */

static void group_settings_prepare (ug_data *ud);
static void group_update_xml (xmlNodePtr node, gboolean adv);
static GList *get_group_users (xmlNodePtr group_node);
static GList *group_fill_members_list (xmlNodePtr node);
static void group_fill_all_users_list (xmlNodePtr node, GList *exclude);
static void del_group_users (xmlNodePtr group_node);


static gboolean
node_exists (xmlNodePtr node, const gchar *name, const gchar *val)
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

		buf = xst_xml_get_child_content (n0, (gchar *)name);

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
	
	if (!strcmp (ar[0], "all")) {
		g_strfreev (ar);
		return TRUE;
	}

	if (!strcmp (ar[1], "group"))
		buf = user_value_group (node);
	else
		buf = xst_xml_get_child_content (node, ar[1]);

	
	if (buf && !strcmp (ar[0], "contains") && strstr (buf, ar[2]))
		ret = TRUE;

	else if (buf && !strcmp (ar[0], "is") && !strcmp (buf, ar[2]))
		ret = TRUE;

	g_free (buf);
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

static gboolean
is_valid_name (const gchar *str)
{
	if (!str || !*str)
		return FALSE;

	for (;*str; str++) {
		if (((*str < 'a') || (*str > 'z')) &&
				((*str < '0') || (*str > '9')))

			return FALSE;
	}

	return TRUE;
}

static gboolean
is_valid_id (const gchar *str)
{
	gdouble nr;
	gchar *buf;
	
	if (!str || !*str)
		return FALSE;

	buf = (gchar *)str;
	
	for (;*buf; buf++) {
		if (!isdigit (*buf))
			return FALSE;
	}

	nr = g_strtod (str, NULL);
	
	if (nr < 0 || nr > IDMAX)
		return FALSE;
	
	return TRUE;
}

gboolean
check_user_root (xmlNodePtr node, const gchar *field, const gchar *value)
{
	gint i;
	gboolean retval = TRUE;
	gchar *nvalue;
	gchar *n[] = { "login", "uid", "home", "gid", NULL };
	gchar *user;


	if ((user = user_value_login (node)) == NULL)
		return retval;

	if (strcmp (user, "root"))
		return retval;

	nvalue = generic_value_string (node, field);
	for (i = 0; n[i]; i++) {
		if (!strcmp (n[i], field) && strcmp (value, nvalue)) {
			retval = FALSE;
			break;
		}
	}

	g_free (user);
	g_free (nvalue);

	return retval;

}

gchar *
check_user_login (xmlNodePtr node, const gchar *login)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (login != NULL, FALSE);

	/* If !empty. */
	if (strlen (login) < 1)
		buf = g_strdup (_("The username is empty."));

	/* If too long. */
	else if (strlen (login) > 32)
		buf = g_strdup (_("The username is too long."));

	else if (!check_user_root (node, "login", login))
		buf = g_strdup (_("root user shouldn't be modified."));
	
	/* if valid. */
	else if (!is_valid_name (login))
		buf = g_strdup (_("Please set a valid username, using only lower-case letters."));

	/* if !exist. */
	else if (node_exists (node, "login", login))
		buf = g_strdup (_("Username already exists."));

	return buf;
}

gchar *
check_user_uid (xmlNodePtr node, const gchar *val)
{
	gchar *buf = NULL;

	if (!is_valid_id (val))
		buf = g_strdup (_("User id must be a positive number."));
	else if (!check_user_root (node, "uid", val))
		buf = g_strdup (_("root user shouldn't be modified."));
	else if (node_exists (node, "uid", val)) {
		buf = g_strdup (_("Such user id already exists."));
		g_warning ("Duplicate id %s", val);
	}
	return buf;
}

static gchar *
user_account_check_comment (UserAccount *account)
{
	gint i;
	gchar *buf = NULL;	
	gchar *comment;

	/* comment can be empty, no error */
	if (!account->comment)
		return buf;

	comment = g_strjoinv (NULL, account->comment);
	
	for (i = 0; i < strlen (comment); i++) {
		if (iscntrl (comment[i]) || comment[i] == ',' ||
		    comment[i] == '=' || comment[i] == ':') {

			buf = g_strdup_printf (N_("Invalid character '%c' in comment."),
						  comment[i]);
			break;
		}
	}

	g_free (comment);
	return buf;
}

gchar *
check_user_shell (xmlNodePtr node, const gchar *val)
{
	gchar *buf = NULL;

	if (strlen (val) > 0 && *val != '/')
		buf = g_strdup (_("Please give shell with full path."));

	return buf;
}

static gchar *
parse_home (UserAccount *account)
{
	const guint max_tokens = 8;
	gchar *home;
	gchar **buf;
	gint i;

	buf = g_strsplit (account->home, "/", max_tokens);

	for (i = 0; buf[i]; i++) {
		if (!strcmp (buf[i], "$user")) {
			g_free (buf[i]);
			buf[i] = g_strdup (account->name);
		}
	}
	
	home = g_strjoinv ("/", buf);
	
	g_strfreev (buf);
	
	return home;
}

static gchar *
user_account_check_home (UserAccount *account)
{
	gchar *home = account->home;
	xmlNodePtr node = account->node;
	struct stat s;
	
	if (!home || (strlen (home) < 1))
		return g_strdup (N_("Home directory must not be empty."));

	if (*home != '/')
		return g_strdup (N_("Please enter full path for home directory."));

	if (!check_user_root (node, "home", home))
		return g_strdup (N_("root user shouldn't be modified."));

	if (stat (home, &s))
	{
		switch (errno) {
		case ENOTDIR: return _("Part of the path to the home directory is a file.");         
		case ELOOP:   return _("There is a loop in the path to the home directory.");        
		case ENOMEM:  return _("Not enough memory to check the home directory.");            
		case ENAMETOOLONG: return _("The path to the home directory is too long.");          
		}
	}

	return NULL;
}

static gchar *
parse_group (UserAccount *account, const gchar *val)
{
	gchar *buf;
	
	if (!strcmp (val, "$user"))
		buf = g_strdup (account->name);
	else
		buf = g_strdup (val);

	return buf;
}

static gint
user_account_check_group (UserAccount *account, gchar **error)
{
	xmlNodePtr group_node;
	gchar *group;
	gint retval;
	gchar *group_name = account->group;	
	
	group = parse_group (account, group_name);
	group_node = get_corresp_field (account->node);

	if (!is_valid_name (group)) {
		*error = g_strdup (_("Group name is not valid."));
		retval = -1;
	} else if (node_exists (group_node, "group", group_name)) {
		*error = g_strdup (_("Such group id already exists."));
		retval = 0;
	} else {
		*error = g_strdup (_("Group does not exist. Create new?"));
		retval = 1;
	}

	g_free (group);
	
	return retval;
}

gboolean
check_group_name (GtkWindow *xd, xmlNodePtr node, const gchar *name)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);

	/* If !empty. */
	if (strlen (name) < 1)
		buf = g_strdup (_("Group name is empty."));

	/* If too long. */
	if (strlen (name) > 16)
		buf = g_strdup (_("The group name is too long."));

	/* if valid. */
	else if (!is_valid_name (name))
		buf = g_strdup (_("Please set a valid group name, using only lower-case letters."));

	/* if !exist. */
	else if (node_exists (node, "name", name))
		buf = g_strdup (_("Group already exists."));

	/* If anything is wrong. */
	if (buf)
	{
		GnomeDialog *dialog;

		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, xd));
		gnome_dialog_run (dialog);
		g_free (buf);
		gtk_widget_grab_focus (GTK_WIDGET (xd));

		return FALSE;
	}

	else
		return TRUE;
}

gboolean
check_group_gid (GtkWindow *xd, xmlNodePtr group_node, const gchar *val)
{
	gboolean retval = TRUE;
	GnomeDialog *dialog;
	gchar *buf = NULL;

	if (!is_valid_id (val))
		buf = g_strdup (_("User id must be a positive number."));
	
	/* Check if gid is available */
	else if (node_exists (group_node, "gid", val))
		buf = g_strdup (_("Such group id already exists."));
	
	if (buf) {
		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, xd));
		gnome_dialog_run (dialog);
		g_free (buf);
		retval = FALSE;
	}

	return retval;
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
get_node_by_data (xmlNodePtr dbnode, const gchar *field, const gchar *fdata)
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
	guint id;
	guint min, max;
	guint ret = 0;
	xmlNodePtr n0;
	Profile *pf;

	g_return_val_if_fail (parent != NULL, NULL);
	
	pf = profile_table_get_profile (NULL);
	if (!strcmp (parent->name, "userdb")) {
		field = g_strdup ("uid");
		min = pf->umin;
		max = pf->umax;
	} else if (!strcmp (parent->name, "groupdb")) {
		field = g_strdup ("gid");
		min = pf->gmin;
		max = pf->gmax;
	} else {
		g_warning ("find_new_id: Unknown data source: %s.", parent->name);
		return NULL;
	}

	ret = min - 1;
	for (n0 = parent->childs; n0; n0 = n0->next) {
		buf = xst_xml_get_child_content (n0, field);
		if (!buf)
			continue;

		id = atoi (buf);
		g_free (buf);

		if (id <= max && ret < id)
			ret = id;
	}
	g_free (field);
	ret++;

	if (ret >= min && ret <= max)
		return g_strdup_printf ("%d", ret);

	g_warning ("find_new_id: failed: %d >= %d && %d <= %d", ret, min, ret, max);
	return NULL;
}

gchar *
find_new_key (xmlNodePtr parent)
{
	/* TODO: Possibily mix together find_new_id and find_new_key. */
	gchar *buf;
	gint id;
	gint ret = -1;
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

/* User related */

void
settings_prepare (ug_data *ud)
{
	gchar *buf;
	
	g_return_if_fail (ud != NULL);

	buf = xst_xml_get_child_content (ud->node, "login");

	if (buf)
	{
		/* Has to be some kind of user */
		g_free (buf);
		g_warning ("settings_prepare: Deprecated, shouldn't be here");
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

void
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

static void
reply_cb (gint val, gpointer data)
{
        reply = val;
        gtk_main_quit ();
}

gboolean
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

/* Group related */

void
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

gboolean
group_update (ug_data *ud)
{
	gboolean ok = TRUE;
	gboolean adv;
	gchar *buf;
	GtkWidget *d;

	adv = (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED);
	d = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");

	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (tool->main_dialog,
								    "group_settings_name")));
	if (!check_group_name (GTK_WINDOW (d), ud->node, buf))
		ok = FALSE;

	if (ok) {
		if (ud->new) {
			/* Add new group, update table. */
			ud->node = group_add_blank_xml (ud->node);
			group_update_xml (ud->node, adv);

			return ok;
		} else {
			/* Entered data ok, not new: just update */
			group_update_xml (ud->node, adv);
			current_table_update_row (ud->table);
			return ok;
		}
	}

	return FALSE;
}

gboolean
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
	xst_ui_entry_set_text (w0, name);

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

#if 0
/* Not used at the moment, if'ed out to get rid of complier warning. */
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
#endif

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

#if 0
/* Not used at the moment, if'ed out to get rid of complier warning. */
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
#endif

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





UserAccount *
user_account_new (const gchar *profile)
{
	UserAccount *account;
	xmlNodePtr node;
	gchar *uid;
	Profile *pf = profile_table_get_profile (profile);

	if (!pf) {
		g_warning ("user_account_new: Couldn't get profile %s. Using default.",
			   profile);
		pf = profile_table_get_profile (NULL);
	}

	node = get_user_root_node ();
	if (!node) {
		g_warning ("user_account_new: Couldn't get node for user.");
		return NULL;
	}

	uid = find_new_id (node);
	if (!uid) {
		g_warning ("user_account_new: Couldn't get uid for user.");
		return NULL;
	}
	
	account = g_new0 (UserAccount, 1);

	account->node = node;
	account->new = TRUE;

	account->uid = uid;
	account->group = g_strdup (pf->group);
	account->shell = g_strdup (pf->shell);
	account->home = g_strdup (pf->home_prefix);
	
	account->pwd_maxdays = pf->pwd_maxdays;
	account->pwd_mindays = pf->pwd_mindays;
	account->pwd_warndays = pf->pwd_warndays;
	
	return account;
}

UserAccount *
user_account_get_by_node (xmlNodePtr node)
{
	UserAccount *account;

	account = g_new0 (UserAccount, 1);

	account->node = node;
	account->new = FALSE;

	account->name = user_value_login (node);
	account->comment = user_value_comment_array (node);
	account->uid = user_value_uid_string (node);
	account->home = user_value_home (node);
	account->shell = user_value_shell (node);
	account->password = user_value_password (node);
	account->group = user_value_group (node);
		
	account->pwd_maxdays = user_value_pwd_maxdays (node);
	account->pwd_mindays = user_value_pwd_mindays (node);
	account->pwd_warndays = user_value_pwd_warndays (node);

	account->extra_groups = user_get_groups (node);
	
	return account;
}

void
user_account_save (UserAccount *account)
{
	gchar *buf;
	xmlNodePtr node = account->node;

	if (account->new)
		node = account->node = user_add_blank_xml (account->node);
	
	user_set_value_login (node, account->name);
	user_set_value_comment_array (node, account->comment);
	user_set_value_uid_string (node, account->uid);
	user_set_value_shell (node, account->shell);

	buf = parse_home (account);
	user_set_value_home (node, buf);
	g_free (buf);

	buf = parse_group (account, account->group);
	user_set_value_group (node, buf);
	g_free (buf);

	user_set_value_pwd_maxdays (node, account->pwd_maxdays);
	user_set_value_pwd_mindays (node, account->pwd_mindays);
	user_set_value_pwd_warndays (node, account->pwd_warndays);

	user_set_groups (node, account->extra_groups);

	if (account->password)
		passwd_set (node, account->password);

	current_table_update_row (TABLE_USER);
	xst_dialog_modify (tool->main_dialog);
}

gchar *
user_account_check (UserAccount *account)
{
	gchar *buf;
	xmlNodePtr node = account->node;	

	if ((buf = check_user_login (node, account->name)))
		return buf;

	if ((buf = user_account_check_comment (account)))
		return buf;
	
	if ((buf = user_account_check_home (account)))
		return buf;

	if ((buf = check_user_shell (node, account->shell)))
		return buf;

	if ((buf = check_user_uid (node, account->uid)))
		return buf;

	if ((user_account_check_group (account, &buf)) < 0)
		return buf;
			
	return NULL;
}

static GList *
user_account_check_home_warnings (UserAccount *account, GList *warnings)
{
	gchar *home = account->home;
	struct stat s;

	if (stat (home, &s))
	{
		gchar *text = NULL;
		
		switch (errno) {
		case ENOENT:
			if (!account->new)
				text = _("The home directory does not exist, or its path is invalid.");
			break;
		case EACCES: text = _("Couldn't get access to the home directory."); break;
		default:
			if (!account->new)
				/* We shouldn't fall here: the other cases are in the error checks. */
				text = _("There was an error trying to access the home directory.");
		}

		if (text)
			warnings = g_list_append (warnings, text);
	} else {
		if (!S_ISDIR (s.st_mode))
			warnings = g_list_append
				(warnings, _("The home directory path exists, but it is not a directory."));
		if (!account->new) {
			if (account->uid && (s.st_uid != atoi (account->uid)))
				warnings = g_list_append
					(warnings, _("The home directory is not owned by the user."));
			else {
				if (!s.st_mode & S_IRUSR)
					warnings = g_list_append
						(warnings, _("The user doesn't have permission to read from the home directory."));
				if (!s.st_mode & S_IWUSR)
					warnings = g_list_append
						(warnings, _("The user doesn't have permission to write in the home directory."));
				if (!s.st_mode & S_IRUSR)
					warnings = g_list_append
						(warnings, _("The user doesn't have permission to use (``execute'') the home directory."));
			}
		}
	}
		
	return warnings;
}

GList *
user_account_check_warnings (UserAccount *account)
{
	GList *warnings = NULL;

	warnings = user_account_check_home_warnings (account, warnings);

	return warnings;
}

void
user_account_destroy_warnings (GList *warnings)
{
	GList *l, *l2;

	for (l2 = l = warnings; l2; l = l2) {
		l2 = l->next;
		g_list_remove_link (warnings, l);
	}
}

void
user_account_destroy (UserAccount *account)
{
	if (account->name)     g_free (account->name);
	if (account->comment)  g_strfreev (account->comment);
	if (account->home)     g_free (account->home);
	if (account->shell)    g_free (account->shell);
	if (account->uid)      g_free (account->uid);
	if (account->group)    g_free (account->group);
	if (account->password) g_free (account->password);

	if (account->extra_groups) g_slist_free (account->extra_groups);
	
	g_free (account);
}
