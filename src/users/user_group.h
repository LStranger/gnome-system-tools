/* user_group.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __USER_GROUP_H
#define __USER_GROUP_H

#include <gnome.h>
#include <gnome-xml/tree.h>

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

xmlNodePtr get_current_root_node (void);
xmlNodePtr get_user_root_node (void);
xmlNodePtr get_group_root_node (void);
xmlNodePtr get_nis_group_root_node (void);
gchar *my_xml_element_get_content (xmlNodePtr node);
void adv_user_settings (xmlNodePtr node, gboolean show);
gboolean check_node_complexity (xmlNodePtr node);

/* Extern functions */
/* User related */

extern void user_new_prepare (xmlNodePtr node);
extern void user_settings_prepare (xmlNodePtr node);
extern gboolean user_update (xmlNodePtr node);
extern void user_passwd_dialog_prepare (xmlNodePtr node);
extern gboolean check_login_delete (xmlNodePtr node);

/* Group related externs. */

extern void group_new_prepare (xmlNodePtr node);
extern void group_settings_prepare (xmlNodePtr node);
extern gboolean group_update (xmlNodePtr node);
extern gboolean check_group_delete (xmlNodePtr node);

#endif /* USER_GROUP_H */

