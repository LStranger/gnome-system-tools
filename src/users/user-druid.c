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
 */

#include <config.h>

#include <string.h>
#include <ctype.h>

#include "user-druid.h"
#include "profile.h"
#include "user-group-xml.h"
#include "user_group.h"
#include "passwd.h"

extern XstTool *tool;

static void
druid_exit (UserSettings *us)
{
	gtk_widget_destroy (us->dialog);
	gtk_object_unref (GTK_OBJECT (us->xml));

	g_free (us->basic);
	g_free (us->group);
	g_free (us->pwd);
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

	druid = GNOME_DRUID (glade_xml_get_widget (us->xml, "user_druid_druid"));
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

	druid = GNOME_DRUID (glade_xml_get_widget (us->xml, "user_druid_druid"));
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

	gtk_widget_grab_focus (glade_xml_get_widget (us->xml, "user_druid_pwd_entry1"));
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
	UserSettings *us = data;
	
	w0 = glade_xml_get_widget (us->xml, "user_druid_druid");
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

		page = GNOME_DRUID_PAGE (glade_xml_get_widget (us->xml, pages[i].name));

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
	druid = GNOME_DRUID (glade_xml_get_widget (us->xml, "user_druid_druid"));
	gtk_signal_connect (GTK_OBJECT (druid), "cancel", druid_cancel, us);

	gtk_signal_connect (GTK_OBJECT (us->basic->name), "changed", identity_changed, us);
	gtk_signal_connect (GTK_OBJECT (us->basic->name), "activate", identity_login_activate, us);
	gtk_signal_connect (GTK_OBJECT (us->basic->comment), "activate", druid_entry_activate, us);
	
	gtk_signal_connect (GTK_OBJECT (us->pwd->pwd1), "activate", password_activate, us);
	gtk_signal_connect (GTK_OBJECT (us->pwd->pwd2), "changed", password_changed, us);
	gtk_signal_connect (GTK_OBJECT (us->pwd->pwd2), "activate", druid_entry_activate, us);
}

void
user_druid_run (xmlNodePtr user_node)
{
	UserSettings *us;

	us = g_new0 (UserSettings, 1);
	us->xml = glade_xml_new (tool->glade_path, NULL);
	
	us->dialog = glade_xml_get_widget (us->xml, "user_druid_window");
	us->node  = user_node;
	us->table = TABLE_USER;

	us->basic = g_new0 (UserSettingsBasic, 1);
	us->basic->name    = GTK_ENTRY (glade_xml_get_widget (us->xml, "user_druid_login"));
	us->basic->comment = GTK_ENTRY (glade_xml_get_widget (us->xml, "user_druid_comment"));
	us->basic->home    = GTK_ENTRY (glade_xml_get_widget (us->xml, "user_settings_home"));
	us->basic->shell   = GTK_COMBO (glade_xml_get_widget (us->xml, "user_settings_shell"));
	us->basic->uid     = GTK_SPIN_BUTTON (glade_xml_get_widget (us->xml, "user_settings_uid"));

	us->group = g_new (UserSettingsGroup, 1);
	us->group->main    = GTK_COMBO (glade_xml_get_widget (us->xml, "user_settings_group"));
	us->group->all     = GTK_CLIST (glade_xml_get_widget (us->xml, "user_settings_gall"));
	us->group->member  = GTK_CLIST (glade_xml_get_widget (us->xml, "user_settings_gmember"));

	us->pwd = g_new0 (UserSettingsPwd, 1);
	us->pwd->quality = GTK_TOGGLE_BUTTON (glade_xml_get_widget (us->xml, "user_druid_pwd_check"));
	us->pwd->pwd1 = GTK_ENTRY (glade_xml_get_widget (us->xml, "user_druid_pwd_entry1"));
	us->pwd->pwd2 = GTK_ENTRY (glade_xml_get_widget (us->xml, "user_druid_pwd_entry2"));
	us->pwd->min  = GTK_SPIN_BUTTON (glade_xml_get_widget (us->xml, "user_passwd_min"));
	us->pwd->max  = GTK_SPIN_BUTTON (glade_xml_get_widget (us->xml, "user_passwd_max"));
	us->pwd->days = GTK_SPIN_BUTTON (glade_xml_get_widget (us->xml, "user_passwd_days"));
	
	us->new = TRUE;

	user_settings_basic_fill (us);
	user_settings_group_fill (us);
	user_settings_pwd_fill   (us);

	connect_signals (us);

	gtk_widget_show (GTK_WIDGET (us->dialog));	
}
