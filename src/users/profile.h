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

BEGIN_GNOME_DECLS

#define PROFILE_DIALOG "profile_dialog"

typedef struct
{
	gchar *name;
	gchar *comment;
	
	guint pwd_maxdays;
	guint pwd_mindays;
	guint pwd_warndays;
	guint umin;
	guint umax;
	guint gmin;
	guint gmax;
	gchar *home_prefix;
	gchar *shell;
	gchar *group;
	gboolean pwd_random;
	gboolean logindefs;
} Profile;

typedef struct
{
	gchar *selected;
	GHashTable *hash;
} ProfileTable;

extern ProfileTable *profile_table;

void profile_table_run    (void);

void     profile_save    (gchar *name);
Profile *profile_add     (Profile *old_pf, const gchar *new_name, gboolean select); 
void     profile_destroy (Profile *pf);


void profile_table_init    (void);
void profile_table_destroy (void);


void profile_table_from_xml (xmlNodePtr root);
void profile_table_to_xml   (xmlNodePtr root);

void     profile_table_add_profile (Profile *pf, gboolean select);
gboolean profile_table_del_profile (gchar *name);

Profile *profile_table_get_profile  (const gchar *name);
void     profile_table_set_selected (const gchar *name);
GSList  *profile_table_get_list     (void);

gboolean validate_var (gchar *var);

END_GNOME_DECLS

#endif /* PROFILE_H */
