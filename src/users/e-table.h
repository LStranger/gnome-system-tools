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

void clear_table (ETreeModel *model, ETreePath *root_path);
void clear_all_tables (void);
void populate_table (ETreeModel *model, ETreePath *root_path, xmlNodePtr root_node);
void populate_all_tables (void);
extern void create_tables (void);
void tables_set_state (gboolean state);
xmlNodePtr get_selected_node (void);
gboolean delete_selected_node (void);
void current_table_update_row (void);
void current_table_new_row (xmlNodePtr node);


#endif /* E_TABLE_H */

