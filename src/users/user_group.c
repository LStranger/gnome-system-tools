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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Tambet Ingo <tambet@ximian.com> and 
 *          Arturo Espinosa <arturo@ximian.com>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <gnome.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <utmp.h>

#include "callbacks.h"
#include "user_group.h"
#include "table.h"
#include "user-settings.h"
#include "user-group-xml.h"
#include "passwd.h"

extern XstTool *tool;

/* Local globals */
/* Static prototypes */

/*static void group_settings_prepare (ug_data *ud);
  static GList *get_group_users (xmlNodePtr group_node);*/
static GList *group_fill_members_list (xmlNodePtr node);
static void group_fill_all_users_list (xmlNodePtr node, GList *exclude);

/* Global functions */

void
show_error_message (gchar *parent_window, gchar *message)
{
	GtkWindow *xd = GTK_WINDOW (xst_dialog_get_widget (tool->main_dialog, parent_window));
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (xd, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, message);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

xmlNodePtr
get_root_node (gint tbl)
{
	xmlNodePtr node, root;

	root = xst_xml_doc_get_root (tool->config);

	switch (tbl)
	{
	case NODE_USER:
		node = xst_xml_element_find_first (root, "userdb");
		break;
	case NODE_GROUP:
		node = xst_xml_element_find_first (root, "groupdb");
		break;
/*	case NODE_NET_USER:
		node = xst_xml_element_find_first (root, "nis_userdb");
		break;
	case NODE_NET_GROUP:
		node = xst_xml_element_find_first (root, "nis_groupdb");
		break;*/
	case NODE_PROFILE:
		node = xst_xml_element_find_first (root, "profiledb");
		break;
	default:
		node = NULL;
		break;
	}

	return node;
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
check_node_visibility (xmlNodePtr node)
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
is_valid_name (const gchar *name)
{
	/*
	 * User/group names must start with a letter, and may not
	 * contain colons, commas, newlines (used in passwd/group
	 * files...) or any non-printable characters.
	 */
        if (!*name || !isalpha(*name))
                return FALSE;

        while (*name) {
		if (!isdigit (*name) && !islower (*name) && *name != '-')
                        return FALSE;
                name++;
        }

        return TRUE;
}


gboolean
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
get_min_max (xmlNodePtr db_node, gint *min, gint *max)
{
	xmlNodePtr profiledb = xst_xml_element_find_first (xst_xml_doc_get_root (tool->config), "profiledb");
	xmlNodePtr profile = xst_xml_element_find_first (profiledb, "profile");
	gint umin = g_strtod ((gchar *) xst_xml_get_child_content (profile, "umin"), NULL);
	gint umax = g_strtod ((gchar *) xst_xml_get_child_content (profile, "umax"), NULL);
	gint gmin = g_strtod ((gchar *) xst_xml_get_child_content (profile, "gmin"), NULL);
	gint gmax = g_strtod ((gchar *) xst_xml_get_child_content (profile, "gmax"), NULL);
	
	g_return_val_if_fail (db_node != NULL, FALSE);

	if (!strcmp (db_node->name, "userdb"))
	{
		*min = umin;
		*max = umax;

		return TRUE;
	}

	if (!strcmp (db_node->name, "groupdb"))
	{
		*min = gmin;
		*max = gmax;

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
	gchar *buf, *key;

	g_return_val_if_fail (dbnode != NULL, NULL);
	g_return_val_if_fail (field != NULL, NULL);

	if (!fdata)
		return NULL;
	
	if (strcmp (dbnode->name, "userdb") == 0)
		key = g_strdup ("user");
	else if (strcmp (dbnode->name, "groupdb") == 0)
		key = g_strdup ("group");
	
	for (node = xst_xml_element_find_first (dbnode, key); node != NULL; node = xst_xml_element_find_next (node, key))
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
			new_list = g_list_append (new_list, list1->data);

		list1 = list1->next;
	}

	return new_list;
}

gchar *
find_new_id (xmlNodePtr parent)
{
	xmlNodePtr profiledb = xst_xml_element_find_first (xst_xml_doc_get_root (tool->config), "profiledb");
	xmlNodePtr profile = xst_xml_element_find_first (profiledb, "profile");
	gint umin = g_strtod ((gchar *) xst_xml_get_child_content (profile, "umin"), NULL);
	gint umax = g_strtod ((gchar *) xst_xml_get_child_content (profile, "umax"), NULL);
	gint gmin = g_strtod ((gchar *) xst_xml_get_child_content (profile, "gmin"), NULL);
	gint gmax = g_strtod ((gchar *) xst_xml_get_child_content (profile, "gmax"), NULL);

	gchar *field, *buf, *key;
	guint id;
	guint min, max;
	guint ret = 0;
	xmlNodePtr n0;

	g_return_val_if_fail (parent != NULL, NULL);
	
	if (!strcmp (parent->name, "userdb")) {
		key = g_strdup ("user");
		field = g_strdup ("uid");
		min = umin;
		max = umax;
	} else if (!strcmp (parent->name, "groupdb")) {
		key = g_strdup ("group");
		field = g_strdup ("gid");
		min = gmin;
		max = gmax;
	} else {
		g_warning ("find_new_id: Unknown data source: %s.", parent->name);
		return NULL;
	}

	ret = min - 1;
	for (n0 = xst_xml_element_find_first (parent, key); n0 != NULL; n0 = xst_xml_element_find_next (n0, key)) {
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
	gchar *buf, *key;
	gint id;
	gint ret = -1;
	xmlNodePtr n0;

	g_return_val_if_fail (parent != NULL, NULL);
	
	if (strcmp (parent->name, "userdb") == 0)
		key = g_strdup ("user");
	else if (strcmp (parent->name, "groupdb") == 0)
		key = g_strdup ("group");
	
	for (n0 = xst_xml_element_find_first (parent, key); n0 != NULL; n0 = xst_xml_element_find_next (n0, key))
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

gint my_strcmp (gconstpointer a, gconstpointer b)
{
	return strcmp ((const char *) a, (const char *) b);
}
