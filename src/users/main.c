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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Tambet Ingo <tambet@ximian.com> and 
 *          Arturo Espinosa <arturo@ximian.com>.
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glade/glade.h>

#include <time.h>
#include <stdlib.h>

#include "transfer.h"
#include "table.h"
#include "callbacks.h"
#include "search-bar/search-bar.h"
#include "gst.h"

GstTool *tool;

void quit_cb (GstTool *tool, gpointer data);

static GstDialogSignal signals[] = {
	{ "showall",                     	"toggled",       	G_CALLBACK (on_showall_toggled) },
	
	/* User settings dialog callbacks */
	{ "user_settings_dialog",		"delete_event",		G_CALLBACK (on_user_settings_dialog_delete_event) },
	{ "user_settings_dialog",		"show",			G_CALLBACK (on_user_settings_dialog_show) },
	{ "user_settings_ok",			"clicked",		G_CALLBACK (on_user_settings_ok_clicked) },
	{ "user_settings_cancel",		"clicked",		G_CALLBACK (on_user_settings_dialog_delete_event) },
	{ "user_settings_profile_menu",         "changed",              G_CALLBACK (on_user_settings_profile_changed) },
	{ "user_passwd_manual",			"toggled",		G_CALLBACK (on_user_settings_passwd_toggled) },
	{ "user_passwd_random",			"toggled",		G_CALLBACK (on_user_settings_passwd_toggled) },
	{ "user_passwd_random_new",		"clicked",		G_CALLBACK (on_user_settings_passwd_random_new) },
	{ "user_passwd_entry1",			"changed",		G_CALLBACK (on_user_settings_passwd_changed) },
	{ "user_passwd_entry2",			"changed",		G_CALLBACK (on_user_settings_passwd_changed) },
	{ "user_settings_profile_button",	"clicked",		G_CALLBACK (on_profile_settings_users_dialog_clicked) },
	{ "user_settings_help",                 "clicked",              G_CALLBACK (on_user_settings_show_help) },
	
	/* Group settings dialog callbacks */
	{ "group_settings_dialog",		"delete_event",  	G_CALLBACK (on_group_settings_dialog_delete_event) },
	{ "group_settings_dialog",		"show",          	G_CALLBACK (on_group_settings_dialog_show) },
	{ "group_settings_ok",			"clicked",       	G_CALLBACK (on_group_settings_ok_clicked) },
	{ "group_settings_cancel",		"clicked",       	G_CALLBACK (on_group_settings_dialog_delete_event) },
	{ "group_settings_add",			"clicked",       	G_CALLBACK (on_add_remove_button_clicked) },
	{ "group_settings_remove",		"clicked",       	G_CALLBACK (on_add_remove_button_clicked) },
	{ "group_settings_help",                "clicked",              G_CALLBACK (on_group_settings_show_help) },

	/* Profile settings dialog callbacks */
	{ "profile_settings_dialog",            "delete_event",         G_CALLBACK (on_profile_settings_dialog_delete_event) },
	{ "profile_settings_cancel",            "clicked",              G_CALLBACK (on_profile_settings_dialog_delete_event) },
	{ "profile_settings_ok",                "clicked",              G_CALLBACK (on_profile_settings_ok_clicked) },
	{ "profile_settings_help",              "clicked",              G_CALLBACK (on_profile_settings_show_help) },

	/* Profile dialog callbacks */
	{ "profile_new",                        "clicked",              G_CALLBACK (on_profile_new_clicked) },
	{ "profile_settings",                   "clicked",              G_CALLBACK (on_profile_settings_clicked) },
	{ "profile_delete",                     "clicked",              G_CALLBACK (on_profile_delete_clicked) },
	{ "profiles_dialog_help",               "clicked",              G_CALLBACK (on_profile_settings_show_help) },

	/* Main dialog callbacks, users tab */
	{ "user_new",				"clicked",		G_CALLBACK (on_user_new_clicked) },
	{ "user_settings",             		"clicked",       	G_CALLBACK (on_user_settings_clicked) },
	{ "user_delete",                	"clicked",       	G_CALLBACK (on_user_delete_clicked) },
	
	/* Main dialog callbacks, groups tab */
	{ "group_new",				"clicked",		G_CALLBACK (on_group_new_clicked) },
	{ "group_settings",			"clicked",		G_CALLBACK (on_group_settings_clicked) },
	{ "group_delete",			"clicked",       	G_CALLBACK (on_group_delete_clicked) },
	{ NULL }};

