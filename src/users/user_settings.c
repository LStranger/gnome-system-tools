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

#include "xst.h"
#include "user_settings.h"

extern XstTool *tool;

static void
user_account_gui_add (GtkButton *button, gpointer data)
{
	UserAccountGui *gui = data;
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = gui->all;
	members = gui->member;

	while (all->selection) {
		row = GPOINTER_TO_INT (all->selection->data);
		gtk_clist_get_text (all, row, 0, &name);
		my_gtk_clist_append (members, g_strdup (name));
		gtk_clist_remove (all, row);
	}
}

static void
user_account_gui_remove (GtkButton *button, gpointer data)
{
	UserAccountGui *gui = data;
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = gui->all;
	members = gui->member;

	while (members->selection) {
		row = GPOINTER_TO_INT (members->selection->data);
		gtk_clist_get_text (members, row, 0, &name);
		my_gtk_clist_append (all, g_strdup (name));
		gtk_clist_remove (members, row);
	}
}

static void
user_account_gui_member_select (GtkCList *clist, gint row, gint column, GdkEventButton *event,
				gpointer data)
{
	UserAccountGui *gui = data;

	g_return_if_fail (xst_tool_get_access (tool));

	if (clist->selection) {
		gtk_widget_set_sensitive (GTK_WIDGET (gui->remove), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->set_primary), TRUE);
	} else {
		gtk_widget_set_sensitive (GTK_WIDGET (gui->remove), FALSE);
		gtk_widget_set_sensitive (GTK_WIDGET (gui->set_primary), FALSE);
	}
}

static void
user_account_gui_all_select (GtkCList *clist, gint row, gint column, GdkEventButton *event,
			     gpointer data)
{
	UserAccountGui *gui = data;

	g_return_if_fail (xst_tool_get_access (tool));

	if (clist->selection)
		gtk_widget_set_sensitive (GTK_WIDGET (gui->add), TRUE);
	else
		gtk_widget_set_sensitive (GTK_WIDGET (gui->add), FALSE);
}

static gint
char_sort_func (gconstpointer a, gconstpointer b)
{
	return (strcmp (a, b));
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

UserAccountGui *
user_account_gui_new (UserAccount *account)
{
	UserAccountGui *gui;

	gui = g_new0 (UserAccountGui, 1);
	gui->account = account;
	gui->xml = glade_xml_new (tool->glade_path, NULL);

	gui->name    = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_name"));
	gui->comment = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_comment"));
	gui->home    = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_settings_home"));
	gui->shell   = GTK_COMBO (glade_xml_get_widget (gui->xml, "user_settings_shell"));
	gui->uid     = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_settings_uid"));
	gui->advanced = glade_xml_get_widget (gui->xml, "user_settings_advanced");

	gui->group   = GTK_COMBO (glade_xml_get_widget (gui->xml, "user_settings_group"));
	gui->all     = GTK_CLIST (glade_xml_get_widget (gui->xml, "user_settings_gall"));
	gui->member  = GTK_CLIST (glade_xml_get_widget (gui->xml, "user_settings_gmember"));
	gui->add     = glade_xml_get_widget (gui->xml, "user_settings_add");
	gui->remove  = glade_xml_get_widget (gui->xml, "user_settings_remove");
	gui->set_primary = glade_xml_get_widget (gui->xml, "user_settings_primary");

	gui->pwd1 = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_passwd_entry1"));
	gui->pwd2 = GTK_ENTRY (glade_xml_get_widget (gui->xml, "user_passwd_entry2"));
	gui->quality = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_quality"));
	gui->optional = glade_xml_get_widget (gui->xml, "user_passwd_optional");
	gui->min  = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_min"));
	gui->max  = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_max"));
	gui->days = GTK_SPIN_BUTTON (glade_xml_get_widget (gui->xml, "user_passwd_days"));

	gtk_signal_connect (GTK_OBJECT (gui->add), "clicked",
			    GTK_SIGNAL_FUNC (user_account_gui_add), gui);
	gtk_signal_connect (GTK_OBJECT (gui->remove), "clicked",
			    GTK_SIGNAL_FUNC (user_account_gui_remove), gui);

	gtk_signal_connect (GTK_OBJECT (gui->all), "select_row",
			    GTK_SIGNAL_FUNC (user_account_gui_all_select), gui);
	gtk_signal_connect (GTK_OBJECT (gui->all), "unselect_row",
			    GTK_SIGNAL_FUNC (user_account_gui_all_select), gui);

	gtk_signal_connect (GTK_OBJECT (gui->member), "select_row",
			    GTK_SIGNAL_FUNC (user_account_gui_member_select), gui);
	gtk_signal_connect (GTK_OBJECT (gui->member), "unselect_row",
			    GTK_SIGNAL_FUNC (user_account_gui_member_select), gui);
	
	return gui;
}

static void
user_account_shells_setup (UserAccountGui *gui)
{
	xmlNodePtr root, node;
	GtkWidget *li;

	root = xst_xml_doc_get_root (tool->config);
	root = xst_xml_element_find_first (root, "shells");

	if (!root)
		return;

	gtk_list_clear_items (GTK_LIST (gui->shell->list), 0, -1);
	
	node = xst_xml_element_find_first (root, "shell");
	while (node) {
		li = gtk_list_item_new_with_label (xst_xml_element_get_content (node));
		gtk_widget_show (li);
		gtk_container_add (GTK_CONTAINER (gui->shell->list), li);
		
		node = xst_xml_element_find_next (node, "shell");
	}
}

static void
user_account_groups_setup (GtkCombo *combo, xmlNodePtr node)
{
	GList *tmp_list, *items;
	gchar *name;

	items = NULL;
	tmp_list = get_group_list ("name", node);
	while (tmp_list) {
		name = tmp_list->data;
		tmp_list = tmp_list->next;

		items = g_list_append (items, name);
	}
	g_list_free (tmp_list);

	items = g_list_sort (items, char_sort_func);

	gtk_combo_set_popdown_strings (combo, items);
	g_list_free (items);
}

static void
user_settings_complexity (UserAccountGui *gui)
{
	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED) {
		gtk_widget_show (gui->advanced);
		gtk_widget_show (gui->optional);
	} else {
		gtk_widget_hide (gui->advanced);
		gtk_widget_hide (gui->optional);
	}
}

