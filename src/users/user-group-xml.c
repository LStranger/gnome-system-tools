/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "xst.h"
#include "user-group-xml.h"

#include "user_group.h"
#include "profile.h"
#include "e-table.h"

void generic_set_value (xmlNodePtr node, const gchar *name, const gchar *value);

gchar *
generic_value_string (xmlNodePtr node, const gchar *name)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return xst_xml_get_child_content (node, (gchar *)name);
}

gint
generic_value_integer (xmlNodePtr node, const gchar *name)
{
	gchar *buf;
	gint val;
	
	g_return_val_if_fail (node != NULL, -1);
	g_return_val_if_fail (name != NULL, -1);

	buf = xst_xml_get_child_content (node, (gchar *)name);
	if (buf) {
		val = atoi (buf);
		g_free (buf);
	}
	else
		val = -1;

	return val;
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

GSList *
user_get_groups (xmlNodePtr user_node)
{
	xmlNodePtr group_node, g, group_users;
	gchar *user_name, *buf;
	GSList *grouplist = NULL;

	g_return_val_if_fail (user_node != NULL, NULL);

	group_node = get_corresp_field (user_node);
	user_name = user_value_login (user_node);

	if (!user_name)
		return NULL;

	for (g = xst_xml_element_find_first (group_node, "group"); g;
	     g = xst_xml_element_find_next (g, "group")) {
		group_users = xst_xml_element_find_first (g, "users");
		for (group_users = xst_xml_element_find_first (group_users, "user");
		     group_users;
		     group_users = xst_xml_element_find_next (group_users, "user")) {
			buf = xst_xml_element_get_content (group_users);
			if (!buf)
				continue;

			if (!strcmp (user_name, buf))
				grouplist = g_slist_prepend (grouplist, group_value_name (g));
			
			g_free (buf);
		}
	}

	g_free (user_name);
	return grouplist;
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
	     g = xst_xml_element_find_next (g, "group")) {
		group_users = xst_xml_element_find_first (g, "users");

		group_users = xst_xml_element_find_first (group_users, "user");
		while (group_users) {
			found = FALSE;
			buf = xst_xml_element_get_content (group_users);
			if (buf) {
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

void
user_set_groups (xmlNodePtr user_node, GSList *list)
{
	xmlNodePtr group_node, g, group_users;
	gchar *user_name, *buf;
	GSList *tmp;

	g_return_if_fail (user_node != NULL);

	del_user_groups (user_node);
	
	group_node = get_corresp_field (user_node);
	user_name = xst_xml_get_child_content (user_node, "login");

	for (g = xst_xml_element_find_first (group_node, "group"); g;
	     g = xst_xml_element_find_next (g, "group")) {
		buf = xst_xml_get_child_content (g, "name");

		tmp = list;
		while (tmp) {
			gchar *group_name = tmp->data;
			tmp = tmp->next;

			if (!strcmp (buf, group_name)) {
				group_users = xst_xml_element_find_first (g, "users");
				if (!group_users)
					group_users = xst_xml_element_add (g, "users");

				group_users = xst_xml_element_add (group_users, "user");
				xst_xml_element_set_content (group_users, user_name);
			}
		}
		g_free (buf);
	}

	g_free (user_name);
}

void
generic_set_value_string (xmlNodePtr node, const gchar *name, const gchar *value)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);

	xst_xml_set_child_content (node, (gchar *)name, (gchar *)value);
}

void
generic_set_value_integer (xmlNodePtr node, const gchar *name, gint value)
{
	gchar *buf;
	
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);

	buf = g_strdup_printf ("%d", value);
	xst_xml_set_child_content (node, (gchar *)name, buf);
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
	
	gid = xst_xml_get_child_content (group, "gid");
	generic_set_value_string (user_node, "gid", gid);
	g_free (gid);
}

gboolean
group_set_value_name (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_group_name (GTK_WINDOW (xd), node, value)) {
		generic_set_value_string (node, "gid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
group_set_value_gid (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_group_gid (GTK_WINDOW (xd), node, value)) {
		generic_set_value_string (node, "gid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

xmlNodePtr
user_add_blank_xml (xmlNodePtr user_db)
{
	xmlNodePtr user;
	Profile *pf;

	g_return_val_if_fail (user_db != NULL, NULL);

	user = xst_xml_element_add (user_db, "user");
	pf = profile_table_get_profile (NULL);

	xst_xml_element_add_with_content (user, "key", find_new_key (user_db));
	xst_xml_element_add (user, "login");
	xst_xml_element_add (user, "password");
	xst_xml_element_add (user, "uid");	
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

	current_table_new_row (user, TABLE_USER);
	
	return user;
}

xmlNodePtr
group_add_blank_xml (xmlNodePtr group_db)
{
	xmlNodePtr group;

	g_return_val_if_fail (group_db != NULL, NULL);

	group = xst_xml_element_add (group_db, "group");

	xst_xml_element_add_with_content (group, "key", find_new_key (group_db));
	xst_xml_element_add (group, "name");
	xst_xml_element_add (group, "password");
	xst_xml_element_add_with_content (group, "gid", find_new_id (group_db));
	xst_xml_element_add (group, "users");

	current_table_new_row (group, TABLE_GROUP);
	
	return group;
}
