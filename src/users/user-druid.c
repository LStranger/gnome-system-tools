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
#include "user_settings.h"
#include "passwd.h"

extern XstTool *tool;

static void user_druid_class_init (UserDruidClass *class);
static void user_druid_finalize   (GtkObject *obj);

static GtkWindowClass *parent_class;

GtkType
user_druid_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo type_info = {
			"UserDruid",
			sizeof (UserDruid),
			sizeof (UserDruidClass),
			(GtkClassInitFunc) user_druid_class_init,
			(GtkObjectInitFunc) NULL,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		type = gtk_type_unique (gtk_window_get_type (), &type_info);
	}

	return type;
}

static void
user_druid_class_init (UserDruidClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;
	parent_class = gtk_type_class (gtk_window_get_type ());

	/* override methods */
	object_class->finalize = user_druid_finalize;
}

static void
user_druid_finalize (GtkObject *obj)
{
	UserDruid *druid = (UserDruid *) obj;

	user_account_gui_destroy (druid->gui);
        ((GtkObjectClass *)(parent_class))->finalize (obj);
}


static void
druid_exit (UserDruid *druid)
{
	gtk_widget_destroy (GTK_WIDGET (druid));
}

static void
druid_cancel (GtkWidget *w, gpointer data)
{
	druid_exit ((UserDruid *) data);
}

/* Identity Page */
static void
identity_check (UserDruid *druid)
{
	gchar *login;

	login = gtk_entry_get_text (druid->gui->name);
	if (strlen (login) > 0)
		gnome_druid_set_buttons_sensitive (druid->druid, TRUE, TRUE, TRUE);
	else
		gnome_druid_set_buttons_sensitive (druid->druid, TRUE, FALSE, TRUE);
}

static void
identity_changed (GtkWidget *widget, gpointer data)
{
	UserDruid *druid = data;
	
	identity_check (druid);
}

static void
identity_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	UserDruid *config = data;

	gtk_widget_grab_focus (GTK_WIDGET (config->gui->name));
	identity_check (config);
}

static gboolean
identity_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	UserDruid *config = data;
	gchar *login, *buf;

	login = gtk_entry_get_text (config->gui->name);
	if ((buf = check_user_login (config->gui->account->node, login))) {
		user_account_gui_error (GTK_WINDOW (config->gui->top), buf);
		return TRUE;
	} else
		return FALSE;
}

/* Password Page */
static void
password_check (UserDruid *druid)
{
	gchar *pwd1, *pwd2;
	gboolean quality;

	pwd1 = gtk_entry_get_text (druid->gui->pwd1);
	pwd2 = gtk_entry_get_text (druid->gui->pwd2);
	quality = gtk_toggle_button_get_active (druid->gui->quality);
	
	if (strlen (pwd1) > 0 && !strcmp (pwd1, pwd2))
		gnome_druid_set_buttons_sensitive (druid->druid, TRUE, TRUE, TRUE);
	else
		gnome_druid_set_buttons_sensitive (druid->druid, TRUE, FALSE, TRUE);
}

static void
password_changed (GtkWidget *widget, gpointer data)
{
	UserDruid *druid = data;
	
	password_check (druid);
}

static void
password_prepare (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	UserDruid *config = data;

	gtk_widget_grab_focus (GTK_WIDGET (config->gui->pwd1));
	password_check (config);
}

static gboolean
password_next (GnomeDruidPage *page, GnomeDruid *druid, gpointer data)
{
	gchar *pwd1, *pwd2, *error;
	gboolean quality;
	UserDruid *config = data;

	pwd1 = gtk_entry_get_text (config->gui->pwd1);
	pwd2 = gtk_entry_get_text (config->gui->pwd2);
	quality = gtk_toggle_button_get_active (config->gui->quality);
	
	if ((error = passwd_check (pwd1, pwd2, quality))) {
		user_account_gui_error (GTK_WINDOW (config->gui->top), error);
		return TRUE;
	} else
		return FALSE;
}

/* Common stuff */
static void
druid_entry_activate (GtkWidget *w, gpointer data)
{
	GtkWidget *widget = NULL;
	UserDruid *druid = data;
	
	if (GTK_WIDGET_MAPPED (druid->druid->next))
		widget = druid->druid->next;
	if (GTK_WIDGET_MAPPED (druid->druid->finish))
		widget = druid->druid->finish;

	if (widget)
		gtk_widget_grab_focus (widget);
}

static void
druid_finish (GnomeDruidPage *druid_page, GnomeDruid *druid, gpointer data)
{
	UserDruid *config = data;

	user_account_gui_save (config->gui);
	user_account_save (config->gui->account);
	druid_exit (config);
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

static gboolean
construct (UserDruid *druid)
{
	GtkWidget *widget;
	UserAccount *account;
	int i;

	account = user_account_new (NULL);
	if (!account)
		return FALSE;

	druid->gui = user_account_gui_new (account, GTK_WIDGET (druid));

	/* get our toplevel widget and reparent it */
	widget = glade_xml_get_widget (druid->gui->xml, "user_druid_druid");
	gtk_widget_reparent (widget, GTK_WIDGET (druid));

	druid->druid = GNOME_DRUID (widget);

	/* set window title */
	gtk_window_set_title (GTK_WINDOW (druid), _("User Account Wizard"));
	gtk_window_set_policy (GTK_WINDOW (druid), FALSE, TRUE, FALSE);
	gtk_window_set_modal (GTK_WINDOW (druid), TRUE);
	gtk_object_set (GTK_OBJECT (druid), "type", GTK_WINDOW_DIALOG, NULL);

	/* attach to druid page signals */
	for (i = 0; pages[i].name != NULL; i++) {
		GtkWidget *page;

		page = glade_xml_get_widget (druid->gui->xml, pages[i].name);

		if (pages[i].next_func)
			gtk_signal_connect (GTK_OBJECT (page), "next",
					    pages[i].next_func, druid);
		if (pages[i].prepare_func)
			gtk_signal_connect (GTK_OBJECT (page), "prepare",
					    pages[i].prepare_func, druid);
		if (pages[i].back_func)
			gtk_signal_connect (GTK_OBJECT (page), "back",
					    pages[i].back_func, druid);
		if (pages[i].finish_func)
			gtk_signal_connect (GTK_OBJECT (page), "finish",
					    pages[i].finish_func, druid);
	}
	gtk_signal_connect (GTK_OBJECT (druid->druid), "cancel", druid_cancel, druid);
	
	user_account_gui_setup (druid->gui, NULL);
	
	gtk_signal_connect (GTK_OBJECT (druid->gui->name), "changed", identity_changed, druid);
	gtk_signal_connect (GTK_OBJECT (druid->gui->hphone), "activate",
			    GTK_SIGNAL_FUNC (druid_entry_activate), druid);
	
	gtk_signal_connect (GTK_OBJECT (druid->gui->pwd2), "changed", password_changed, druid);
	gtk_signal_connect (GTK_OBJECT (druid->gui->pwd2), "activate", druid_entry_activate, druid);

	return TRUE;
}

UserDruid *
user_druid_new (void)
{
	UserDruid *new;

	new = (UserDruid *) gtk_type_new (user_druid_get_type ());

	if (construct (new))
		return new;
	else {
		gtk_widget_destroy (GTK_WIDGET (new));
		return NULL;
	}
}
