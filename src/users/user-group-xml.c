/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-group-xml.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es> and
 *          Tambet Ingo <tambet@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gst.h"
#include "user-group-xml.h"

#include "user_group.h"
#include "table.h"

extern GstTool *tool;

void generic_set_value (xmlNodePtr node, const gchar *name, const gchar *value);

gchar *
generic_value_string (xmlNodePtr node, const gchar *name)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return gst_xml_get_child_content (node, (gchar *)name);
}

gint
generic_value_integer (xmlNodePtr node, const gchar *name)
{
	gchar *buf;
	gint val;
	
	g_return_val_if_fail (node != NULL, -1);
	g_return_val_if_fail (name != NULL, -1);

	buf = gst_xml_get_child_content (node, (gchar *)name);
	if (buf) {
		val = atoi (buf);
		g_free (buf);
	}
	else
		val = -1;

	return val;
}

gpointer
generic_value_string_peek (xmlNodePtr parent, const gchar *name)
{
	xmlNodePtr node;
	
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	node = gst_xml_element_find_first (parent, name);
	if (node)
		return (gpointer) gst_xml_element_peek_content (node);

	return NULL;
}

gchar *
user_value_group (xmlNodePtr user_node)
{
	gchar *gid, *buf;
	xmlNodePtr group_node;

	gid = user_value_gid_string (user_node);
	group_node = get_corresp_field (get_db_node (user_node));
	group_node = get_node_by_data (group_node, "gid", gid);
	if (!group_node)
		return gid;
	
	buf = group_value_name (group_node);
	if (!buf)
		return gid;
	
	g_free (gid);
	return buf;
}

gpointer
user_value_group_peek (xmlNodePtr user_node)
{
	gchar *gid, *buf;
	xmlNodePtr group_node;

	gid = user_value_gid_string_peek (user_node);
	group_node = get_corresp_field (get_db_node (user_node));
	group_node = get_node_by_data (group_node, "gid", gid);
	if (!group_node)
		return gid;
	
	buf = group_value_name_peek (group_node);
	if (!buf)
		return gid;
	return buf;
}

GList*
user_get_groups (xmlNodePtr user_node)
{
	xmlNodePtr group_node, g, group_users;
	gchar *user_name, *buf;
	GList *grouplist = NULL;

	g_return_val_if_fail (user_node != NULL, NULL);

	group_node = get_corresp_field (user_node);
	user_name = user_value_login (user_node);

	if (!user_name)
		return NULL;

	for (g = gst_xml_element_find_first (group_node, "group"); g;
	     g = gst_xml_element_find_next (g, "group")) {
		group_users = gst_xml_element_find_first (g, "users");
		for (group_users = gst_xml_element_find_first (group_users, "user");
		     group_users;
		     group_users = gst_xml_element_find_next (group_users, "user")) {
			buf = gst_xml_element_get_content (group_users);
			if (!buf)
				continue;

			if (!strcmp (user_name, buf))
				grouplist = g_list_insert_sorted (grouplist, group_value_name (g), my_strcmp);
			
			g_free (buf);
		}
	}

	g_free (user_name);
	return grouplist;
}

void
del_user_groups (xmlNodePtr user_node)
{
	xmlNodePtr group_node, g, group_users, tmp_node;
	gchar *user_name, *buf;
	gboolean found;
	
	g_return_if_fail (user_node != NULL);

	group_node = get_corresp_field (user_node);
	user_name = gst_xml_get_child_content (user_node, "login");

	for (g = gst_xml_element_find_first (group_node, "group");
	     g;
	     g = gst_xml_element_find_next (g, "group")) {
		group_users = gst_xml_element_find_first (g, "users");

		group_users = gst_xml_element_find_first (group_users, "user");
		while (group_users) {
			found = FALSE;
			buf = gst_xml_element_get_content (group_users);
			if (buf) {
				if (!strcmp (user_name, buf))
					found = TRUE;
				
				g_free (buf);
			}

			tmp_node = group_users;
			group_users = gst_xml_element_find_next (group_users, "user");
			if (found)
				gst_xml_element_destroy (tmp_node);
		}
	}

	g_free (user_name);
}

