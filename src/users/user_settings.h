/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user_settings.h: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

#ifndef __USER_SETTINGS_H
#define __USER_SETTINGS_H

#include <gnome.h>
#include <gnome-xml/tree.h>

#include "user_group.h"

typedef struct {
	GtkWidget       *top;
	UserAccount     *account;
	GladeXML        *xml;

	/* Basic settings frame */
	GtkWidget       *basic_frame;
	GtkEntry        *name;
	GtkEntry        *comment;

	/* Contact inforamtion frame */
	GtkWidget       *contact_frame;
	GtkEntry        *office;
	GtkEntry        *wphone;
	GtkEntry        *hphone;

	/* Advanced settings frame */
	GtkWidget       *advanced;
	GtkEntry        *home;
	GtkCombo        *shell;
	GtkSpinButton   *uid;

	/* Profile selection box */
	GtkWidget       *profile_box;
	GtkOptionMenu   *profile_menu;

	/* Groups box */
	GtkWidget       *group_box;
	GtkWidget       *group_extra;
	GtkCombo        *group;
	GtkCList        *all;
	GtkCList        *member;
	GtkWidget       *add;
	GtkWidget       *remove;

	/* Password box */
	GtkWidget       *pwd_box;
	GtkNotebook     *pwd_notebook;
	GtkToggleButton *pwd_manual;
	GtkToggleButton *pwd_random;
	GtkLabel        *pwd_random_label;
	GtkWidget       *pwd_random_new;
	GtkWidget       *pwd_frame;
	GtkToggleButton *quality;
	GtkEntry        *pwd1;
	GtkEntry        *pwd2;

	/* Optional settings frame */
	GtkWidget       *optional;
	GtkSpinButton   *min;
	GtkSpinButton   *max;
	GtkSpinButton   *days;
} UserAccountGui;

UserAccountGui *user_account_gui_new     (UserAccount *account, GtkWidget *parent);
void            user_account_gui_setup   (UserAccountGui *gui, GtkWidget *top);
gboolean        user_account_gui_save    (UserAccountGui *gui);
void            user_account_gui_error   (GtkWindow *parent, gchar *error);
void            user_account_gui_destroy (UserAccountGui *gui);

#endif /* USER_SETTINGS_H */
