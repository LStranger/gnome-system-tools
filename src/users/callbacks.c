/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of users-admin, a ximian-setup-tool frontend 
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>
#include <gnome.h>

#include "callbacks.h"
#include "user_group.h"
#include "transfer.h"
#include "passwd.h"
#include "e-table.h"
#include "user_settings.h"
#include "profile.h"
#include "user-druid.h"
#include "user-account-editor.h"

XstTool *tool;

/* Main window callbacks */
/* "global" callbacks :) */

void
on_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
			 guint page_num, gpointer user_dat)
{
	set_active_table (page_num);
}

void
on_showall_toggled (GtkToggleButton *toggle, gpointer user_data)
{
	xst_conf_set_boolean (tool, "showall", gtk_toggle_button_get_active (toggle));
	tables_update_content ();
}

void
on_settings_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	ud = g_new (ug_data, 1);

	ud->new = FALSE;
	ud->table = GPOINTER_TO_INT (user_data);
	ud->node = get_selected_node ();

	settings_prepare (ud);
}

/* Users tab */

void
on_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	if (xst_dialog_get_complexity (tool->main_dialog) == XST_DIALOG_ADVANCED) {
		UserAccount *account = user_account_get_default ();
		UserAccountEditor *editor = user_account_editor_new (account);
		
		gtk_widget_show (GTK_WIDGET (editor));
	} else {
		UserDruid *druid = user_druid_new ();
		gtk_widget_show (GTK_WIDGET (druid));
	}
}

void
on_user_settings_clicked (GtkButton *button, gpointer user_data)
{
	UserAccount *account;
	UserAccountEditor *editor;
	xmlNodePtr node = get_selected_node ();

	account = user_account_get_by_node (node);
	editor = user_account_editor_new (account);

	gtk_widget_show (GTK_WIDGET (editor));
}

void
on_user_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));
	g_return_if_fail (node = get_selected_node ());

	if (check_login_delete (node)) {
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (TABLE_USER))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_USER, FALSE);
	}
}

void
on_user_profiles_clicked (GtkButton *button, gpointer user_data)
{
	XstDialog *xd;

	xd = XST_DIALOG (gtk_object_get_data (GTK_OBJECT (tool), PROFILE_DIALOG));
	gtk_widget_show (GTK_WIDGET (xd));
}

/* Groups tab */

void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	ud = g_new (ug_data, 1);

	ud->new = TRUE;
	ud->table = TABLE_GROUP;
	ud->node = get_root_node (ud->table);

	group_new_prepare (ud);
}

void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	xmlNodePtr node;

	g_return_if_fail (xst_tool_get_access (tool));

	node = get_selected_node ();

	if (check_group_delete (node))
	{
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (TABLE_GROUP))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_GROUP, FALSE);
	}
}

#ifdef NIS
/* Network tab. */

void
on_network_delete_clicked (GtkWidget *button, gpointer user_data)
{
	xmlNodePtr node;
	gboolean delete = FALSE;
	gint tbl = -1;

	g_return_if_fail (xst_tool_get_access (tool));

	node = get_selected_node ();

	/* Is it user or group? */

	if (!strcmp (node->name, "user"))
	{
		delete = check_login_delete (node);
		tbl = TABLE_NET_USER;
	}
	else if (!strcmp (node->name, "group"))
	{
		delete = check_group_delete (node);
		tbl = TABLE_NET_GROUP;
	}

	if (delete)
	{
		xst_dialog_modify (tool->main_dialog);
		if (delete_selected_node (tbl))
			xst_xml_element_destroy (node);

		actions_set_sensitive (TABLE_GROUP, FALSE);
	}
}

void
on_network_user_new_clicked (GtkButton *button, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	user_settings_prepare (get_root_node (TABLE_NET_USER));
}

void
on_network_group_new_clicked (GtkButton *button, gpointer user_data)
{
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	ud = g_new (ug_data, 1);

	ud->new = TRUE;
	ud->table = TABLE_NET_GROUP;
	ud->node = get_root_node (ud->table);

	group_new_prepare (ud);
}
#endif

