/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* profiless-table.h: this file is part of users-admin, a gnome-system-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>
 */

#ifndef _PROFILES_TABLE_H
#define _PROFILES_TABLE_H

enum {
	   COL_PROFILE_NAME,
	   COL_PROFILE_COMMENT,
	   COL_PROFILE_POINTER,

	   COL_PROFILE_LAST
};

typedef struct 
{
	gchar *name;
	gboolean advanced_state_showable;
	gboolean basic_state_showable;
} ProfilesTableConfig;

void   create_profiles_table          (void);
void   populate_profiles_table        (void);
void   profiles_table_update_content  (void);

#endif /* _PROFILES_TABLE_H */