void
user_set_groups (xmlNodePtr user_node, GSList *list)
{
	xmlNodePtr group_node, g, group_users;
	gchar *user_name, *buf;
	GSList *tmp;

	g_return_if_fail (user_node != NULL);

	del_user_groups (user_node);
	
	group_node = get_corresp_field (user_node);
	user_name = gst_xml_get_child_content (user_node, "login");

	for (g = gst_xml_element_find_first (group_node, "group"); g;
	     g = gst_xml_element_find_next (g, "group")) {
		buf = gst_xml_get_child_content (g, "name");

		tmp = list;
		while (tmp) {
			gchar *group_name = tmp->data;
			tmp = tmp->next;

			if (!strcmp (buf, group_name)) {
				group_users = gst_xml_element_find_first (g, "users");
				if (!group_users)
					group_users = gst_xml_element_add (g, "users");

				group_users = gst_xml_element_add (group_users, "user");
				gst_xml_element_set_content (group_users, user_name);
			}
		}
		g_free (buf);
	}

	g_free (user_name);
}

gchar **
user_value_comment_array (xmlNodePtr node)
{
	gchar **buf = NULL;
	gchar *comment;

	comment = generic_value_string (node, "comment");	
	if (comment)
		buf =  g_strsplit (comment, ",", 4);

	g_free (comment);
	return buf;
}

void
generic_set_value_string (xmlNodePtr node, const gchar *name, const gchar *value)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);

	gst_xml_set_child_content (node, (gchar *)name, (gchar *)value);
}

void
generic_set_value_integer (xmlNodePtr node, const gchar *name, gint value)
{
	gchar *buf;
	
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);

	buf = g_strdup_printf ("%d", value);
	gst_xml_set_child_content (node, (gchar *)name, buf);
	g_free (buf);
}

void
user_set_value_comment_array (xmlNodePtr node, gchar **comment)
{
	gint i, len;
	gchar *buf;

	/* Empty comment field if no new comment */
	if (!comment) {
		generic_set_value_string (node, "comment", "");
		return;
	}

	/* All this hassle is to remove empty "," from the ebd of comment */
	
	/* Get len of **comment */
	len = 0;
	for (i = 0; i < 4; i++) {
		if (comment[i])
			len++;
		else
			break;
	}
	
	/* Get rid of empty parts */
	for (i = len - 1; i >= 0; i--) {
		if (strlen (comment[i]) < 1) {
			g_free (comment[i]);
			comment[i] = NULL;
		} else
			break;
	}

	buf = g_strjoinv (",", comment);	
	generic_set_value_string (node, "comment", buf);
	g_free (buf);
}

void
user_set_value_group (xmlNodePtr user_node, const gchar *value)
{
	xmlNodePtr group, group_db;
	gchar *gid;

	group_db = get_corresp_field (user_node);
	group = get_node_by_data (group_db, "name", value);
	if (!group) {
		group = group_add_blank_xml (group_db);
		generic_set_value_string (group, "name", value);
	}
	
	gid = gst_xml_get_child_content (group, "gid");
	generic_set_value_string (user_node, "gid", gid);
	g_free (gid);
}
/*
gboolean
group_set_value_name (GstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_group_name (GTK_WINDOW (xd), node, value)) {
		generic_set_value_string (node, "gid", value);
		gst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
group_set_value_gid (GstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_group_gid (GTK_WINDOW (xd), node, value)) {
		generic_set_value_string (node, "gid", value);
		gst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}
*/
xmlNodePtr
user_add_blank_xml (xmlNodePtr user_db)
{
	xmlNodePtr user;

	g_return_val_if_fail (user_db != NULL, NULL);

	user = gst_xml_element_add (user_db, "user");

	gst_xml_element_add_with_content (user, "key", find_new_key (user_db));
	gst_xml_element_add (user, "login");
	gst_xml_element_add (user, "password");
	gst_xml_element_add (user, "uid");
	gst_xml_element_add (user, "gid");
	gst_xml_element_add (user, "comment");
	gst_xml_element_add (user, "home");
	gst_xml_element_add (user, "shell");
	gst_xml_element_add (user, "last_mod");

	gst_xml_element_add (user, "passwd_min_life");
	gst_xml_element_add (user, "passwd_max_life");
	gst_xml_element_add (user, "passwd_exp_warn");
	
	gst_xml_element_add (user, "passwd_exp_disable");
	gst_xml_element_add (user, "passwd_disable");
	gst_xml_element_add (user, "reserved");
	gst_xml_element_add_with_content (user, "is_shadow", "1");

	return user;
}

