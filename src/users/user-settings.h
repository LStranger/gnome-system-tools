/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* user-settings.h: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */

#ifndef __USER_SETTINGS_H
#define __USER_SETTINGS_H

#include <gnome.h>

#include "user_group.h"

typedef struct {
	gchar *login;
	gchar *uid;
	gchar *gid;
		
	gchar *name;
	gchar *location;
	gchar *work_phone;
	gchar *home_phone;
		
	gchar *group;
	gchar *home;
	gchar *shell;
	gchar *password1;
	gchar *password2;
		
	gchar *pwd_maxdays;
	gchar *pwd_mindays;
	gchar *pwd_warndays;
		
	GList *extra_groups;
} UserAccountData;

void		user_new_prepare		(ug_data*);
void		user_settings_dialog_close	(void);
gboolean	user_update 			(ug_data*);
void		user_set_profile		(const gchar*);
gboolean	delete_user			(xmlNodePtr);
void		user_settings_prepare		(ug_data*);

#endif /* USER_SETTINGS_H */
