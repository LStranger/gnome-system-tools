/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks-gui.h: this file is part of disks-admin, a gnome-system-tool frontend 
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

#ifndef __DISKS_GUI_H
#define __DISKS_GUI_H

#include <gnome.h>

#include "disks-config.h"
#include "disks-storage-disk.h"
#include "disks-storage-cdrom.h"
#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-cdrom-disc-audio.h"

enum {
	POPUP_PARTITION_FORMAT,
	POPUP_PARTITION_REMOVE
};

enum {
	STORAGE_LIST_ICON,
	STORAGE_LIST_NAME,
	STORAGE_LIST_POINTER,

	STORAGE_LIST_LAST
};

enum {
	PARTITION_LIST_NAME,
	PARTITION_LIST_POINTER,

	PARTITION_LIST_LAST
};

gchar *        gst_storage_get_human_readable_size (const gulong size);
GdkPixbuf *    gst_storage_get_icon            (const gchar *icon_name);
gfloat         gst_storage_get_float_size      (const gulong size);

void           gst_disks_gui_setup                      (GstDisksConfig *cfg);
void           gst_disks_gui_set_device_speed           (GstDisksStorage *storage);
void           gst_disks_gui_setup_partition_list       (GtkWidget *treeview, GList *partitions);
void           gst_disks_gui_setup_disk_properties      (GstDisksStorageDisk *disk);
void           gst_disks_gui_setup_partition_properties (GstDisksPartition *part);
void           gst_disks_gui_setup_cdrom_properties     (GstDisksStorageCdrom *cdrom);
void           gst_disks_gui_setup_cdrom_disc_data      (GstCdromDiscData *disc_data);
void           gst_disks_gui_setup_cdrom_disc_audio     (GstCdromDiscAudio *disc_audio);

#endif /* __DISKS_GUI_H */
