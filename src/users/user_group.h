/* user_group.h: this file is part of users-admin, a helix-setup-tool frontend 
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

#ifndef __USER_GROUP_H
#define __USER_GROUP_H

#include <gnome.h>
#include <gnome-xml/tree.h>


#define LOCAL 1

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

extern gboolean user_add (gchar type);
extern gboolean user_update (xmlNodePtr node);
extern void user_fill_settings_group (GtkCombo *combo, gboolean adv);
extern GList *user_current_list (void);

extern gboolean group_add (gchar type);
extern gboolean group_update (xmlNodePtr node);
gchar *find_new_id (gchar from);
extern gchar *find_new_key (gchar from);
gboolean is_free_uid (gint new_uid);
extern GList *group_fill_members_list (xmlNodePtr node);
extern void group_fill_all_users_list (GList *member_rows);
extern GList *get_group_list (gchar *field, gboolean adv);
extern GList *get_user_list (gchar *field, gboolean adv);

gchar *get_group_by_data (gchar *field, gchar *fdata, gchar *data);
int basic_user_count (xmlNodePtr parent);
xmlNodePtr basic_user_find_nth (xmlNodePtr parent, int n);
int basic_group_count (xmlNodePtr parent);
xmlNodePtr basic_group_find_nth (xmlNodePtr parent, int n);

void adv_user_settings (xmlNodePtr node, gboolean show);
void adv_user_settings_new (void);
void adv_user_settings_update (xmlNodePtr node, gchar *login);

gchar *my_xml_get_content (xmlNodePtr parent, gchar *name);
GList *get_group_users (xmlNodePtr node);
void del_group_users (xmlNodePtr node);
void add_group_users (xmlNodePtr node, gchar *name);

extern void group_settings_prepare (xmlNodePtr node);
extern void user_settings_prepare (xmlNodePtr node);
extern void user_new_prepare (gchar *group_name);
extern void group_new_prepare (void);
void my_xml_set_child_content (xmlNodePtr parent, gchar *name, gchar *val);
xmlNodePtr group_add_to_xml (gchar *name, gchar type);
xmlNodePtr user_add_to_xml (gchar *name, gchar type);

#endif /* USER_GROUP_H */