/* Profiles tab */

void
on_pro_name_changed (GtkMenuItem *menu_item, gpointer user_data)
{
	profile_table_set_selected ((gchar *) user_data);
	tables_update_content ();
}

void
on_pro_del_clicked (GtkButton *button, gpointer user_data)
{
	XstDialog *xd = XST_DIALOG (gtk_object_get_data (GTK_OBJECT (tool), PROFILE_DIALOG));
	
	g_return_if_fail (xst_tool_get_access (tool));
	
	if (profile_table_del_profile (NULL))
		xst_dialog_modify (xd);
}

enum
{
	PROFILE_ERROR,
	PROFILE_NEW,
	PROFILE_COPY,
};

static void
pro_ask_name (gint action)
{
	gchar *buf;
	GtkWidget *w0;
	Profile *new, *pf = NULL;
	XstDialog *xd = XST_DIALOG (gtk_object_get_data (GTK_OBJECT (tool), PROFILE_DIALOG));
	
	switch (action)
	{
	case PROFILE_NEW:
		break;
		
	case PROFILE_COPY:
		w0 = xst_dialog_get_widget (xd, "profile_new_menu");
		buf = xst_ui_option_menu_get_selected_string (GTK_OPTION_MENU (w0));
		pf = profile_table_get_profile (buf);
		g_free (buf);
		break;
		
	case PROFILE_ERROR:
	default:
		g_warning ("pro_ask_name: Shouldn't be here");
		return;
	}

	buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (xd, "profile_new_name")));
	new = profile_add (pf, buf, TRUE);
	if (new) {
		buf = gtk_entry_get_text (GTK_ENTRY (xst_dialog_get_widget (xd, "profile_new_comment")));
		new->comment = g_strdup (buf);
		xst_dialog_modify (xd);
	}
	
}

static void
pro_prepare (gint action)
{
	GtkWidget *w;
	XstDialog *xd = XST_DIALOG (gtk_object_get_data (GTK_OBJECT (tool), PROFILE_DIALOG));

	w = xst_dialog_get_widget (xd, "profile_new_name");
	gtk_entry_set_text (GTK_ENTRY (w), "");
	gtk_widget_grab_focus (w);

	gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget (xd, "profile_new_comment")), "");

	w = xst_dialog_get_widget (xd, "profile_new_copy");
	
	if (action == PROFILE_NEW)
		gtk_widget_hide (w);
	else {
		GtkWidget *menu = xst_dialog_get_widget (xd, "profile_new_menu");
		GSList *list = profile_table_get_list ();
		Profile *pf = profile_table_get_profile (NULL);

		xst_ui_option_menu_clear (GTK_OPTION_MENU (menu));

		while (list) {
			xst_ui_option_menu_add_string (GTK_OPTION_MENU (menu), list->data);
			list = list->next;
		}
		g_slist_free (list);
		xst_ui_option_menu_set_selected_string (GTK_OPTION_MENU (menu), pf->name);
	
		gtk_widget_show (w);
	}
}

void
on_pro_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;
	gint res;
	XstDialog *xd = XST_DIALOG (gtk_object_get_data (GTK_OBJECT (tool), PROFILE_DIALOG));

	g_return_if_fail (xst_tool_get_access (tool));

	d = xst_dialog_get_widget (xd, "profile_new_dialog");
	pro_prepare (PROFILE_NEW);
	res = gnome_dialog_run (GNOME_DIALOG (d));

	if (res)
		return;

	pro_ask_name (PROFILE_NEW);
}

void
on_pro_copy_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *d;
	gint res;
	XstDialog *xd = XST_DIALOG (gtk_object_get_data (GTK_OBJECT (tool), PROFILE_DIALOG));
	
	g_return_if_fail (xst_tool_get_access (tool));
	
	d = xst_dialog_get_widget (xd, "profile_new_dialog");
	pro_prepare (PROFILE_COPY);
	res = gnome_dialog_run (GNOME_DIALOG (d));

	if (res)
		return;

	pro_ask_name (PROFILE_COPY);
}

