/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-group-xml.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es> and
 *          Tambet Ingo <tambet@ximian.com>
 */

#ifndef __USER_GROUP_XML_H
#define __USER_GROUP_XML_H

#include <gnome.h>
#include "user-settings.h"

gchar    *generic_value_string         (xmlNodePtr node, const gchar *name);
gint      generic_value_integer        (xmlNodePtr node, const gchar *name);
gpointer  generic_value_string_peek    (xmlNodePtr parent, const gchar *name);
void      generic_set_value_string     (xmlNodePtr node, const gchar *name, const gchar *value);
void      generic_set_value_integer    (xmlNodePtr node, const gchar *name, gint value);
gchar    *user_value_group             (xmlNodePtr user_node);
gpointer  user_value_group_peek        (xmlNodePtr user_node);
void      user_set_value_group         (xmlNodePtr user_node, const gchar *value);
void      user_set_groups              (xmlNodePtr user_node, GSList *list);
gchar   **user_value_comment_array     (xmlNodePtr node);
void      user_set_value_comment_array (xmlNodePtr node, gchar **comment);


#define user_value_login(node)       (generic_value_string (node, "login"))
#define user_value_home(node)        (generic_value_string (node, "home"))
#define user_value_shell(node)       (generic_value_string (node, "shell"))
#define user_value_comment(node)     (generic_value_string (node, "comment"))
#define user_value_uid_string(node)  (generic_value_string (node, "uid"))
#define user_value_uid_integer(node) (generic_value_integer (node, "uid"))
#define user_value_gid_string(node)  (generic_value_string (node, "gid"))
#define user_value_gid_integer(node) (generic_value_integer (node, "gid"))
#define user_value_pwd_maxdays(node) (generic_value_integer (node, "passwd_max_life"))
#define user_value_pwd_mindays(node) (generic_value_integer (node, "passwd_min_life"))
#define user_value_pwd_warndays(node) (generic_value_integer (node, "passwd_exp_warn"))
#define user_value_password(node)    (generic_value_string (node, "password"))

#define user_value_login_peek(node)      (generic_value_string_peek (node, "login"))
#define user_value_home_peek(node)       (generic_value_string_peek (node, "home"))
#define user_value_shell_peek(node)      (generic_value_string_peek (node, "shell"))
#define user_value_comment_peek(node)    (generic_value_string_peek (node, "comment"))
#define user_value_uid_string_peek(node) (generic_value_string_peek (node, "uid"))
#define user_value_gid_string_peek(node) (generic_value_string_peek (node, "gid"))
#define user_value_password_peek(node)   (generic_value_string_peek (node, "password"))

#define group_value_name(node)           (generic_value_string (node, "name"))
#define group_value_gid_string(node)     (generic_value_string (node, "gid"))
#define group_value_gid_integer(node)    (generic_value_integer (node, "gid"))

#define group_value_name_peek(node)      (generic_value_string_peek (node, "name"))
#define group_value_gid_string_peek(node) (generic_value_string_peek (node, "gid"))

#define user_set_value_login(node,value)       (generic_set_value_string (node, "login", value))
#define user_set_value_home(node,value)        (generic_set_value_string (node, "home", value))
#define user_set_value_shell(node,value)       (generic_set_value_string (node, "shell", value))
#define user_set_value_comment(node,value)     (generic_set_value_string (node, "comment", value))
#define user_set_value_uid_string(node,value)  (generic_set_value_string (node, "uid", value))
#define user_set_value_uid_integer(node,value) (generic_set_value_integer (node, "uid", value))
#define user_set_value_gid_string(node,value)  (generic_set_value_string (node, "gid", value))
#define user_set_value_gid_integer(node,value) (generic_set_value_integer (node, "gid", value))
#define user_set_value_pwd_maxdays(node,value) (generic_set_value_integer (node, "passwd_max_life", value))
#define user_set_value_pwd_mindays(node,value) (generic_set_value_integer (node, "passwd_min_life", value))
#define user_set_value_pwd_warndays(node,value) (generic_set_value_integer (node, "passwd_exp_warn", value))


gboolean group_set_value_name (GstDialog *xd, xmlNodePtr node, const gchar *value);
gboolean group_set_value_gid (GstDialog *xd, xmlNodePtr node, const gchar *value);

gboolean	node_exists		(xmlNodePtr, const gchar*, const gchar*);

xmlNodePtr	group_add_blank_xml 	(xmlNodePtr);
void		group_update_xml	(xmlNodePtr, gchar*, gchar*, GList*);

xmlNodePtr	user_add_blank_xml  	(xmlNodePtr);
void		user_update_xml		(xmlNodePtr, UserAccountData*, gboolean);
void		del_user_groups		(xmlNodePtr);

GList *		get_group_users		(xmlNodePtr);
GList *		get_list_from_node	(gchar*, gint);

gchar*		group_xml_get_gid	(xmlNodePtr, gchar*);

gchar*		get_group_name		(gchar *);

#endif /* __USER_GROUP_XML_H */
