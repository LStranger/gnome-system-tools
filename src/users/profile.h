/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* profile.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __PROFILE_H
#define __PROFILE_H

#include <gnome.h>
#include <gnome-xml/tree.h>

typedef struct
{
	gchar *name;
	
	guint pwd_maxdays;
	guint pwd_mindays;
	guint pwd_warndays;
	guint pwd_len;
	guint umin;
	guint umax;
	guint gmin;
	guint gmax;
	gchar *home_prefix;
	gchar *shell;
	gboolean logindefs;
} Profile;

typedef struct
{
	GtkCombo *name;
	GtkToggleButton *create_home;
	GnomeFileEntry *home_prefix;
	GtkCombo *shell;
	GtkSpinButton *umin;
	GtkSpinButton *umax;
	GtkSpinButton *gmin;
	GtkSpinButton *gmax;
	GtkSpinButton *pwd_maxdays;
	GtkSpinButton *pwd_mindays;
	GtkSpinButton *pwd_warndays;
	GtkSpinButton *pwd_len;
} ProfileTab;

typedef struct
{
	gchar *selected;
	GHashTable *hash;
} ProfileTable;

extern ProfileTable *profile_table;

void profile_save (gchar *name);
Profile *profile_get_default (void);
void profile_get_from_xml (xmlNodePtr root);
void profile_to_xml (xmlNodePtr root);
void profile_add (Profile *old_pf);
void profile_destroy (Profile *pf);

void profile_table_init (void);
void profile_table_destroy (void);
void profile_table_add_profile (Profile *pf, gboolean select);
void profile_table_del_profile (gchar *name);
Profile *profile_table_get_profile (gchar *name);
void profile_table_set_selected (gchar *name);

#endif /* PROFILE_H */