xmlNodePtr
group_add_blank_xml (xmlNodePtr group_db)
{
	xmlNodePtr group;
	gchar* content;

	g_return_val_if_fail (group_db != NULL, NULL);

	group = gst_xml_element_add (group_db, "group");

	content = find_new_key (group_db);
	gst_xml_element_add_with_content (group, "key", content);
	g_free (content);
	gst_xml_element_add (group, "name");
	gst_xml_element_add (group, "gid");
	gst_xml_element_add (group, "users");

	return group;
}
/*
static void
add_group_users (xmlNodePtr group_node, gchar *name)
{
	xmlNodePtr user;

	g_return_if_fail (group_node != NULL);

	user = gst_xml_element_find_first (group_node, "users");
	if (!user)
		user = gst_xml_element_add (group_node, "users");

	user = gst_xml_element_add (user, "user");
	gst_xml_element_set_content (user, name);
}
*/

static void
del_group_users (xmlNodePtr group_node)
{
	xmlNodePtr node;
	
	g_return_if_fail (group_node != NULL);

	node = gst_xml_element_find_first (group_node, "users");
	if (!node)
		return;

	gst_xml_element_destroy_children (node);
}

static void
user_add_extra_groups (GList *groups_list, const gchar *username)
{
	xmlNodePtr groupdb = gst_xml_element_find_first (gst_xml_doc_get_root (tool->config), "groupdb");
	xmlNodePtr node, users, user_node;
	gchar *groupname;
	GList *list;
	gboolean found;
	
	if (!groups_list)
		return;
	
	for (node = gst_xml_element_find_first (groupdb, "group"); node != NULL; node = gst_xml_element_find_next (node, "group")) {
		groupname = gst_xml_get_child_content (node, "name");
		list = groups_list;
		found = FALSE;
	
		/* is the group in the list? */
		while (list && !found) {
			if (strcmp (groupname, list->data) == 0) 
				found = TRUE;
			list = list->next;
		}
		
		if (found) {
			users = gst_xml_element_find_first (node, "users");
			user_node = gst_xml_element_add (users, "user");
			gst_xml_element_set_content (user_node, (gchar *) username);
			g_list_remove (groups_list, groupname);
		}
		g_free (groupname);
	}
	
	g_list_free (groups_list);
}

void
user_update_xml (xmlNodePtr node, UserAccountData *data, gboolean change_password)
{
	gchar *comment;

	gst_xml_set_child_content (node, "login", data->login);
	gst_xml_set_child_content (node, "uid", data->uid);
        comment = g_strjoin (",", data->name, data->location, data->work_phone, data->home_phone, data->other_info, NULL);
	gst_xml_set_child_content (node, "comment", comment);
	g_free (comment);
	gst_xml_set_child_content (node, "gid", data->gid);
	gst_xml_set_child_content (node, "home", data->home);
	gst_xml_set_child_content (node, "shell", data->shell);
	if (change_password) 
		passwd_set (node, data->password1);

	gst_xml_set_child_content (node, "passwd_max_life", data->pwd_maxdays);
	gst_xml_set_child_content (node, "passwd_min_life", data->pwd_mindays);
	gst_xml_set_child_content (node, "passwd_exp_warn", data->pwd_warndays);

	user_add_extra_groups (data->extra_groups, data->login);
}

