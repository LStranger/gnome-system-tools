/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user_settings.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#include <stdlib.h>
#include <gnome.h>

#include "global.h"
#include "callbacks.h"
#include "user_group.h"
#include "transfer.h"
#include "e-table.h"
#include "user_settings.h"

extern XstTool *tool;

static gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
}

static GList *
get_user_groups (xmlNodePtr user_node)
{
	xmlNodePtr group_node, g, group_users;
	gchar *user_name, *buf;
	GList *grouplist = NULL;

	g_return_val_if_fail (user_node != NULL, NULL);

	group_node = get_corresp_field (user_node);
	user_name = xst_xml_get_child_content (user_node, "login");

	if (!user_name)
		return NULL;

	for (g = xst_xml_element_find_first (group_node, "group");
	     g;
	     g = xst_xml_element_find_next (g, "group"))
	{
		
		group_users = xst_xml_element_find_first (g, "users");
		for (group_users = xst_xml_element_find_first (group_users, "user");
		     group_users;
		     group_users = xst_xml_element_find_next (group_users, "user"))
		{
			buf = xst_xml_element_get_content (group_users);
			if (!buf)
				continue;

			if (!strcmp (user_name, buf))
			{
				grouplist = g_list_prepend (grouplist,
							    xst_xml_get_child_content (g, "name"));
			}

			g_free (buf);
		}
	}

	g_free (user_name);
	return grouplist;
}

static GList *
get_group_list (gchar *field, xmlNodePtr user_node)
{
	GList *list = NULL;
	xmlNodePtr node, u;

	node = get_corresp_field (user_node);
	
	if (!node)
		return NULL;

	for (u = xst_xml_element_find_first (node, "group");
	     u;
	     u = xst_xml_element_find_next (u, "group"))
	{

		if (check_node_complexity (u))
			list = g_list_prepend (list, xst_xml_get_child_content (u, field));
	}

	return list;
}

static void
user_fill_settings_group (GtkCombo *combo, xmlNodePtr node)
{
	GList *tmp_list, *items;
	gchar *name;

	items = NULL;
	tmp_list = get_group_list ("name", node);
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

static UserSettingsBasic *
user_settings_basic_prepare (void)
{
	/* Map user_settings_dialog glade "Basic" tab widgets to our struct and return it. */
	UserSettingsBasic *ub;
	XstDialog *xd;

	xd = tool->main_dialog;
	ub = g_new (UserSettingsBasic, 1);

	ub->name    = GTK_ENTRY (xst_dialog_get_widget (xd, "user_settings_name"));
	ub->comment = GTK_ENTRY (xst_dialog_get_widget (xd, "user_settings_comment"));
	ub->home    = GTK_ENTRY (xst_dialog_get_widget (xd, "user_settings_home"));
	ub->shell   = GTK_ENTRY (xst_dialog_get_widget (xd, "user_settings_shell"));
	ub->uid     = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_settings_uid"));
	ub->advanced = xst_dialog_get_widget (xd, "user_settings_advanced");

	return ub;
}

static UserSettingsGroup *
user_settings_group_prepare (void)
{
	/* Map user_settings_dialog glade "Groups" tab widgets to our struct and return it. */
	UserSettingsGroup *ug;
	XstDialog *xd;

	xd = tool->main_dialog;
	ug = g_new (UserSettingsGroup, 1);

	ug->main    = GTK_COMBO (xst_dialog_get_widget (xd, "user_settings_group"));
	ug->all     = GTK_CLIST (xst_dialog_get_widget (xd, "user_settings_gall"));
	ug->member  = GTK_CLIST (xst_dialog_get_widget (xd, "user_settings_gmember"));
	ug->add     = xst_dialog_get_widget (xd, "user_settings_add");
	ug->remove  = xst_dialog_get_widget (xd, "user_settings_remove");
	ug->set_primary = xst_dialog_get_widget (xd, "user_settings_primary");

	return ug;
}

static UserSettingsPwd *
user_settings_pwd_prepare (void)
{
	UserSettingsPwd *pw;
	XstDialog *xd;

	xd = tool->main_dialog;
	pw = g_new (UserSettingsPwd, 1);

	pw->quality = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (xd, "user_passwd_quality"));
	pw->optional = xst_dialog_get_widget (xd, "user_passwd_optional");
	pw->min  = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_passwd_min"));
	pw->max  = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_passwd_max"));
	pw->days = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_passwd_days"));

	return pw;
}

