/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks.c: this file is part of disks-admin, a gnome-system-tool frontend 
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "gst.h"

#include "disks.h"

GstDisksConfig *
gst_disks_config_new ()
{
	GstDisksConfig *dsk;

	dsk = g_new0 (GstDisksConfig, 1);

	dsk->disks = NULL;
	dsk->num_disks = 0;

	return dsk;
}

void 
gst_disks_config_add_disk (GstDisksConfig *dsk, GstDisk *disk)
{
	dsk->disks = g_list_append (dsk->disks, (gpointer) disk);
	dsk->num_disks ++;
}

GstDisk *
gst_disks_disk_new (const gchar *name)
{
	GstDisk *disk;

	disk = g_new0 (GstDisk, 1);

	disk->name = g_strdup (name);
	disk->num_parts = 0;
	disk->partitions = NULL;

	return disk;
}

void
gst_disks_disk_add_partition (GstDisk *disk, GstPartition *part)
{
	disk->partitions = g_list_append (disk->partitions, (gpointer) part);
	disk->num_parts ++;
}

GstPartition *
gst_disks_partition_new (const gchar *device)
{
	GstPartition *part;

	part = g_new0 (GstPartition, 1);

	part->device = g_strdup (device);
	part->type = NULL;
	part->point = NULL;
	part->bootable = FALSE;
	part->mounted = FALSE;
	part->listed = FALSE;
	part->detected = FALSE;

	return part;
}
