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


#include <config.h>
#include "gst.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "table.h"
#include "callbacks.h"

GstTool *tool;

void quit_cb (GstTool *tool, gpointer data);

static GstDialogSignal signals[] = {
	/* User settings dialog callbacks */
	{ "user_settings_name",                 "changed",              G_CALLBACK (on_user_settings_login_changed) },
	{ "user_passwd_manual",			"toggled",		G_CALLBACK (on_user_settings_passwd_toggled) },
	{ "user_passwd_random",			"toggled",		G_CALLBACK (on_user_settings_passwd_toggled) },
	{ "user_passwd_random_new",		"clicked",		G_CALLBACK (on_user_settings_passwd_random_new) },
	{ "user_settings_passwd1",		"changed",		G_CALLBACK (on_user_settings_passwd_changed) },
	{ "user_settings_profile_menu",         "changed",              G_CALLBACK (on_user_settings_profile_changed) },

	/* Main dialog callbacks, users tab */
	{ "user_new",				"clicked",		G_CALLBACK (on_user_new_clicked) },
	{ "user_settings",             		"clicked",       	G_CALLBACK (on_user_settings_clicked) },
	{ "user_delete",                	"clicked",       	G_CALLBACK (on_user_delete_clicked) },
	{ "manage_groups",                      "clicked",              G_CALLBACK (on_manage_groups_clicked) },
	
	/* Main dialog callbacks, groups tab */
	{ "group_new",				"clicked",		G_CALLBACK (on_group_new_clicked) },
	{ "group_settings",			"clicked",		G_CALLBACK (on_group_settings_clicked) },
	{ "group_delete",			"clicked",       	G_CALLBACK (on_group_delete_clicked) },
	{ "groups_dialog_help",                 "clicked",              G_CALLBACK (on_groups_dialog_show_help) },
	{ NULL }};

static const gchar *policy_widgets [] = {
	"user_new",
	"user_delete",
	"groups_table",
	"group_new",
	"group_delete",
	"group_settings",
	"profile_new",
	"profile_delete",
	"profile_settings",
	"user_settings_name",
	"user_settings_profile_menu",
	"user_privileges",
	"user_settings_home",
	"user_settings_shell",
	"user_settings_uid",
	"user_passwd_max",
	"user_passwd_min",
	"user_passwd_days",
	"user_settings_group",
	NULL
};

static void
main_window_prepare (GstUsersTool *tool)
{
	create_tables (tool);

	/* For random password generation. */
	srand (time (NULL));

	/* This sucks, but calculating the needed size for simple mode based on the
	 * hidden widgets plus the tabs size is going to be ugly. Chema
	 */
	gtk_window_set_default_size (GTK_WINDOW (GST_TOOL (tool)->main_dialog), 450, 300);
}

int
main (int argc, char *argv[])
{
	gst_init_tool ("users-admin", argc, argv, NULL);
	tool = GST_TOOL (gst_users_tool_new ());

	gst_dialog_require_authentication_for_widgets (tool->main_dialog, policy_widgets);
	gst_dialog_connect_signals (tool->main_dialog, signals);
	main_window_prepare (GST_USERS_TOOL (tool));

	gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	gtk_main ();
	
	return 0;
}
