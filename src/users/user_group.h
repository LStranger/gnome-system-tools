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

#include "xst.h"

#define IDMAX 100000

typedef struct
{
	xmlNodePtr node;
	gboolean new;
	gint table;
} ug_data;

typedef struct {
	xmlNodePtr node;
	gboolean new;
	
	gchar *name;
	gchar **comment;
	gchar *uid;
	gchar *group;
	gchar *home;
	gchar *shell;
	gchar *password;

	gint pwd_maxdays;
	gint pwd_mindays;
	gint pwd_warndays;

	GSList *extra_groups;
} UserAccount;


UserAccount *user_account_new              (const gchar *profile);
UserAccount *user_account_get_by_node      (xmlNodePtr node);
void         user_account_save             (UserAccount *account);
gchar       *user_account_check            (UserAccount *account);
GList       *user_account_check_warnings   (UserAccount *account);
void         user_account_destroy_warnings (GList *warnings);
void         user_account_destroy          (UserAccount *account);


xmlNodePtr get_root_node (gint tbl);
xmlNodePtr get_user_root_node (void);
xmlNodePtr get_group_root_node (void);
xmlNodePtr get_nis_group_root_node (void);
xmlNodePtr get_nis_user_root_node (void);
xmlNodePtr get_db_node (xmlNodePtr node);
gchar *my_xst_xml_element_get_content (xmlNodePtr node);
gboolean check_node_complexity (xmlNodePtr node);

gboolean  check_user_root (xmlNodePtr node, const gchar *field, const gchar *value);
gchar    *check_user_login (xmlNodePtr node, const gchar *login);
gchar    *check_user_uid (xmlNodePtr node, const gchar *val);
gchar    *check_user_shell (xmlNodePtr node, const gchar *val);

gboolean check_group_name (GtkWindow *xd, xmlNodePtr node, const gchar *name);
gboolean check_group_gid (GtkWindow *xd, xmlNodePtr node, const gchar *val);
gboolean get_min_max (xmlNodePtr db_node, gint *min, gint *max);
xmlNodePtr get_corresp_field (xmlNodePtr node);
xmlNodePtr get_node_by_data (xmlNodePtr dbnode, const gchar *field, const gchar *fdata);
GList *get_user_list (gchar *field, xmlNodePtr group_node);
GList *my_g_list_remove_duplicates (GList *list1, GList *list2);
gchar *find_new_id (xmlNodePtr parent);
gchar *find_new_key (xmlNodePtr parent);

/* User related */

void settings_prepare (ug_data *ud);
void user_new_prepare (ug_data *ud);
void user_passwd_dialog_prepare (xmlNodePtr node);
gboolean check_login_delete (xmlNodePtr node);

/* Group related */

void group_new_prepare (ug_data *ud);
void group_add (UserAccount *account, const gchar *group_name);
gboolean group_update (ug_data *ud);
gboolean check_group_delete (xmlNodePtr node);

/* Helpers */
void my_gtk_clist_append_items (GtkCList *list, GList *items);
gint my_gtk_clist_append (GtkCList *list, gchar *text);
void user_query_string_set (gchar *str);
gchar *user_query_string_get (void);

#endif /* USER_GROUP_H */

