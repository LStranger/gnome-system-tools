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

void
generic_set_value (xmlNodePtr node, const gchar *name, const gchar *value)
{
	g_return_if_fail (node != NULL);
	g_return_if_fail (name != NULL);

	xst_xml_set_child_content (node, (gchar *)name, (gchar *)value);
}

gboolean
user_set_value_login (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_user_login (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "login", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_home (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_user_home (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "home", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_shell (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_user_shell (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "shell", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_comment (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_user_comment (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "comment", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_uid (XstDialog *xd, xmlNodePtr node, gchar const *value)
{
	if (check_user_uid (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "uid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_gid (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	xmlNodePtr group_node;

	group_node = get_corresp_field (node);
	
	if (check_group_gid (GTK_WINDOW (xd), group_node, value)) {
		generic_set_value (node, "gid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_group (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	return FALSE;
}

gboolean
group_set_value_name (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_group_name (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "gid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
group_set_value_gid (XstDialog *xd, xmlNodePtr node, const gchar *value)
{
	if (check_group_gid (GTK_WINDOW (xd), node, value)) {
		generic_set_value (node, "gid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}
