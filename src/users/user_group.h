/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user_group.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Tambet Ingo <tambet@ximian.com> and 
 *          Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __USER_GROUP_H
#define __USER_GROUP_H

#include <gnome.h>

#include "gst.h"

#define IDMAX 100000

enum {
	NODE_USER,
	NODE_GROUP,
	NODE_DEFAULT,
	NODE_NET_GROUP,
	NODE_NET_USER,
	NODE_PROFILE
};

typedef struct {
	xmlNodePtr node;
	gboolean is_new;
	gint table;
} ug_data;

xmlNodePtr  get_root_node (gint tbl);
xmlNodePtr  get_db_node (xmlNodePtr node);
gboolean    check_node_visibility (xmlNodePtr node);
gchar *     my_gst_xml_element_get_content (xmlNodePtr node);

gboolean    get_min_max (xmlNodePtr db_node, gint *min, gint *max);
xmlNodePtr  get_corresp_field (xmlNodePtr node);
xmlNodePtr  get_node_by_data (xmlNodePtr dbnode, const gchar *field, const gchar *fdata);
GList *     my_g_list_remove_duplicates (GList *list1, GList *list2);
gchar *     find_new_id (xmlNodePtr parent, xmlNodePtr profile);
gchar *     find_new_key (xmlNodePtr parent);

/* Helpers */
gboolean    is_valid_name (const gchar*);
gboolean    is_valid_id (const gchar*);

void        user_query_string_set (gchar *str);
gchar *     user_query_string_get (void);

gint        my_strcmp (gconstpointer, gconstpointer);

void        show_error_message (gchar*,gchar*, gchar*);
void        combo_add_shells   (GtkWidget*);
void        combo_add_groups   (GtkWidget*, gboolean);
void        combo_add_profiles (GtkWidget*);

#endif /* USER_GROUP_H */