static const GstWidgetPolicy policies[] = {
	/* Name                     Basic                        Advanced                   Root   User */
	{ "user_new",               GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
/*	{ "user_settings_advanced", GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },*/
	{ "user_delete",            GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "user_settings",          GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "groups_table",           GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, FALSE, TRUE  },
	{ "group_new",              GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "group_delete",           GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "group_settings",         GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "profile_new",            GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "profile_delete",         GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "profile_settings",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "showall",                GST_WIDGET_MODE_HIDDEN,      GST_WIDGET_MODE_SENSITIVE, FALSE, TRUE  },
	{ NULL }
};

enum {
	SB_USER_NAME,
	SB_USER_UID,
	SB_GROUP_NAME,
	SB_USER_GID,
};

static SearchBarItem user_search_option_items[] = {
	{ N_("User name contains"), SB_USER_NAME },
	{ N_("User ID is"), SB_USER_UID },
	{ N_("Group name contains"), SB_GROUP_NAME },
	{ N_("User GID is"), SB_USER_GID },
	{ NULL, -1 }
};

static void
user_query_changed (SearchBar *esb, gpointer user_data)
{
	gchar *search_word, *search_query;
	int search_type;

	g_object_get (G_OBJECT (esb),
	              "text", &search_word,
	              "option_choice", &search_type,
	              NULL);

	if (search_word && strlen (search_word)) {
		switch (search_type) {
		case SB_USER_NAME:
			search_query = g_strdup_printf ("contains login %s",
							search_word);
			break;
		case SB_USER_UID:
			search_query = g_strdup_printf ("is uid %s",
							search_word);
			break;
		case SB_USER_GID:
			search_query = g_strdup_printf ("is gid %s",
							search_word);
			break;
		case SB_GROUP_NAME:
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
update_toggle ()
{
	GtkWidget *toggle = gst_dialog_get_widget (tool->main_dialog, "showall");
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gst_conf_get_boolean (tool, "showall"));
}


static void
create_searchbar (void)
{
	GtkWidget *table;
	SearchBar *search;

	table = gst_dialog_get_widget (tool->main_dialog, "user_parent");

	search = SEARCH_BAR (search_bar_new (user_search_option_items));
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (search), 0, 1, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);

	g_signal_connect (G_OBJECT (search), "query_changed",
			  G_CALLBACK (user_query_changed), 0);
	g_object_set_data (G_OBJECT (tool->main_dialog), "SearchBar",
			   (gpointer) search);

	gtk_widget_show_all (GTK_WIDGET (search));
}

static void
create_button_sizegroup (void)
{
	GtkWidget *users_buttonvbox, *groups_buttonvbox;
	GtkSizeGroup *button_sizegroup;

	button_sizegroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	
	users_buttonvbox = gst_dialog_get_widget (tool->main_dialog,
						  "users_buttonvbox");

	groups_buttonvbox = gst_dialog_get_widget (tool->main_dialog,
						   "groups_buttonvbox");

	gtk_size_group_add_widget (button_sizegroup, users_buttonvbox);
	gtk_size_group_add_widget (button_sizegroup, groups_buttonvbox);
}

static void
main_window_prepare (void)
{
	/* For random password generation. */
	srand (time (NULL));

	create_tables ();
	create_searchbar ();

	create_button_sizegroup ();
	update_toggle ();

	/* This sucks, but calculating the needed size for simple mode based on the
	 * hidden widgets plus the tabs size is going to be ugly. Chema
	 */
	gtk_window_set_default_size (GTK_WINDOW (tool->main_dialog), 550, 400);
}

int
main (int argc, char *argv[])
{
	gst_init ("users-admin", argc, argv, NULL);
	tool = gst_tool_new ();
	gst_tool_construct (tool, "users", _("Users and Groups"));

	gst_tool_set_xml_funcs  (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);

	gst_dialog_set_widget_policies (tool->main_dialog, policies);

	main_window_prepare ();
	gst_dialog_connect_signals (tool->main_dialog, signals);

	gst_tool_main (tool, FALSE);
	
	return 0;
}
