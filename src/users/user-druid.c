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
druid_exit (UserSettings *us)
{
	/* TODO: free everything */
	gtk_widget_hide (us->dialog);
	g_free (us);
}

static void
druid_cancel (GtkWidget *w, gpointer data)
{
	druid_exit ((UserSettings *) data);
}

/* Identity Page */
static void
identity_check (UserSettings *us)
{
	GnomeDruid *druid;
	gchar *login;

	druid = GNOME_DRUID (xst_dialog_get_widget (tool->main_dialog, "user_druid_druid"));
	login = gtk_entry_get_text (us->basic->name);
	if (strlen (login) > 0 && check_user_login (GTK_WINDOW (us->dialog), us->node, login))
		gnome_druid_set_buttons_sensitive (druid, TRUE, TRUE, TRUE);
	else
		gnome_druid_set_buttons_sensitive (druid, TRUE, FALSE, TRUE);
}

static void
identity_changed (GtkWidget *widget, gpointer data)
{
	UserSettings *us = data;
	
	identity_check (us);
}

static void
identity_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	UserSettings *us = data;

	gtk_widget_grab_focus (GTK_WIDGET (us->basic->name));
	identity_check (us);
}

static gboolean
identity_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	/* go to the next page */
	return FALSE;
}

static void
identity_login_activate (GtkWidget *w, gpointer data)
{
	UserSettings *us = data;

	gtk_widget_grab_focus (GTK_WIDGET (us->basic->comment));
}

/* Password Page */
static void
password_check (UserSettings *us)
{
	GnomeDruid *druid;
	gchar *pwd1, *pwd2;
	gboolean quality;

	druid = GNOME_DRUID (xst_dialog_get_widget (tool->main_dialog, "user_druid_druid"));
	pwd1 = gtk_entry_get_text (us->pwd->pwd1);
	pwd2 = gtk_entry_get_text (us->pwd->pwd2);
	quality = gtk_toggle_button_get_active (us->pwd->quality);
	
	if (strlen (pwd1) > 0 && !strcmp (pwd1, pwd2))
		gnome_druid_set_buttons_sensitive (druid, TRUE, TRUE, TRUE);
	else
		gnome_druid_set_buttons_sensitive (druid, TRUE, FALSE, TRUE);
}

static void
password_changed (GtkWidget *widget, gpointer data)
{
	UserSettings *us = data;
	
	password_check (us);
}

static void
password_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	UserSettings *us = data;

	gtk_widget_grab_focus (xst_dialog_get_widget (tool->main_dialog, "user_druid_pwd_entry1"));
	password_check (us);
}

static gboolean
password_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	gchar *pwd1, *pwd2;
	gboolean quality;
	UserSettings *us = data;

	pwd1 = gtk_entry_get_text (us->pwd->pwd1);
	pwd2 = gtk_entry_get_text (us->pwd->pwd2);
	quality = gtk_toggle_button_get_active (us->pwd->quality);
	
	if (strlen (pwd1) > 0 && passwd_check (NULL, pwd1, pwd2, quality))
		return FALSE;
	else
		return TRUE;
}

static void
password_activate (GtkWidget *w, gpointer data)
{
	UserSettings *us = data;

	gtk_widget_grab_focus (GTK_WIDGET (us->pwd->pwd2));
}

/* Common stuff */
static void
druid_entry_activate (GtkWidget *w, gpointer data)
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
druid_finish (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	UserSettings *us = data;

	user_update (us);
	druid_exit (us);
}

static struct {
	gchar *name;
	GtkSignalFunc next_func;
	GtkSignalFunc prepare_func;
	GtkSignalFunc back_func;
	GtkSignalFunc finish_func;
} pages[] = {
	{ "user_druidStartPage",
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL) },
	{ "user_druidIdentityPage",
	  GTK_SIGNAL_FUNC (identity_next),
	  GTK_SIGNAL_FUNC (identity_prepare),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL) },
	{ "user_druidPasswordPage",
	  GTK_SIGNAL_FUNC (password_next),
	  GTK_SIGNAL_FUNC (password_prepare),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL) },
	{ "user_druidFinishPage",
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (druid_finish) },
	{ NULL,
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL),
	  GTK_SIGNAL_FUNC (NULL) }
};

static void
connect_signals (UserSettings *us)
{
	gint i;
	GnomeDruid *druid;
	
	for (i = 0; pages[i].name != NULL; i++) {
		GnomeDruidPage *page;

		page = GNOME_DRUID_PAGE (xst_dialog_get_widget (tool->main_dialog, pages[i].name));

		if (pages[i].next_func)
			gtk_signal_connect (GTK_OBJECT (page), "next",
					    pages[i].next_func, us);
		if (pages[i].prepare_func)
			gtk_signal_connect (GTK_OBJECT (page), "prepare",
					    pages[i].prepare_func, us);
		if (pages[i].back_func)
			gtk_signal_connect (GTK_OBJECT (page), "back",
					    pages[i].back_func, us);
		if (pages[i].finish_func)
			gtk_signal_connect (GTK_OBJECT (page), "finish",
					    pages[i].finish_func, us);
	}
	druid = GNOME_DRUID (xst_dialog_get_widget (tool->main_dialog, "user_druid_druid"));
	gtk_signal_connect (GTK_OBJECT (druid), "cancel", druid_cancel, us);

	gtk_signal_connect (GTK_OBJECT (us->basic->name), "changed", identity_changed, us);
	gtk_signal_connect (GTK_OBJECT (us->basic->name), "activate", identity_login_activate, us);
	gtk_signal_connect (GTK_OBJECT (us->basic->comment), "activate", druid_entry_activate, us);
	
	gtk_signal_connect (GTK_OBJECT (us->pwd->pwd1), "activate", password_activate, us);
	gtk_signal_connect (GTK_OBJECT (us->pwd->pwd2), "changed", password_changed, us);
	gtk_signal_connect (GTK_OBJECT (us->pwd->pwd2), "activate", druid_entry_activate, us);
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
