/* callbacks.h: this file is part of users-admin, a helix-setup-tool frontend 
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

#include <gnome.h>
#include <gnome-xml/tree.h>


extern xmlDocPtr doc;

extern void 
on_users_admin_delete_event            (GtkWidget * widget, 
																				GdkEvent * event, 
																				gpointer gdata);

extern void
on_close_clicked                       (GtkButton *button, 
																				gpointer data);

extern void
on_apply_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

extern void
on_user_settings_clicked (GtkButton *button, gpointer user_data);

extern void
on_user_settings_cancel_clicked (GtkButton *button, gpointer user_data);

extern void
on_user_settings_ok_clicked (GtkButton *button, gpointer user_data);



extern void
on_user_chpasswd_clicked (GtkButton *button, gpointer user_data);

extern void
on_user_new_clicked (GtkButton *button, gpointer user_data);

extern void
on_user_delete_clicked (GtkButton *button, gpointer user_data);

extern void
on_user_list_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event, 
		gpointer user_data);



extern void
on_group_settings_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_new_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_delete_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_list_select_row (GtkCList *clist, gint row, gint column, GdkEventButton *event, 
		gpointer user_data);



extern void
on_user_passwd_cancel_clicked (GtkButton *button, gpointer user_data);

extern void
on_user_passwd_ok_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_settings_cancel_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_settings_ok_clicked (GtkButton *button, gpointer user_data);



extern void
on_group_settings_add_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_settings_remove_clicked (GtkButton *button, gpointer user_data);

extern void
on_group_settings_all_selection_changed (GtkWidget *list, gpointer user_data);

extern void
on_group_settings_members_selection_changed (GtkWidget *list, gpointer user_data);

