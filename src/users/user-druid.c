/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001 Ximian, Inc.
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
 *
 * Based on ximian-setup-tools/src/network/ppp-druid.c
 */

#include <config.h>

#include <string.h>
#include <ctype.h>

#include "user-druid.h"
#include "profile.h"
#include "user-group-xml.h"
#include "user_group.h"
#include "passwd.h"

#define USER_DRUID_MAX_PAGE 4

extern XstTool *tool;

static void
user_druid_exit (UserSettings *us)
{
	/* TODO: free everything */
	gtk_widget_hide (us->dialog);
	g_free (us);
}

static void
user_druid_cancel (GtkWidget *w, gpointer data)
{
	user_druid_exit ((UserSettings *) data);
}

static void
user_druid_entry_activate (GtkWidget *w, gpointer data)
{
	GtkWidget *w0, *w1;
	
	w0 = xst_dialog_get_widget (tool->main_dialog, "user_druid_druid");
	w1 = NULL;
	
	if (GTK_WIDGET_MAPPED (GNOME_DRUID (w0)->next))
		w1 = GNOME_DRUID (w0)->next;
	if (GTK_WIDGET_MAPPED (GNOME_DRUID (w0)->finish))
		w1 = GNOME_DRUID (w0)->finish;

	if (w1)
		gtk_widget_grab_focus (w1);
}

static void
user_druid_login_activate (GtkWidget *w, gpointer data)
{
	UserSettings *us = data;

	gtk_widget_grab_focus (GTK_WIDGET (us->basic->comment));
}

static void
user_druid_passwd_activate (GtkWidget *w, gpointer data)
{
	UserSettings *us = data;

	gtk_widget_grab_focus (GTK_WIDGET (us->pwd->pwd2));
}

static void
user_druid_page_finish (GnomeDruidPage *druid_page, GtkWidget *druid, gpointer data)
{
	UserSettings *us = data;

	user_update (us);
	user_druid_exit (us);
}

static void
connect_signals (UserSettings *us)
{
	gint i;
	XstDialog *xd;
	XstDialogSignal signals[] = {
		{ "user_druid_window",    "delete_event", user_druid_cancel },
		{ "user_druid_druid",     "cancel",       user_druid_cancel },
		{ "user_druid_login",     "activate",     user_druid_login_activate },
		{ "user_druid_comment",   "activate",     user_druid_entry_activate },
		{ "user_druid_pwd_entry1","activate",     user_druid_passwd_activate },
		{ "user_druid_pwd_entry2","activate",     user_druid_entry_activate },
		{ "user_druid_page_last", "finish",       user_druid_page_finish },
		{ NULL }
	};

	xd = tool->main_dialog;
	
	for (i = 0; signals[i].widget; i++)
		gtk_signal_connect_after (GTK_OBJECT (xst_dialog_get_widget (xd, signals[i].widget)),
							 signals[i].signal_name, signals[i].func, us);
}

static UserSettingsBasic *
user_druid_basic_prepare (void)
{
	UserSettingsBasic *ub;
	XstDialog *xd;

	xd = tool->main_dialog;
	ub = g_new0 (UserSettingsBasic, 1);

	ub->name    = GTK_ENTRY (xst_dialog_get_widget (xd, "user_druid_login"));
	ub->comment = GTK_ENTRY (xst_dialog_get_widget (xd, "user_druid_comment"));

	ub->home    = GTK_ENTRY (xst_dialog_get_widget (xd, "user_settings_home"));
	ub->shell   = GTK_COMBO (xst_dialog_get_widget (xd, "user_settings_shell"));
	ub->uid     = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_settings_uid"));

	return ub;
}

static UserSettingsGroup *
user_druid_group_prepare (void)
{
	UserSettingsGroup *ug;
	XstDialog *xd;

	xd = tool->main_dialog;
	ug = g_new (UserSettingsGroup, 1);

	ug->main    = GTK_COMBO (xst_dialog_get_widget (xd, "user_settings_group"));
	ug->all     = GTK_CLIST (xst_dialog_get_widget (xd, "user_settings_gall"));
	ug->member  = GTK_CLIST (xst_dialog_get_widget (xd, "user_settings_gmember"));

	return ug;
}

static UserSettingsPwd *
user_druid_pwd_prepare (void)
{
	UserSettingsPwd *pw;
	XstDialog *xd;

	xd = tool->main_dialog;
	pw = g_new0 (UserSettingsPwd, 1);

	pw->quality = GTK_TOGGLE_BUTTON (xst_dialog_get_widget (xd, "user_druid_pwd_check"));
	pw->pwd1 = GTK_ENTRY (xst_dialog_get_widget (xd, "user_druid_pwd_entry1"));
	pw->pwd2 = GTK_ENTRY (xst_dialog_get_widget (xd, "user_druid_pwd_entry2"));

	pw->min  = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_passwd_min"));
	pw->max  = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_passwd_max"));
	pw->days = GTK_SPIN_BUTTON (xst_dialog_get_widget (xd, "user_passwd_days"));
	
	return pw;
}

static void
user_druid_clear (UserSettings *us)
{
	gtk_entry_set_text (us->basic->name, "");
	gtk_entry_set_text (us->basic->comment, "");
	gtk_entry_set_text (us->pwd->pwd1, "");
	gtk_entry_set_text (us->pwd->pwd2, "");

	gnome_druid_set_page (GNOME_DRUID (xst_dialog_get_widget (tool->main_dialog, "user_druid_druid")),
					  GNOME_DRUID_PAGE (xst_dialog_get_widget (tool->main_dialog, "user_druid_page0")));
}

void
user_druid_run (xmlNodePtr user_node)
{
	UserSettings *us;
	static gboolean first_time = TRUE;

	us = g_new0 (UserSettings, 1);
	
	us->dialog = xst_dialog_get_widget (tool->main_dialog, "user_druid_window");
	us->node  = user_node;
	us->table = TABLE_USER;
	us->basic = user_druid_basic_prepare ();
	us->group = user_druid_group_prepare ();
	us->pwd   = user_druid_pwd_prepare ();
	us->new = TRUE;

	user_settings_basic_fill (us);
	user_settings_group_fill (us);
	user_settings_pwd_fill   (us);

	if (first_time) {
		connect_signals (us);
		first_time = FALSE;
	} else
		user_druid_clear (us);

	gtk_widget_show (GTK_WIDGET (us->dialog));
	
}
