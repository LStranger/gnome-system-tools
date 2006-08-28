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
	{ NULL }};

static const GstWidgetPolicy policies[] = {
	/* Name                     Basic                        Advanced                   Root   User */
	{ "user_new",               GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "user_delete",            GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "user_settings",          GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "groups_table",           GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, FALSE, TRUE  },
	{ "group_new",              GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "group_delete",           GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "group_settings",         GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "profile_new",            GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "profile_delete",         GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "profile_settings",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ NULL }
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

	gst_dialog_set_widget_policies (tool->main_dialog, policies);
	gst_dialog_connect_signals (tool->main_dialog, signals);
	main_window_prepare (GST_USERS_TOOL (tool));

	gtk_widget_show (GTK_WIDGET (tool->main_dialog));
	gtk_main ();
	
	return 0;
}
