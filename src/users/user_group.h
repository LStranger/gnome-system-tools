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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __USER_GROUP_H
#define __USER_GROUP_H

#include <gnome.h>
#include <gnome-xml/tree.h>

#include "user_settings.h"

/* Just as specified in the @login_defs_prop_array in the users-conf backend: */
typedef struct
{
        gchar *qmail_dir;
        gchar *mailbox_dir;
        gchar *mailbox_file;
        gint passwd_max_day_use;
        gint passwd_min_day_use;
        guint passwd_min_length;
        guint passwd_warning_advance_days;
        guint new_user_min_id;
        guint new_user_max_id;
        guint new_group_min_id;
        guint new_group_max_id;
        gchar *del_user_additional_command;
        gboolean create_home;
} login_defs;

extern login_defs logindefs;

typedef struct
{
	xmlNodePtr node;
	gboolean new;
	gint table;
} ug_data;

xmlNodePtr get_root_node (gint tbl);
xmlNodePtr get_user_root_node (void);
xmlNodePtr get_group_root_node (void);
xmlNodePtr get_nis_group_root_node (void);
xmlNodePtr get_nis_user_root_node (void);
xmlNodePtr get_db_node (xmlNodePtr node);
gchar *my_xst_xml_element_get_content (xmlNodePtr node);
void adv_user_settings (xmlNodePtr node, gboolean show);
gboolean check_node_complexity (xmlNodePtr node);
gboolean check_user_login (xmlNodePtr node, gchar *login);
gboolean check_user_uid (xmlNodePtr node, gchar *val);
gboolean check_user_comment (xmlNodePtr, gchar *val);
gint check_user_group (xmlNodePtr node);
gboolean check_user_home (xmlNodePtr node, gchar *val);
gboolean check_user_shell (xmlNodePtr node, gchar *val);
gboolean check_group_name (xmlNodePtr node, gchar *name);
gboolean check_group_gid (xmlNodePtr node, gchar *val);
gboolean get_min_max (xmlNodePtr db_node, gint *min, gint *max);
xmlNodePtr get_corresp_field (xmlNodePtr node);
xmlNodePtr get_node_by_data (xmlNodePtr dbnode, gchar *field, gchar *fdata);
GList *get_user_list (gchar *field, xmlNodePtr group_node);
GList *my_g_list_remove_duplicates (GList *list1, GList *list2);
gchar *find_new_id (xmlNodePtr parent);
gchar *find_new_key (xmlNodePtr parent);

/* Extern functions */
/* User related */

extern void settings_prepare (ug_data *ud);
extern void user_new_prepare (ug_data *ud);
extern gboolean user_update (UserSettings *us);
extern void user_passwd_dialog_prepare (xmlNodePtr node);
extern gboolean check_login_delete (xmlNodePtr node);

/* Group related externs. */

extern void group_new_prepare (ug_data *ud);
extern gboolean group_update (ug_data *ud);
extern gboolean check_group_delete (xmlNodePtr node);

/* Helpers */
void my_gtk_clist_append_items (GtkCList *list, GList *items);
gint my_gtk_clist_append (GtkCList *list, gchar *text);

#endif /* USER_GROUP_H */

