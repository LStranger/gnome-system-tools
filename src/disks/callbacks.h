/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.h: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
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
 * Authors: Alvaro Peña Gonzalez <apg@esware.com>
 *          Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

#include <gtk/gtk.h>

void gst_on_storage_list_selection_change   (GtkTreeSelection *selection, gpointer gdata);
gboolean gst_on_storage_list_button_press   (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata);
void gst_on_partition_list_selection_change (GtkTreeSelection *selection, gpointer gdata);
void gst_on_point_entry_changed             (GtkWidget *entry, gpointer gdata);
void gst_on_format_button_clicked           (GtkWidget *button, gpointer gdata);
void gst_on_mount_button_clicked            (GtkWidget *button, gpointer gdata);
void gst_on_browse_button_clicked           (GtkWidget *button, gpointer gdata);
void gst_on_play_button_clicked             (GtkWidget *button, gpointer gdata);
void     gst_on_speed_property_changed      (GObject *object, GParamSpec *spec, gpointer gdata);
void  gst_disks_storage_get_device_speed_cb (GstDirectiveEntry *entry);
void  gst_disks_storage_update_device_speed (GtkWidget *w, gpointer gdata);
void  gst_on_change_mp_button_clicked       (GtkWidget *w, gpointer gdata);

#endif /* __CALLBACKS_H */
