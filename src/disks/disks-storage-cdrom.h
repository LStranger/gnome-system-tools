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

#ifndef __GST_DISKS_STORAGE_CDROM_H__
#define __GST_DISKS_STORAGE_CDROM_H__

#include <glib-object.h>

#include "disks-storage.h"
#include "disks-cdrom-disc.h"

#define GST_TYPE_DISKS_STORAGE_CDROM         (gst_disks_storage_cdrom_get_type ())
#define GST_DISKS_STORAGE_CDROM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_STORAGE_CDROM, GstDisksStorageCdrom))
#define GST_DISKS_STORAGE_CDROM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_DISKS_STORAGE_CDROM, GstDisksStorageCdromClass))
#define GST_IS_DISKS_STORAGE_CDROM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_STORAGE_CDROM))
#define GST_IS_DISKS_STORAGE_CDROM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_DISKS_STORAGE_CDROM))
#define GST_DISKS_STORAGE_CDROM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_DISKS_STORAGE_CDROM, GstDisksStorageCdromClass))

typedef struct _GstDisksStorageCdrom      GstDisksStorageCdrom;
typedef struct _GstDisksStorageCdromClass GstDisksStorageCdromClass;
typedef struct _GstDisksStorageCdromPriv  GstDisksStorageCdromPriv;

struct _GstDisksStorageCdrom {
        GstDisksStorage      parent;

        GstDisksStorageCdromPriv *priv;
};

struct _GstDisksStorageCdromClass {
        GstDisksStorageClass parent_class;
};

GType            gst_disks_storage_cdrom_get_type (void);
GstDisksStorage *gst_disks_storage_cdrom_new      (void);

/*void             gst_disks_cdrom_mount                             (GstDisksStorageCdrom *cdrom);*/
gchar           *gst_disks_storage_cdrom_get_human_readable_status (GstDisksStorageCdrom *cdrom);
GstCdromDisc    *gst_disks_cdrom_set_disc                          (GstDisksStorageCdrom *cdrom);

#endif /* __GST_DISKS_STORAGE_CDROM_H__  */
