/* e-table.h: this file is part of users-admin, a helix-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000 Helix Code, Inc.
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
 * Authors: Tambet Ingo <tambeti@sa.ee> and Arturo Espinosa <arturo@helixcode.com>.
 */

#include <gnome-xml/tree.h>

#define USER 1
#define GROUP 2

/* Functions for both tables */

void e_table_create (void);
void e_table_state (gboolean state);
xmlNodePtr e_table_get_table_data (gchar get);

/* User table functions. */

xmlNodePtr e_table_get_current_user (void);
void e_table_del_user (xmlNodePtr node);
void e_table_change_user (void);
void e_table_add_user (void);

/* Group table functions. */

xmlNodePtr e_table_get_current_group (void);
void e_table_del_group (xmlNodePtr node);
void e_table_change_group (void);
void e_table_add_group (void);

