/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.h: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Tambet Ingo <tambet@ximian.com> and 
 *          Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gnome.h>
#include "xst.h"

/* Main dialog general callbacks */
void on_showall_toggled (GtkToggleButton*, gpointer);
void on_table_clicked (GtkTreeSelection*, gpointer);

/* Main dialog callbacks, users tab */
void on_user_new_clicked (GtkButton *button, gpointer user_data);
void on_user_settings_clicked (GtkButton *button, gpointer data);
void on_user_delete_clicked (GtkButton *button, gpointer user_data);
void on_profile_settings_dialog_clicked (GtkButton *, gpointer);

/* Main dialog callbacks, groups tab */
void on_group_new_clicked (GtkButton*, gpointer);
void on_group_settings_clicked (GtkButton*, gpointer);
void on_group_delete_clicked (GtkButton*, gpointer);

#ifdef NIS
void on_network_delete_clicked (GtkWidget *button, gpointer user_data);
void on_network_user_new_clicked (GtkButton *button, gpointer user_data);
void on_network_group_new_clicked (GtkButton *button, gpointer user_data);
#endif


/* user settings dialog callbacks */
void on_user_settings_dialog_show (GtkWidget*, gpointer);
void on_user_settings_dialog_delete_event (GtkWidget*, gpointer);
void on_user_settings_ok_clicked (GtkButton*, gpointer);
void on_user_settings_passwd_random_new (GtkButton*, gpointer);
void on_user_settings_passwd_toggled (GtkToggleButton*, gpointer);
void on_user_settings_profile_changed (GtkWidget*, gpointer);
void on_user_settings_passwd_changed (GtkEntry*, gpointer);

/* group settings dialog callbacks */
void on_group_settings_dialog_show (GtkWidget*, gpointer);
void on_group_settings_dialog_delete_event (GtkWidget*, gpointer);
void on_group_settings_ok_clicked (GtkButton*, gpointer);

/* Profile settings dialog callbacks */
void on_profile_settings_dialog_delete_event (GtkWidget*, gpointer);
void on_profile_settings_ok_clicked (GtkButton*, gpointer);

/* profiles dialog callbacks */
void on_profile_new_clicked (GtkButton*, gpointer);
void on_profile_settings_clicked (GtkButton*, gpointer);
void on_profile_delete_clicked (GtkButton *, gpointer);

/* general callbacks */
void on_add_remove_button_clicked (GtkButton *button, gpointer user_data);
void on_list_select_row (GtkTreeSelection*, gpointer);

/* Helpers */
void actions_set_sensitive (gint table, gboolean state);


#endif /* CALLBACKS_H */
