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

typedef struct _UserSettingsBasic UserSettingsBasic;
typedef struct _UserSettingsGroup UserSettingsGroup;
typedef struct _UserSettingsPwd   UserSettingsPwd;
typedef struct _UserSettings      UserSettings;

struct _UserSettingsBasic
{
	GtkEntry *name;
	GtkEntry *comment;
	GtkEntry *home;
	GtkEntry *shell;
	GtkSpinButton *uid;
	GtkWidget *advanced;
};

struct _UserSettingsGroup
{
	GtkCombo  *main;
	GtkCList  *all;
	GtkCList  *member;
	GtkWidget *add;
	GtkWidget *remove;
	GtkWidget *set_primary;
};

struct _UserSettingsPwd
{
	GtkToggleButton *quality;
	GtkWidget *optional;
	GtkSpinButton *min;
	GtkSpinButton *max;
	GtkSpinButton *days;
};

struct _UserSettings
{
	GnomeDialog       *dialog;
	UserSettingsBasic *basic;
	UserSettingsGroup *group;
	UserSettingsPwd   *pwd;
	xmlNodePtr         node;
	gboolean           new;
	gint               table;
};


void user_settings_prepare (xmlNodePtr user_node);
void user_settings_destroy (UserSettings *us);
void user_settings_helper (UserSettings *us);

#endif /* USER_SETTINGS_H */