static void
user_settings_basic_fill (UserSettings *us)
{
	/* Fill "Basic" tab widgets. */
	gchar *buf;

	if (us->node != get_db_node (us->node)) /* user node (not userdb), not new. */
	{
		gtk_entry_set_text (us->basic->name,
				    xst_xml_get_child_content (us->node, "login"));
		gtk_entry_set_text (us->basic->comment,
				    xst_xml_get_child_content (us->node, "comment"));
		gtk_entry_set_text (us->basic->home,
				    xst_xml_get_child_content (us->node, "home"));
		gtk_entry_set_text (us->basic->shell,
				    xst_xml_get_child_content (us->node, "shell"));

		buf = xst_xml_get_child_content (us->node, "uid");
		gtk_spin_button_set_value (us->basic->uid, g_strtod (buf, NULL));
		g_free (buf);
	}

	else /* New user. */
	{
		gtk_spin_button_set_value (us->basic->uid,
					   g_strtod (find_new_id (us->node), NULL));
		
		gtk_entry_set_text (us->basic->shell, "/bin/bash");
	}
}

static void
user_settings_group_fill (UserSettings *us)
{
	/* Fill "Groups" tab widgets. */
	gchar *buf;
	xmlNodePtr gnode, dbnode;
	GList *users, *items, *members;

	/* Main group. */
	
	user_fill_settings_group (us->group->main, us->node);

	buf = xst_xml_get_child_content (us->node, "gid");
	dbnode = get_corresp_field (get_db_node (us->node));
	gnode = get_node_by_data (dbnode, "gid", buf);
	g_free (buf);
	buf = NULL;

	if (gnode)
		buf = xst_xml_get_child_content (gnode, "name");

	if (buf)
		my_gtk_entry_set_text (GTK_ENTRY (us->group->main->entry), buf);
	else
		my_gtk_entry_set_text (GTK_ENTRY (us->group->main->entry), "");
	
	/* Members Secondary groups */
	members = get_user_groups (us->node);
	my_gtk_clist_append_items (us->group->member, members);

	/* Others */

	users = get_group_list ("name", us->node);
	items = my_g_list_remove_duplicates (users, members);
	
	my_gtk_clist_append_items (us->group->all, items);
}

static void
user_settings_pwd_fill (UserSettings *us)
{
	gchar *buf;
	
	if (us->node != get_db_node (us->node)) /* user node (not userdb), not new. */
	{
		buf = xst_xml_get_child_content (us->node, "passwd_min_life");
		gtk_spin_button_set_value (us->pwd->min, g_strtod (buf, NULL));
		g_free (buf);

		buf = xst_xml_get_child_content (us->node, "passwd_max_life");
		gtk_spin_button_set_value (us->pwd->max, g_strtod (buf, NULL));
		g_free (buf);

		buf = xst_xml_get_child_content (us->node, "passwd_exp_warn");
		gtk_spin_button_set_value (us->pwd->days, g_strtod (buf, NULL));
		g_free (buf);
	}

	else
	{
		gtk_spin_button_set_value (us->pwd->min,  logindefs.passwd_min_day_use);
		gtk_spin_button_set_value (us->pwd->max,  logindefs.passwd_max_day_use);
		gtk_spin_button_set_value (us->pwd->days, logindefs.passwd_warning_advance_days);
	}
}

static void
user_settings_complexity (UserSettings *us)
{
	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED)
	{
		/* Show advanced widgets */
		gtk_widget_show (us->basic->advanced);
	}

	else
	{
		/* Hide advanced widgets */
		gtk_widget_hide (us->basic->advanced);
	}
}

void
user_settings_prepare (xmlNodePtr user_node)
{
	UserSettings *us;

	us = g_new (UserSettings, 1);
	
	us->dialog = GNOME_DIALOG (xst_dialog_get_widget (tool->main_dialog, "user_settings_dialog"));
	us->node  = user_node;
	us->table = TABLE_USER;
	us->basic = user_settings_basic_prepare ();
	us->group = user_settings_group_prepare ();
	us->pwd   = user_settings_pwd_prepare ();

	if (user_node == get_db_node (user_node))
		us->new = TRUE;
	else
		us->new = FALSE;
	
	user_settings_basic_fill (us);
	user_settings_group_fill (us);
	user_settings_pwd_fill   (us);

	user_settings_complexity (us);

	gtk_object_set_data (GTK_OBJECT (us->dialog), "UserSettings", us);
	gtk_widget_show (GTK_WIDGET (us->dialog));
}

void
user_settings_destroy (UserSettings *us)
{
	g_free (us->basic);
	g_free (us->group);
	g_free (us->pwd);
	gtk_object_remove_data (GTK_OBJECT (us->dialog), "UserSettings");
	g_free (us);
}

void
user_settings_helper (UserSettings *us)
{
	/* In basic complexity mode user doesn't see all fields, so we have to fill them. */

	/* Hardcoded homedir preffix - BAD */
	gtk_entry_set_text (us->basic->home,
			    g_strdup_printf ("/home/%s", gtk_entry_get_text (us->basic->name)));
}
