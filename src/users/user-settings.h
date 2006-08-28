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

gboolean        user_delete                      (GtkTreeModel *model,
						  GtkTreePath *path);
GtkWidget*      user_settings_dialog_new         (OobsUser *user);
gboolean        user_settings_dialog_user_is_new (void);
gint            user_settings_dialog_run         (GtkWidget *dialog,
						  OobsUser *user);

void            user_settings_dialog_get_data    (OobsUser *user);
void            user_settings_apply_profile      (GstUsersTool *tool, GstUserProfile *profile);


#endif /* USER_SETTINGS_H */
