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

#include "users-tool.h"

enum {
	COL_USER_FACE,
	COL_USER_NAME,
	COL_USER_LOGIN,
	COL_USER_HOME,
	COL_USER_ID,
	COL_USER_MEMBER, /* used in group members dialog */
	COL_USER_OBJECT,
	COL_USER_ITER,
	COL_USER_LAST
};

void	create_users_table	(GstUsersTool *tool);
void    users_table_set_user    (OobsUser     *user,
				 OobsListIter *list_iter,
				 GtkTreeIter  *iter);
void    users_table_add_user    (OobsUser     *user,
				 OobsListIter *list_iter);

#endif /* _USERS_TABLE_H */