static void
update_gid_in_users (xmlNodePtr node, gchar *new_gid)
{
	xmlNodePtr users = get_root_node (NODE_USER);
	gchar *old_gid = gst_xml_get_child_content (node, "gid");
	gchar *gid;

	for (users = gst_xml_element_find_first (users, "user");
	     users != NULL;
	     users = gst_xml_element_find_next (users, "user"))
	{
		gid = gst_xml_get_child_content (users, "gid");
		if (strcmp (gid, old_gid) == 0) 
			gst_xml_set_child_content (users, "gid", new_gid);

		g_free (gid);
	}

	g_free (old_gid);
}

void
group_update_xml (xmlNodePtr node, gchar *name, gchar *gid, GList *users)
{
	xmlNodePtr u, n;

	update_gid_in_users (node, gid);
	
	gst_xml_set_child_content (node, "name", name);
	gst_xml_set_child_content (node, "gid", gid);
	
	del_group_users (node);
	
	while (users) {
		u = gst_xml_element_find_first (node, "users");
		n = gst_xml_element_add (u, "user");
		gst_xml_element_set_content (n, users->data);
		users = users->next;
	}
}

gboolean
node_exists (xmlNodePtr node, const gchar *name, const gchar *val)
{
	xmlNodePtr n0;
	gchar *buf, *key;
	xmlNodePtr parent;
	gboolean self;

	parent = get_db_node (node);
	if (parent == node)
		self = FALSE;
	else
		self = TRUE;
	
	if (strcmp (parent->name, "userdb") == 0)
		key = g_strdup ("user");
	else if (strcmp (parent->name, "groupdb") == 0)
		key = g_strdup ("group");
	
	for (n0 = gst_xml_element_find_first (parent, key); n0 != NULL; n0 = gst_xml_element_find_next (n0, key))
	{
		if (self && n0 == node)
			continue;  /* Self */

		buf = gst_xml_get_child_content (n0, (gchar *)name);

		if (!buf)
			continue;  /* No content */

		if (!strcmp (val, buf))
		{
			g_free (key);
			g_free (buf);  /* Woohoo! found! */
			return TRUE;
		}

		g_free (buf);
	}
	g_free (key);

	return FALSE;
}

GList *
get_group_users (xmlNodePtr group_node)
{
	GList      *userlist = NULL;
	xmlNodePtr  node, u;
	gchar      *user;

	g_return_val_if_fail (group_node != NULL, NULL);

	node = gst_xml_element_find_first (group_node, "users");
	if (!node)
		return NULL;

	for (u = gst_xml_element_find_first (node, "user"); u; u = gst_xml_element_find_next (u, "user")) {
		user = gst_xml_element_get_content (u);
		userlist = g_list_insert_sorted (userlist, user, my_strcmp);
	}

	return userlist;
}

GList *
get_list_from_node (gchar *field, gint table)
{
	GList *list = NULL;
	xmlNodePtr n, u;
	gchar *key;

	n = get_root_node (table);

	if (!n)
		return NULL;
	
	if (strcmp (n->name, "userdb") == 0) {
		key = g_strdup ("user");
	} else {
		key = g_strdup ("group");
	}
	

	for (u = gst_xml_element_find_first (n, key); u != NULL; u = gst_xml_element_find_next (u, key))
		list = g_list_insert_sorted (list, gst_xml_get_child_content (u, field), my_strcmp);

	g_free (key);

	return list;
}

gchar*
group_xml_get_gid (xmlNodePtr root, gchar *name)
{
	xmlNodePtr node;
	
	for (node = gst_xml_element_find_first (root, "group"); node != NULL; node = gst_xml_element_find_next (node, "group")) {
		if (strcmp (gst_xml_get_child_content (node, "name"), name) == 0) {
			return gst_xml_get_child_content (node, "gid");
		}
	}
	return NULL;
}

gchar *
get_group_name (gchar *gid)
{
	xmlNodePtr groupdb = gst_xml_element_find_first (gst_xml_doc_get_root (tool->config), "groupdb");
	xmlNodePtr n;
	
	for (n = gst_xml_element_find_first (groupdb, "group"); n != NULL; n = gst_xml_element_find_next (n, "group")) {
		if (strcmp (gid, gst_xml_get_child_content (n, "gid")) == 0) 
			return gst_xml_get_child_content (n, "name");
	}
	
	return NULL;
}
