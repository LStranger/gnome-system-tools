/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

typedef struct _GstTablePopup GstTablePopup;

struct _GstTablePopup
{
  void (* setup) (GtkWidget *);
  void (* properties) (GtkWidget*, gpointer);
  GtkWidget *popup;
};

void  on_table_selection_changed   (GtkTreeSelection*, gpointer);
void  on_iface_properties_clicked  (GtkWidget*, gpointer);
void  on_activate_button_clicked   (GtkWidget*, gpointer);
void  on_deactivate_button_clicked (GtkWidget*, gpointer);

void  on_iface_active_changed      (GtkWidget*, gpointer);
void  on_bootproto_changed         (GtkWidget*, gpointer);
void  on_connection_cancel_clicked (GtkWidget*, gpointer);
void  on_connection_ok_clicked     (GtkWidget*, gpointer);
void  on_detect_modem_clicked      (GtkWidget*, gpointer);

gboolean on_table_button_press (GtkWidget*, GdkEventButton*, gpointer);
gboolean on_table_popup_menu   (GtkWidget*, gpointer);

void  on_host_aliases_add_clicked        (GtkWidget*, gpointer);
void  on_host_aliases_properties_clicked (GtkWidget*, gpointer);
void  on_host_aliases_delete_clicked     (GtkWidget*, gpointer);

void  on_dialog_changed (GtkWidget*, gpointer);

gboolean on_ip_address_focus_out (GtkWidget*, GdkEventFocus*, gpointer);
gboolean callbacks_check_hostname_hook (GstDialog*, gpointer);

#endif /* __CALLBACKS_H */
