/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks.h: this file is part of disks-admin, a gnome-system-tool frontend 
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

#ifndef __DISKS_H
#define __DISKS_H

#include <gnome.h>


typedef struct
{
	GList *disks;
	gint num_disks;
	
} GstDisksConfig;

typedef struct
{
	gchar *device;
	gchar *type;
	gchar *point;
	gboolean bootable;
	gboolean mounted;
	gboolean listed;
	gboolean detected;
} GstPartition;

typedef struct 
{
	gint num_parts;
	gchar *name;
	GList *partitions;
	
} GstDisk;

GstDisksConfig * gst_disks_config_new ();
void gst_disks_config_add_disk (GstDisksConfig *dsk, GstDisk *disk);
GstDisk * gst_disks_disk_new (const gchar *name);
void gst_disks_disk_add_partition (GstDisk *disk, GstPartition *part);
GstPartition * gst_disks_partition_new (const gchar *device);

#endif /* __DISKS_H */
