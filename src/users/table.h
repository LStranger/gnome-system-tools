/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>
 */

#ifndef __TABLE_H
#define __TABLE_H

/* User and group lists creation function */
void                    create_gtk_tree_list		(GtkWidget*, GtkTargetEntry);
void			populate_gtk_tree_list		(GtkTreeView*, GList*);
GList*                  get_gtk_tree_list_items         (GtkTreeView*);
GtkWidget*              popup_menu_create               (GtkWidget*);

/* User and group tables manipulation functions */
void			create_tables			(void);
void			update_tables_complexity	(GstDialogComplexity);
void			populate_all_tables		(void);
xmlNodePtr		get_selected_row_node		(gint);

#endif /* __TABLE_H */
