/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-table.h: this file is part of users-admin, a ximian-setup-tool frontend 
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

#ifndef __E_TABLE_H
#define __E_TABLE_H

#include <gnome.h>
#include <gal/e-table/e-tree-simple.h>

#include "user_group.h"

#define COLOR_NORMAL "black"

enum {
	COL_USER_LOGIN,
	COL_USER_UID,
	COL_USER_HOME,
	COL_USER_SHELL,
	COL_USER_COMMENT,
	COL_USER_GROUP,

	COL_USER_LAST,

	/* Invisible columns */
	COL_USER_COLOR,
};

enum {
	COL_GROUP_NAME,
	COL_GROUP_GID,

	COL_GROUP_LAST,

	/* Invisible columns */
	COL_GROUP_COLOR,
};

enum {
	TABLE_USER,
	TABLE_GROUP,
	TABLE_DEFAULT,
	TABLE_NET_GROUP,
	TABLE_NET_USER,
};

void clear_table (ETreeModel *model, ETreePath *root_path);
void clear_all_tables (void);
void populate_table (ETreeModel *model, ETreePath *root_path, xmlNodePtr root_node);
void populate_all_tables (void);
extern guint create_tables (void);
void tables_update_content (void);
void tables_set_state (gboolean state);
xmlNodePtr get_selected_node (void);
gboolean delete_selected_node (gint tbl);
void current_table_update_row (ug_data *ud);
void current_table_new_row (ug_data *ud);
void set_active_table (guint tbl);


#endif /* E_TABLE_H */

