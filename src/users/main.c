/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of users-admin, a ximian-setup-tool frontend 
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

#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>

#include <time.h>
#include <stdlib.h>

#include "global.h"

#include "transfer.h"
#include "e-table.h"
#include "callbacks.h"
#include "e-search-bar/e-search-bar.h"

XstTool *tool;

void quit_cb (XstTool *tool, gpointer data);

static XstDialogSignal signals[] = {
	{ "notebook",                    "switch_page",   on_notebook_switch_page },
	{ "user_settings_dialog",        "delete_event",  on_user_settings_dialog_delete_event },
	{ "user_settings_dialog",        "show",          on_user_settings_dialog_show },
	{ "user_settings_dialog",        "clicked",       on_user_settings_clicked },
	{ "group_settings_dialog",       "delete_event",  on_group_settings_dialog_delete_event },
	{ "group_settings_dialog",       "show",          on_group_settings_dialog_show },
	{ "group_settings_ok",           "clicked",       on_group_settings_ok_clicked },
	{ "group_settings_cancel",       "clicked",       on_group_settings_cancel_clicked },
	{ "group_settings_add",          "clicked",       on_group_settings_add_clicked },
	{ "group_settings_remove",       "clicked",       on_group_settings_remove_clicked },
	{ "group_settings_members",      "select_row",    on_group_settings_members_select_row },
	{ "group_settings_members",      "unselect_row",  on_group_settings_members_select_row },
	{ "group_settings_all",          "select_row",    on_group_settings_all_select_row },
	{ "group_settings_all",          "unselect_row",  on_group_settings_all_select_row },
	{ "user_new",                    "clicked",       on_user_new_clicked },
	{ "user_delete",                 "clicked",       on_user_delete_clicked },
	{ "group_new",                   "clicked",       on_group_new_clicked },
	{ "group_delete",                "clicked",       on_group_delete_clicked },
	{ "network_user_new",            "clicked",       on_network_user_new_clicked },
	{ "network_group_new",           "clicked",       on_network_group_new_clicked },
	{ "network_delete",              "clicked",       on_network_delete_clicked },
	{ "showall",                     "toggled",       on_showall_toggled },
	{ "user_settings_gmember",       "select_row",    on_user_settings_gmember_select_row },
	{ "user_settings_gmember",       "unselect_row",  on_user_settings_gmember_select_row },
	{ "user_settings_gall",          "select_row",    on_user_settings_gall_select_row },
	{ "user_settings_gall",          "unselect_row",  on_user_settings_gall_select_row },
	{ "user_settings_add",           "clicked",       on_user_settings_add_clicked },
	{ "user_settings_remove",        "clicked",       on_user_settings_remove_clicked },
	{ "pro_del",                     "clicked",       on_pro_del_clicked },
	{ "pro_save",                    "clicked",       on_pro_save_clicked },
	{ "pro_new",                     "clicked",       on_pro_new_clicked },
	{ "pro_copy",                    "clicked",       on_pro_copy_clicked },
	{ "user_passwd_change",          "clicked",       on_user_passwd_change_clicked },
	{ "user_passwd_random",          "clicked",       on_user_passwd_random_clicked },
	{ NULL }};

static const XstWidgetPolicy policies[] = {
	/* Name                     Basic                        Advanced                   Root   User */
	{ "users_holder",           XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, FALSE, TRUE  },
	{ "user_new",               XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "user_settings_basic",    XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "user_settings_advanced", XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "user_delete",            XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "user_settings",          XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "groups_holder",          XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, FALSE, TRUE  },
	{ "group_new",              XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "group_delete",           XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "group_settings",         XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "group_settings_name_label", XST_WIDGET_MODE_SENSITIVE, XST_WIDGET_MODE_SENSITIVE, TRUE, TRUE  },
	{ "network_user_new",       XST_WIDGET_MODE_INSENSITIVE, XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "network_group_new",      XST_WIDGET_MODE_INSENSITIVE, XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "network_delete",         XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "network_settings",       XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "defs_container",         XST_WIDGET_MODE_INSENSITIVE, XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "showall",                XST_WIDGET_MODE_HIDDEN,      XST_WIDGET_MODE_SENSITIVE, FALSE, TRUE  },
	{ "user_passwd_optional",   XST_WIDGET_MODE_HIDDEN,      XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE },
	{ NULL }
};

static void
update_complexity (void)
{
	tables_set_state (tool->main_dialog->complexity == XST_DIALOG_ADVANCED);
}