void
on_pro_apply_clicked (XstDialog *xd, gpointer user_data)
{
	g_return_if_fail (xst_tool_get_access (tool));

	profile_save (NULL);
	tables_update_content ();

	xst_dialog_modify (tool->main_dialog);
}

/* Group settings callbacks */

void
on_group_settings_dialog_show (GtkWidget *widget, gpointer user_data)
{
	/* Set focus to user name entry. */
	gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "group_settings_name"));
}

static void
group_settings_dialog_close (void)
{
	GtkWidget *w0;
	ug_data *ud;

	/* Clear group name */

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (w0), "");

	/* Clear both lists. */

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_all");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_members");
	gtk_clist_clear (GTK_CLIST (w0));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	ud = gtk_object_get_data (GTK_OBJECT (w0), "data");
	g_free (ud);
	gtk_object_remove_data (GTK_OBJECT (w0), "data");
	gtk_widget_hide (w0);
}

void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data)
{
	group_settings_dialog_close ();
}

void
on_group_settings_dialog_delete_event (GtkWidget *w, gpointer user_data)
{
	group_settings_dialog_close ();
}

void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *w0;
	ug_data *ud;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	ud = gtk_object_get_data (GTK_OBJECT (w0), "data");

	if (group_update (ud))
	{
		xst_dialog_modify (tool->main_dialog);
		group_settings_dialog_close ();
	}
}

void
on_group_settings_add_clicked (GtkButton *button, gpointer user_data)
{
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_all"));
	members = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));

	while (all->selection)
	{
		row = GPOINTER_TO_INT (all->selection->data);
		gtk_clist_get_text (all, row, 0, &name);
		my_gtk_clist_append (members, g_strdup (name));
		gtk_clist_remove (all, row);
	}
}

void
on_group_settings_remove_clicked (GtkButton *button, gpointer user_data)
{
	/* TODO: Maybe mix with previous func? */
	GtkCList *all, *members;
	gchar *name;
	gint row;

	g_return_if_fail (xst_tool_get_access (tool));

	all = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_all"));
	members = GTK_CLIST (xst_dialog_get_widget (tool->main_dialog, "group_settings_members"));

	while (members->selection)
	{
		row = GPOINTER_TO_INT (members->selection->data);
		gtk_clist_get_text (members, row, 0, &name);
		my_gtk_clist_append (all, g_strdup (name));
		gtk_clist_remove (members, row);
	}
}

void
on_group_settings_all_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	GtkWidget *w0;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_add");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}

void
on_group_settings_members_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event,
		gpointer user_data)
{
	/* TODO: maybe mix with previous? */
	GtkWidget *w0;

	g_return_if_fail (xst_tool_get_access (tool));

	w0 = xst_dialog_get_widget (tool->main_dialog, "group_settings_remove");

	if (!clist->selection)
		gtk_widget_set_sensitive (w0, FALSE);
	else
		gtk_widget_set_sensitive (w0, TRUE);
}


/* Helpers .*/

void
actions_set_sensitive (gint table, gboolean state)
{
	switch (table)
	{
	case TABLE_USER:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_new",      TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_delete",   state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "user_settings", state);
		break;
	case TABLE_GROUP:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_new",      TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_delete",   state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "group_settings", state);
		break;
	case TABLE_NET_USER:
	case TABLE_NET_GROUP:
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_group_new", TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_user_new",  TRUE);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_delete",    state);
		xst_dialog_widget_set_user_sensitive (tool->main_dialog, "network_settings",  state);
		break;
	default:
		g_warning ("actions_set_sensitive: Shouldn't be here.");
		return;
	}
}

void
my_gtk_entry_set_text (void *entry, gchar *str)
{
	gtk_entry_set_text (GTK_ENTRY (entry), (str)? str: "");
}

