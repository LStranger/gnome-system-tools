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

typedef enum
{
	USER_GROUP_TYPE_USER,
	USER_GROUP_TYPE_GROUP
} UserGroupType;

typedef struct
{
	UserGroupType type;
	gchar *key; /* Unique, data-independent field */
	gchar *name; /* Login or group name */
	gchar *password;
	guint id; /* UID or GID */
} user_group;

typedef struct
{
	user_group ug;
	guint gid;			/* Group id */
	gchar *comment;			/* Usually account owner's name */
	gchar *home;			/* Home directory */
	gchar *shell;			/* Account's shell */
	
	guint last_mod;			/* Days since Jan 1, 1970 that password was last changed */
	guint passwd_min_life;		/* Days before password may be changed */
	guint passwd_max_life;		/* Days after which password must be changed */
	guint passwd_exp_warn;		/* Days before password is to expire that user is warned */
	guint passwd_exp_disable;	/* Days after password expires that account is disabled */
	gboolean is_passwd_exp_disable; /* Is this field being used? */
	guint passwd_disable;	/* Days since Jan 1, 1970 that account is disabled */
	gboolean is_passwd_disable; /* Is this field being used? */
	gchar *reserved;			/* Obscure field. Passed through */
	gboolean is_shadow;		/* true if using shadow passwords */
} user;

typedef struct
{
	user_group ug;
	GList *users;
} group;


extern GList *user_basic_list;
extern GList *user_adv_list;
extern GList *group_basic_list;
extern GList *group_adv_list;

extern void user_group_free (user_group *ug);
extern gboolean user_group_is_system (user_group *ug);

extern user *user_new (gchar *name);
extern void user_free (user *u);
extern gboolean user_add (void);
extern gboolean user_update (void);
extern void user_fill_settings_group (GtkCombo *combo);
extern GList *user_current_list (void);

extern group *group_new (void);
extern void group_free (group *u);
extern gboolean group_add (void);
extern gboolean group_update (void);
extern GList *group_fill_members_list (void);
extern void group_fill_all_users_list (GList *member_rows);
extern GList *group_current_list (void);
extern GList *get_group_list (gchar *field, gboolean adv);
extern gchar *get_group_by_data (gchar *field, gchar *fdata, gchar *data);


#endif /* USER_GROUP_H */
