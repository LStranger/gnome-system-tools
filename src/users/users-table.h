/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* users-table.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>
 */

#ifndef _USERS_TABLE_H
#define _USERS_TABLE_H

enum {
	COL_USER_LOGIN,
	COL_USER_UID,
	COL_USER_HOME,
	COL_USER_SHELL,
	COL_USER_COMMENT,
	COL_USER_GROUP,
	COL_USER_POINTER,

	COL_USER_LAST
};

typedef struct 
{
	gchar *name;
	gboolean advanced_state_showable;
	gboolean basic_state_showable;
} TableConfig;

typedef struct UserTreeItem_ UserTreeItem;
	
struct UserTreeItem_
{
	const gchar *login;
	guint UID;
	const gchar *home;
	const gchar *shell;
	const gchar *comment;
	const gchar *group;
	
	UserTreeItem *children;
};

void	create_users_table		(void);
void	populate_users_table		(void);
void	update_users_table_complexity	(GstDialogComplexity);
void	users_table_update_content	(void);

#endif /* _USERS_TABLE_H */

