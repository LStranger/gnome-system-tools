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
 * Authors: Tambet Ingo <tambet@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gnome.h>
#include <gnome-xml/tree.h>

void on_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
			      guint page_num, gpointer user_data);

void on_showall_toggled (GtkToggleButton *toggle, gpointer user_data);
void on_settings_clicked (GtkButton *button, gpointer user_data);

void on_user_chpasswd_clicked (GtkButton *button, gpointer user_data);
void on_user_new_clicked (GtkButton *button, gpointer user_data);
void on_user_delete_clicked (GtkButton *button, gpointer user_data);

void on_group_new_clicked (GtkButton *button, gpointer user_data);
void on_group_delete_clicked (GtkButton *button, gpointer user_data);

void on_network_delete_clicked (GtkWidget *button, gpointer user_data);
void on_network_user_new_clicked (GtkButton *button, gpointer user_data);
void on_network_group_new_clicked (GtkButton *button, gpointer user_data);

void on_pro_name_changed (GtkEditable *editable, gpointer user_data);
void on_pro_del_clicked (GtkButton *button, gpointer user_data);
void on_pro_save_clicked (GtkButton *button, gpointer user_data);
void on_pro_new_clicked (GtkButton *button, gpointer user_data);
void on_pro_copy_clicked (GtkButton *button, gpointer user_data);

void on_user_settings_dialog_show (GtkWidget *button, gpointer user_data);
void on_user_settings_dialog_delete_event (GnomeDialog *dialog, gpointer user_data);
void on_user_settings_clicked (GnomeDialog *dialog, gint button_number, gpointer user_data);
void on_user_settings_add_clicked (GtkButton *button, gpointer user_data);
void on_user_settings_remove_clicked (GtkButton *button, gpointer user_data);
void on_user_settings_gall_select_row (GtkCList *clist, gint row, gint column,
				       GdkEventButton *event, gpointer user_data);

void on_user_settings_gmember_select_row (GtkCList *clist, gint row, gint column,
					  GdkEventButton *event, gpointer user_data);

void on_user_passwd_change_clicked (GtkButton *button, gpointer user_data);
void on_user_passwd_random_clicked (GtkButton *button, gpointer user_data);
void user_password_change (xmlNodePtr user_node);

void on_group_settings_dialog_show (GtkWidget *widget, gpointer user_data);
void on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data);
void on_group_settings_dialog_delete_event (GtkWidget *w, gpointer user_data);
void on_group_settings_ok_clicked (GtkButton *button, gpointer user_data);
void on_group_settings_add_clicked (GtkButton *button, gpointer user_data);
void on_group_settings_remove_clicked (GtkButton *button, gpointer user_data);
void on_group_settings_all_select_row (GtkCList *clist, gint row, gint column,
				       GdkEventButton *event, gpointer user_data);

void on_group_settings_members_select_row (GtkCList *clist, gint row, gint column,
					   GdkEventButton *event, gpointer user_data);

void actions_set_sensitive (gint table, gboolean state);
void my_gtk_entry_set_text (void *entry, gchar *str);

void on_user_passwd_dialog_delete_event (GtkWidget *w, gpointer data);

#endif /* CALLBACKS_H */

