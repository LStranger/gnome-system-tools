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

#ifndef __DISKS_PARTITION_H__
#define __DISKS_PARTITION_H__

#include <glib-object.h>

#define GST_TYPE_DISKS_PARTITION          (gst_disks_partition_get_type ())
#define GST_DISKS_PARTITION(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_PARTITION, GstDisksPartition))
#define GST_DISKS_PARTITION_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_DISKS_PARTITION, GstDisksPartitionClass))
#define GST_IS_DISKS_PARTITION(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_PARTITION))
#define GST_IS_DISKS_PARTITION_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_DISKS_PARTITION))
#define GST_DISKS_PARTITION_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_DISKS_PARTITION, GstDisksPartitionClass))

typedef struct _GstDisksPartition      GstDisksPartition;
typedef struct _GstDisksPartitionClass GstDisksPartitionClass;
typedef struct _GstDisksPartitionPriv  GstDisksPartitionPriv;

typedef enum {
	PARTITION_TYPE_EXT2,
	PARTITION_TYPE_EXT3,
	PARTITION_TYPE_REISERFS,
	PARTITION_TYPE_XFS,
	PARTITION_TYPE_JFS,
	PARTITION_TYPE_VFAT,
	PARTITION_TYPE_NTFS,
	PARTITION_TYPE_SWAP,
	PARTITION_TYPE_FREE,
	PARTITION_TYPE_UNKNOWN,
	PARTITION_TYPE_NONE,
	
	PARTITION_TYPE_NUM
} GstPartitionTypeFs;

typedef struct {
	gchar *fs_name;
	gchar *fs_hr_name;
	gchar *fs_format_command;
} GstPartitionTypeFsInfo;

struct _GstDisksPartition {
        GObject               parent;

	GstDisksPartitionPriv *priv;
};

struct _GstDisksPartitionClass {
        GObjectClass parent_class;
};

GType              gst_disks_partition_get_type (void);
GstDisksPartition* gst_disks_partition_new      (void);

void                      gst_disks_partition_setup_properties_widget   (GstDisksPartition *part);
void                      gst_disks_partition_format                    (GstDisksPartition *part,
									 GstPartitionTypeFs fs_type,
									 const gchar *point);
void                      gst_disks_partition_browse                    (GstDisksPartition *part);
gchar                    *gst_disks_partition_get_human_readable_typefs (GstPartitionTypeFs type);
GstPartitionTypeFs        gst_disks_partition_get_typefs_from_name      (const gchar *name);
GstPartitionTypeFs        gst_disks_partition_get_typefs_from_hr_name   (const gchar *name);
gchar                    *gst_disks_partition_get_typefs                (GstPartitionTypeFs type);
GstPartitionTypeFsInfo   *gst_disks_partition_get_type_fs_info_table    ();

#endif /* __GST_DISKS_PARTITION_H__  */
