/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>
 */

#ifndef __USER_GROUP_XML_H
#define __USER_GROUP_XML_H

#include <gnome.h>
#include <gnome-xml/tree.h>

gchar *generic_value_string  (xmlNodePtr node, const gchar *name);
gint   generic_value_integer (xmlNodePtr node, const gchar *name);
gchar *user_value_group      (xmlNodePtr user_node);

#define user_value_login(node)       (generic_value_string (node, "login"))
#define user_value_home(node)        (generic_value_string (node, "home"))
#define user_value_shell(node)       (generic_value_string (node, "shell"))
#define user_value_comment(node)     (generic_value_string (node, "comment"))
#define user_value_uid_string(node)  (generic_value_string (node, "uid"))
#define user_value_uid_integer(node) (generic_value_integer (node, "uid"))
#define user_value_gid_string(node)  (generic_value_string (node, "gid"))
#define user_value_gid_integer(node) (generic_value_integer (node, "gid"))

#define group_value_name(node)       (generic_value_string (node, "name"))
#define group_value_gid_string(node) (generic_value_string (node, "gid"))
#define group_value_gid_integer(node) (generic_value_integer (node, "gid"))

#endif /* __USER_GROP_XML_H */
