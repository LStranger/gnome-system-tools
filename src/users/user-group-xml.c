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

#include "global.h"
#include "user-group-xml.h"

#include "user_group.h"

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



static gboolean
user_check_login (XstDialog *xd, xmlNodePtr node, gchar *login)
{
	gchar *buf = NULL;

	g_return_val_if_fail (node != NULL, FALSE);
	g_return_val_if_fail (login != NULL, FALSE);

	if (strlen (login) < 1)
		buf = g_strdup (_("The username is empty."));

	else if (!is_valid_name (login)) /*TODO: re */
		buf = g_strdup (_("Please set a valid username, using only lower-case letters."));

	else if (node_exsists (node, "login", login)) /* TODO: re */
		buf = g_strdup (_("Username already exsists."));

	if (buf)
	{
		GnomeDialog *dialog;

		dialog = GNOME_DIALOG (gnome_error_dialog_parented (buf, GTK_WINDOW (xd)));
		gnome_dialog_run (dialog);
		g_free (buf);
		return FALSE;
	}

	return TRUE;
}

static gboolean
user_check_home (XstDialog *xd, xmlNodePtr node, gchar *val)
{
	if (strlen (val) < 1)
	{
		GnomeDialog *dialog;
		
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
						   (_("Home directory must not be empty."), GTK_WINDOW (xd)));
		gnome_dialog_run (dialog);
		return FALSE;
	}

	return TRUE;
}

static gboolean
user_check_uid (XstDialog *xd, xmlNodePtr node, gchar *val)
{
	if (node_exsists (node, "uid", val)) /* TODO: re */
	{
		GnomeDialog *dialog;
		
		dialog = GNOME_DIALOG (gnome_error_dialog_parented
				(_("Such user id already exsists."), GTK_WINDOW (xd)));
		gnome_dialog_run (dialog);
		return FALSE;
	}

	return TRUE;
}

static gboolean
user_check_gid (XstDialog *xd, xmlNodePtr node, gchar *val)
{
	/* TODO: just do it! */
	return TRUE;
}

static gint
check_user_group (XstDialog *xd, xmlNodePtr node, gchar *val)
{
	gchar *buf;
	GnomeDialog *dialog;
	xmlNodePtr group_node;

	g_return_val_if_fail (us != NULL, -1);

	buf = parse_group (us); /* TODO: re */
	group_node = get_corresp_field (us->node);

	if (node_exsists (group_node, "name", buf)) /* TODO: re */
		return 0;

	if (!is_valid_name (buf)) /* TODO: re */
	{
		dialog = GNOME_DIALOG (gnome_error_dialog_parented(_(
			"Please set a valid main group name, with only lower-case letters,"
			"\nor select one from pull-down menu."),
												 GTK_WINDOW (xd)));

		gnome_dialog_run (dialog);
		return -1;
	}

	/* Group not found, but name is valid. */
	return 1;
}


void
generic_set_value (xmlNodePtr node, const gchar *name, gchar *value)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	xst_xml_set_child_content (node, (gchar *)name, value);
}

gboolean
user_set_value_login (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	if (user_check_login (xd, node, value)) {
		generic_set_value (node, "login", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_home (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	if (user_check_home (xd, node, value)) {
		generic_set_value (node, "home", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_shell (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	/* Should we check something here? */
	generic_set_value (node, "shell", value);
	xst_dialog_modify (xd);
	return TRUE;
}

gboolean
user_set_value_comment (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	/* Should we check something here? */
	generic_set_value (node, "comment", value);
	xst_dialog_modify (xd);
	return TRUE;
}

gboolean
user_set_value_uid (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	if (user_check_uid (xd, node, value)) {
		generic_set_value (node, "uid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_gid (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	if (user_check_gid (xd, node, value)) {
		generic_set_value (node, "gid", value);
		xst_dialog_modify (xd);
		return TRUE;
	}

	return FALSE;
}

gboolean
user_set_value_group (XstDialog *xd, xmlNodePtr node, gchar *value)
{
	gint val;
	
	val = user_check_group (xd, node, value);

	switch (val) {
	case 0: /* Group exists */
		
		xst_dialog_modify (xd);
		retval = TRUE;
		break;
	case 1: /* New valid group */
		
		xst_dialog_modify (xd);
		retval = TRUE;
		break;
	case -1:
	default:
		retval = FALSE;
		break;
	}

	return retval;
}

