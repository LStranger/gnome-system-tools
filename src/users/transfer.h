/* transfer.h: this file is part of users-admin, a helix-setup-tool frontend 
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

#include <glade/glade.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>



typedef struct __logindefs _logindefs;

/* Just as specified in the @login_defs_prop_array in the users-conf backend: */

struct __logindefs
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
};

typedef struct _user user;

struct _user
{
	gchar *key;			/* Unique, data-independent field */
	gchar *login;			/* Login name */
	gchar *password;		/* Password */
	guint uid;			/* User id */
	guint gid;			/* Group id */
	gchar *comment;			/* Usually account owner's name */
	gchar *home;			/* Home directory */
	gchar *shell;			/* Account's shell */
	gint last_mod;			/* */
	gint passwd_max_life;		/* Password expiration time, 99999 if doesn't expire */
	gint passwd_exp_warn;		/* Number of days before users gets account exp. warning */
	gboolean passwd_exp_disable;	/* true if password doesn't expire */
	gboolean passwd_disable;	/* true if password is disabled */
	gchar *reserved;			/* Obscure field. Passed through */
	gboolean is_shadow;		/* true if using shadow passwords */
};

typedef struct _group group;

struct _group
{
	guint key;
	gchar *name;
	gchar *password;
	guint gid;
	GList *users;
};


extern user *current_user;
extern GList *user_list;

extern group *current_group;
extern GList *group_list;

extern _logindefs logindefs;

extern const gchar *user_list_data_key;
extern const gchar *group_list_data_key;

void transfer_config_saved(xmlNodePtr root);
void transfer_xml_to_gui(xmlNodePtr root);
void transfer_gui_to_xml(xmlNodePtr root);