static void
connect_signals (void)
{
	gtk_signal_connect (GTK_OBJECT (tool->main_dialog), "complexity_change",
					GTK_SIGNAL_FUNC (update_complexity),
					NULL);

	/* Stupid libglade converts user_data to strings */

	gtk_signal_connect (GTK_OBJECT (xst_dialog_get_widget (tool->main_dialog, "user_settings")),
					"clicked",
					GTK_SIGNAL_FUNC (on_settings_clicked),
					GINT_TO_POINTER (TABLE_USER));
	
	gtk_signal_connect (GTK_OBJECT (xst_dialog_get_widget (tool->main_dialog, "group_settings")),
					"clicked",
					GTK_SIGNAL_FUNC (on_settings_clicked),
					GINT_TO_POINTER (TABLE_GROUP));
	
	gtk_signal_connect (GTK_OBJECT (xst_dialog_get_widget (tool->main_dialog, "network_settings")),
					"clicked",
					GTK_SIGNAL_FUNC (on_settings_clicked),
					GINT_TO_POINTER (TABLE_NET_GROUP));

	xst_dialog_connect_signals (tool->main_dialog, signals);

	gtk_signal_connect (GTK_OBJECT (GTK_COMBO (xst_dialog_get_widget (tool->main_dialog,
									  "pro_name"))->entry),
			    "changed",
			    GTK_SIGNAL_FUNC (on_pro_name_changed),
			    NULL);
}

static void
config_clists (void)
{
	XstDialog *xd;
	gint i;
	gchar *lists[] = {"group_settings_all", "group_settings_members",
			  "user_settings_gall", "user_settings_gmember", NULL};

	xd = tool->main_dialog;

	for (i = 0; lists[i]; i++)
		gtk_clist_set_auto_sort (GTK_CLIST (xst_dialog_get_widget (xd, lists[i])), TRUE);
}

static ESearchBarItem user_search_menu_items[] = {
	{ N_("Show All"), 0 },
	{ NULL, -1}
};

static void
user_menu_activated (ESearchBar *esb, int id, gpointer user_data)
{
	switch (id)
	{
	case 0:
		user_query_string_set ("all");
		tables_update_content ();
		break;
	default:
		g_warning ("user_menu_activated: shouldn't be here.");
		break;
	}
}

enum {
	ESB_USER_NAME,
	ESB_USER_GID,
	ESB_GROUP_NAME,
};

static ESearchBarItem user_search_option_items[] = {
	{ N_("User name contains"), ESB_USER_NAME },
	{ N_("User GID contains"), ESB_USER_GID },
	{ N_("Group name contains"), ESB_GROUP_NAME },
	{ NULL, -1 }
};

static void
user_query_changed (ESearchBar *esb, gpointer user_data)
{
	gchar *search_word, *search_query;
	int search_type;

	gtk_object_get (GTK_OBJECT (esb),
			"text", &search_word,
			"option_choice", &search_type,
			NULL);

	if (search_word && strlen (search_word))
	{
		switch (search_type)
		{
		case ESB_USER_NAME:
			search_query = g_strdup_printf ("contains login %s",
							search_word);
			break;
		case ESB_USER_GID:
			search_query = g_strdup_printf ("contains gid %s",
							search_word);
			break;
		case ESB_GROUP_NAME:
			search_query = g_strdup_printf ("contains group %s",
							search_word);
			break;
		default:
			search_query = g_strdup ("all");
			break;
		}
	}

	else
		search_query = g_strdup ("all");
	
	user_query_string_set (search_query);
	tables_update_content ();
	
	g_free (search_query);
	g_free (search_word);
}

static void
create_searchbar (void)
{
	GtkWidget *table;
	ESearchBar *search;

	table = xst_dialog_get_widget (tool->main_dialog, "user_parent");
	
	search = E_SEARCH_BAR (e_search_bar_new (user_search_menu_items,
						 user_search_option_items));
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (search), 0, 1, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (GTK_WIDGET (search));
	gtk_signal_connect (GTK_OBJECT (search), "query_changed",
			    GTK_SIGNAL_FUNC (user_query_changed), 0);
	gtk_signal_connect (GTK_OBJECT (search), "menu_activated",
			    GTK_SIGNAL_FUNC (user_menu_activated), 0);
}

void
quit_cb (XstTool *tool, gpointer data)
{
	g_print("closing and cleaning\n");

	clear_all_tables ();
	destroy_tables ();
}

int
main (int argc, char *argv[])
{
	/* For random password generation. */

	srand (time (NULL));

	tool = xst_tool_init ("users", _("Users and Groups"), argc, argv, NULL);
	xst_tool_set_xml_funcs  (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
	xst_tool_set_close_func (tool, quit_cb, NULL);

	config_clists ();
	create_tables ();
	create_searchbar ();
	connect_signals ();

	xst_dialog_enable_complexity (tool->main_dialog);
	xst_dialog_set_widget_policies (tool->main_dialog, policies);

	xst_tool_main (tool);

	return 0;
}