void
user_account_gui_setup (UserAccountGui *gui, GtkWidget *top)
{
	GList *users, *items;
	UserAccount *account = gui->account;
	
	gui->top = top;

	user_account_shells_setup (gui);
	user_account_groups_setup (gui->group, account->node);
	
	my_gtk_entry_set_text (gui->name, account->name);
	my_gtk_entry_set_text (gui->comment, account->comment);
	my_gtk_entry_set_text (gui->home, account->home);
	my_gtk_entry_set_text (GTK_ENTRY (gui->shell->entry), account->shell);
	gtk_spin_button_set_value (gui->uid, atoi (account->uid));

	my_gtk_entry_set_text (GTK_ENTRY (gui->group->entry), account->group);
	my_gtk_clist_append_items (gui->member, (GList *)account->extra_groups);
	/* Others */
	users = get_group_list ("name", account->node);
	items = my_g_list_remove_duplicates (users, (GList *)account->extra_groups);
	my_gtk_clist_append_items (gui->all, items);

	gtk_spin_button_set_value (gui->min, account->pwd_mindays);
	gtk_spin_button_set_value (gui->max, account->pwd_maxdays);
	gtk_spin_button_set_value (gui->days, account->pwd_warndays);

	gtk_widget_show_all (top);
	
	user_settings_complexity (gui);
}

gboolean
user_account_gui_save (UserAccountGui *gui)
{
	UserAccount *account;
	xmlNodePtr node = gui->account->node;
	GtkWindow *parent = GTK_WINDOW (gui->top);
	gchar *buf, *error;
	gint row = 0;

	account = g_new0 (UserAccount, 1);
	account->node = node;
	
	buf = gtk_entry_get_text (gui->name);
	if ((error = check_user_login (node, buf)))
		goto err;
	account->name = g_strdup (buf);
	
	buf = gtk_entry_get_text (gui->comment);
	if ((error = check_user_comment (node, buf)))
		goto err;
	account->comment = g_strdup (buf);
	
	buf = gtk_entry_get_text (gui->home);
	if ((error = check_user_home (node, buf)))
		goto err;
	account->home = g_strdup (buf);
	
	buf = gtk_entry_get_text (GTK_ENTRY (gui->shell->entry));
	if ((error = check_user_shell (node, buf)))
		goto err;
	account->shell = g_strdup (buf);
	
	buf = g_strdup_printf ("%d", gtk_spin_button_get_value_as_int (gui->uid));
	if ((error = check_user_uid (node, buf))) {
		g_free (buf);
		goto err;
	}
	account->uid = (buf);

	buf = gtk_entry_get_text (GTK_ENTRY (gui->group->entry));
	if ((error = check_user_group (account, buf)))
		goto err;
	account->group = g_strdup (buf);

	account->pwd_mindays = gtk_spin_button_get_value_as_int (gui->min);
	account->pwd_maxdays = gtk_spin_button_get_value_as_int (gui->max);
	account->pwd_warndays = gtk_spin_button_get_value_as_int (gui->days);

	account->extra_groups = NULL;
	while (gtk_clist_get_text (gui->member, row++, 0, &buf))
		account->extra_groups = g_slist_prepend (account->extra_groups, buf);
	
	user_account_destroy (gui->account);
	gui->account = account;
	
	return TRUE;

err:
	user_account_gui_error (parent, error);
	user_account_destroy (account);
	return FALSE;
}

void
user_account_gui_error (GtkWindow *parent, gchar *error)
{
	GtkWidget *d = gnome_error_dialog_parented (error, parent);

	gnome_dialog_run (GNOME_DIALOG (d));
	g_free (error);
}

void
user_account_gui_destroy (UserAccountGui *gui)
{
	gtk_object_unref (GTK_OBJECT (gui->xml));
	user_account_destroy (gui->account);
	g_free (gui);
}
