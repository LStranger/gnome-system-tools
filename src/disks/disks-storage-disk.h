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

#ifndef __DISKS_STORAGE_DISK_H__
#define __DISKS_STORAGE_DISK_H__

#include <glib-object.h>
#include "disks-storage.h"
#include "disks-partition.h"

#define GST_TYPE_DISKS_STORAGE_DISK         (gst_disks_storage_disk_get_type ())
#define GST_DISKS_STORAGE_DISK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_STORAGE_DISK, GstDisksStorageDisk))
#define GST_DISKS_STORAGE_DISK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_DISKS_STORAGE_DISK, GstDisksStorageDiskClass))
#define GST_IS_DISKS_STORAGE_DISK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_STORAGE_DISK))
#define GST_IS_DISKS_STORAGE_DISK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_DISKS_STORAGE_DISK))
#define GST_DISKS_STORAGE_DISK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_DISKS_STORAGE_DISK, GstDisksStorageDiskClass))

typedef struct _GstDisksStorageDisk      GstDisksStorageDisk;
typedef struct _GstDisksStorageDiskClass GstDisksStorageDiskClass;
typedef struct _GstDisksStorageDiskPriv  GstDisksStorageDiskPriv;

struct _GstDisksStorageDisk {
        GstDisksStorage      parent;

        GstDisksStorageDiskPriv *priv;
};

struct _GstDisksStorageDiskClass {
        GstDisksStorageClass parent_class;
};

GType            gst_disks_storage_disk_get_type (void);
GstDisksStorage* gst_disks_storage_disk_new      (void);
GstDisksPartition *gst_disks_storage_disk_get_partition (GstDisksStorageDisk *disk,
							 const gchar *device);

#endif /* __GST_DISKS_STORAGE_DISK_H__  */
