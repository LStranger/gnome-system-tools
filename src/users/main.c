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

XstTool *tool;

static XstDialogSignal signals[] = {
	{ "user_settings_dialog",        "delete_event",         on_user_settings_dialog_delete_event },
	{ "user_settings_dialog",        "show",                 on_user_settings_dialog_show },
	{ "user_settings_ok",            "clicked",              on_user_settings_ok_clicked },
	{ "user_settings_cancel",        "clicked",              on_user_settings_cancel_clicked },
	{ "user_passwd_dialog",          "delete_event",         on_user_passwd_dialog_delete_event },
	{ "user_passwd_ok",              "clicked",              on_user_passwd_ok_clicked },
	{ "user_passwd_cancel",          "clicked",              on_user_passwd_cancel_clicked },
	{ "user_passwd_random",          "clicked",              on_user_passwd_random_clicked },
	{ "group_settings_dialog",       "delete_event",         on_group_settings_dialog_delete_event },
	{ "group_settings_dialog",       "show",                 on_group_settings_dialog_show },
	{ "group_settings_ok",           "clicked",              on_group_settings_ok_clicked },
	{ "group_settings_cancel",       "clicked",              on_group_settings_cancel_clicked },
	{ "group_settings_add",          "clicked",              on_group_settings_add_clicked },
	{ "group_settings_remove",       "clicked",              on_group_settings_remove_clicked },
	{ "group_settings_members",      "select_row",           on_group_settings_members_select_row },
	/* { "group_settings_members",      "unselect_row",         on_group_settings_members_unselect_row }, */
	{ "group_settings_all",          "select_row",           on_group_settings_all_select_row },
	/* { "group_settings_all",          "unselect_row",         on_group_settings_all_unselect_row }, */
	{ "user_new",                    "clicked",              on_user_new_clicked },
	{ "user_delete",                 "clicked",              on_user_delete_clicked },
	{ "user_chpasswd",               "clicked",              on_user_chpasswd_clicked },
	{ "group_new",                   "clicked",              on_group_new_clicked },
	{ "group_delete",                "clicked",              on_group_delete_clicked },
	{ "network_user_new",            "clicked",              on_network_user_new_clicked },
	{ "network_group_new",           "clicked",              on_network_group_new_clicked },
	{ "network_delete",              "clicked",              on_network_delete_clicked },
	{ NULL }
};

static void
set_access_sensitivity (void)
{
	char *access_no[] = {"user_new", "user_chpasswd", "group_new",
					 "user_settings_basic", "user_settings_advanced",
					 "group_settings_name_label", "group_settings_add",
					 "group_settings_remove", "defs_min_uid", "defs_max_uid",
					 "defs_min_gid", "defs_max_gid", "defs_passwd_max_days",
					 "defs_passwd_min_days", "defs_passwd_warn",
					 "defs_passwd_min_len", "defs_mail_dir",
					 "defs_create_home", "network_user_new", "network_group_new",
					 NULL};
	
	char *access_yes[] = {"users_holder", "groups_holder", NULL};
	char *unsensitive[] = {"user_delete", "user_settings", "user_chpasswd", "group_delete",
					   "group_settings", "network_delete", "network_settings", NULL};
	int i;

	/* Those widgets that won't be available if you don't have the access. */
	for (i = 0; access_no[i]; i++)
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, access_no[i]),
							 xst_tool_get_access(tool));

	/* Those widgets that will be available, even if you don't have the access. */
	for (i = 0; access_yes[i]; i++)
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, access_yes[i]),
							 TRUE);

	/* Those widgets you should never have access to, and will be activated later on. */
	for (i = 0; unsensitive[i]; i++)
		gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, unsensitive[i]),
							 FALSE);
}

static void
update_complexity (void)
{
	XstDialogComplexity complexity;

	complexity = tool->main_dialog->complexity;
	
	tables_set_state (complexity == XST_DIALOG_ADVANCED);
	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "defs_container"),
				  complexity == XST_DIALOG_ADVANCED);

	gtk_widget_set_sensitive (xst_dialog_get_widget (tool->main_dialog, "network_container"),
						 complexity == XST_DIALOG_ADVANCED);

	actions_set_sensitive (FALSE);

	clear_all_tables ();
	populate_all_tables ();
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
}

int
main (int argc, char *argv[])
{
	/* For random password generation. */

	srand (time (NULL));

	tool = xst_tool_init ("users", _("Users and Groups"), argc, argv);
	xst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);

	create_tables ();
	connect_signals ();

	xst_dialog_enable_complexity (tool->main_dialog);

	set_access_sensitivity ();

	xst_tool_main (tool);
	
	return 0;
}
