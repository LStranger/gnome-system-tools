/* e-table.h: this file is part of users-admin, a ximian-setup-tool frontend 
 * for boot administration.
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

#ifndef __E_TABLE_H
#define __E_TABLE_H

#include <gnome.h>
#include <gal/e-table/e-table-simple.h>

enum {
	COL_DEFAULT,
	COL_TYPE,
	COL_LABEL,
	COL_IMAGE,
	COL_DEV,

	COL_LAST,
};

extern void create_table (xmlNodePtr root);
extern void boot_table_update_state (void);

void *boot_value_default (xmlNodePtr node);
void *boot_value_label (xmlNodePtr node);
void *boot_value_type (xmlNodePtr node);
void *boot_value_image (xmlNodePtr node);
void *boot_value_dev (xmlNodePtr node);
void *boot_value_root (xmlNodePtr node);

void boot_value_set_default (xmlNodePtr node);
void boot_value_set_label (xmlNodePtr node, gchar *val);
void boot_value_set_type (xmlNodePtr node, gchar *val);
void boot_value_set_image (xmlNodePtr node, gchar *val);
void boot_value_set_dev (xmlNodePtr node, gchar *val);
void boot_value_set_root (xmlNodePtr node, gchar *val);

xmlNodePtr get_selected_node (void);
void boot_table_delete (void);
void boot_table_update (void);
xmlNodePtr boot_table_add (void);

#endif /* E_TABLE_H */
