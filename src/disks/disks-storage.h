/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  
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

#ifndef __DISKS_STORAGE_H__
#define __DISKS_STORAGE_H__

#include <glib-object.h>

#define GST_TYPE_DISKS_STORAGE         (gst_disks_storage_get_type ())
#define GST_DISKS_STORAGE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_STORAGE, GstDisksStorage))
#define GST_DISKS_STORAGE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_DISKS_STORAGE, GstDisksStorageClass))
#define GST_IS_DISKS_STORAGE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_STORAGE))
#define GST_IS_DISKS_STORAGE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_DISKS_STORAGE))
#define GST_DISKS_STORAGE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_DISKS_STORAGE, GstDisksStorageClass))

typedef struct _GstDisksStorage      GstDisksStorage;
typedef struct _GstDisksStorageClass GstDisksStorageClass;
typedef struct _GstDisksStoragePriv  GstDisksStoragePriv;

struct _GstDisksStorage {
        GObject        parent;

	GstDisksStoragePriv *priv;
};

struct _GstDisksStorageClass {
        GObjectClass parent_class;

	void       (* setup_properties_widget) (GstDisksStorage *storage);
	void       (* setup_common_properties) (GstDisksStorage *storage);
};

GType      gst_disks_storage_get_type      (void);

void       gst_disks_storage_add_child               (GstDisksStorage *storage, GstDisksStorage *child);
void       gst_disks_storage_setup_properties_widget (GstDisksStorage *storage);

#endif /* __DISKS_STORAGE_H__  */
