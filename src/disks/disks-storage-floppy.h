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

#ifndef __DISKS_STORAGE_FLOPPY_H__
#define __DISKS_STORAGE_FLOPPY_H__

#include <glib-object.h>

#define GST_TYPE_DISKS_STORAGE_FLOPPY         (gst_disks_storage_floppy_get_type ())
#define GST_DISKS_STORAGE_FLOPPY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_STORAGE_FLOPPY, GstDisksStorageFloppy))
#define GST_DISKS_STORAGE_FLOPPY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_DISKS_STORAGE_FLOPPY, GstDisksStorageFloppyClass))
#define GST_IS_DISKS_STORAGE_FLOPPY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_STORAGE_FLOPPY))
#define GST_IS_DISKS_STORAGE_FLOPPY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_DISKS_STORAGE_FLOPPY))
#define GST_DISKS_STORAGE_FLOPPY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_DISKS_STORAGE_FLOPPY, GstDisksStorageFloppyClass))

typedef struct _GstDisksStorageFloppy      GstDisksStorageFloppy;
typedef struct _GstDisksStorageFloppyClass GstDisksStorageFloppyClass;

struct _GstDisksStorageFloppy {
        GstDisksStorage      parent;
};

struct _GstDisksStorageFloppyClass {
        GstDisksStorageClass parent_class;
};

GType            gst_disks_storage_floppy_get_type (void);
GstDisksStorage* gst_disks_storage_floppy_new      (void);

#endif /* __GST_DISKS_STORAGE_FLOPPY_H__  */
