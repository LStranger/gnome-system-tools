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

#include <gnome.h>

void gst_on_storage_list_selection_change   (GtkWidget *selection, gpointer gdata);
void gst_on_partition_list_selection_change (GtkTreeSelection *selection, gpointer gdata);
gboolean gst_on_partition_list_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata);
void     gst_on_speed_property_changed      (GObject *object, GParamSpec *spec, gpointer gdata);
void  gst_disks_storage_get_device_speed_cb (GstDirectiveEntry *entry);
void  gst_disks_storage_update_device_speed (GtkWidget *w, gpointer gdata);

#endif /* __CALLBACKS_H */
