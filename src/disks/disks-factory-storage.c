/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks-factory-storage.c: this file is part of disks-admin, a gnome-system-tool frontend
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

#include "disks-factory-storage.h"
#include "disks-partition.h"
#include "disks-storage-disk.h"
#include "disks-storage-cdrom.h"
#include "disks-storage-floppy.h"

GstDisksStorage*
gst_disks_factory_storage_get (const gchar *media)
{
	   GstDisksStorage *storage = NULL;

	   g_return_val_if_fail (media != NULL, NULL);
	   
	   if (media) {
			 if (g_ascii_strcasecmp (media, "disk") == 0)
				    storage = GST_DISKS_STORAGE (gst_disks_storage_disk_new ());
			 else if (g_ascii_strcasecmp (media, "cdrom") == 0)
				    storage = GST_DISKS_STORAGE (gst_disks_storage_cdrom_new ());
			 else if (g_ascii_strcasecmp (media, "floppy") == 0)
				    storage = GST_DISKS_STORAGE (gst_disks_storage_floppy_new ());
			 else
				 g_warning ("Unknown media type: %s\n", media);
	   }
	   
	   return storage;
}
