/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.h: this file is part of services-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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


#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

void		on_main_dialog_update_complexity	(GtkWidget*,   gpointer);
void		on_services_table_select_row		(GtkTreeSelection*, gpointer);
void		on_description_button_clicked		(GtkWidget*,   gpointer);
void		on_settings_button_clicked		(GtkWidget*,   gpointer);
void            on_menu_item_activate                   (GtkWidget*,   gpointer);
void            on_throw_service_button_clicked         (GtkWidget*,   gpointer);
void            on_service_priority_changed             (GtkWidget*,   gpointer);
void            on_runlevel_changed                     (GtkWidget*,   gpointer);
void            on_service_toggled                      (GtkWidget*,   gchar*, gpointer);
void            on_popup_settings_activate              (gpointer, guint, GtkWidget*);
gboolean        on_table_button_press_event             (GtkWidget*, GdkEventButton*, gpointer);

#endif /* CALLBACKS_H */
