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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>

#include "disks-storage.h"
#include "disks-storage-disk.h"
#include "disks-gui.h"

#define PARENT_TYPE GST_TYPE_DISKS_STORAGE

enum {
	PROP_0,
	PROP_PARTITIONS
};

struct _GstDisksStorageDiskPriv
{
	GList *partitions;
	/*gboolean use_dma;*/
};

static void storage_disk_init       (GstDisksStorageDisk      *storage);
static void storage_disk_class_init (GstDisksStorageDiskClass *klass);
static void storage_disk_finalize   (GObject                  *object);

static void storage_set_property (GObject  *object, guint prop_id,
				  const GValue *value, GParamSpec *spec);
static void storage_get_property (GObject  *object, guint prop_id,
				  GValue *value, GParamSpec *spec);

static void storage_disk_setup_properties_widget (GstDisksStorage *storage);

static GObjectClass *parent_class = NULL;

GType
gst_disks_storage_disk_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GstDisksStorageDiskClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) storage_disk_class_init,
			NULL,
			NULL,
			sizeof (GstDisksStorageDisk),
			0,
			(GInstanceInitFunc) storage_disk_init
		};
		type = g_type_register_static (PARENT_TYPE, "GstDisksStorageDisk",
					       &info, 0);
	   }
	   return type;
}

static void
storage_disk_init (GstDisksStorageDisk *storage)
{
	g_return_if_fail (GST_IS_DISKS_STORAGE_DISK (storage));
	
	storage->priv = g_new0 (GstDisksStorageDiskPriv, 1);
	/*storage->priv->use_dma = FALSE;*/
	storage->priv->partitions = NULL;

	g_object_set (G_OBJECT (storage),
		      "name", _("Hard Disk"),
		      "icon_name", "gnome-dev-harddisk",
		      NULL);
}

static void
storage_disk_class_init (GstDisksStorageDiskClass *klass)
{
	GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
	GstDisksStorageClass *storage_class = GST_DISKS_STORAGE_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = storage_set_property;
	object_class->get_property = storage_get_property;
	
	storage_class->setup_properties_widget = storage_disk_setup_properties_widget;

	g_object_class_install_property (object_class, PROP_PARTITIONS,
					 g_param_spec_pointer ("partitions", NULL, NULL,
							       G_PARAM_READWRITE));
	
	object_class->finalize = storage_disk_finalize;
}

static void
delete_object (gpointer object, gpointer gdata)
{
	g_object_unref (G_OBJECT (object));
}

static void
storage_disk_finalize (GObject *object)
{
	GstDisksStorageDisk *storage = GST_DISKS_STORAGE_DISK (object);
	g_return_if_fail (GST_IS_DISKS_STORAGE_DISK (storage));

	if (storage->priv) {
		if (storage->priv->partitions) {
			g_list_foreach (storage->priv->partitions,
					delete_object, NULL);
			g_list_free (storage->priv->partitions);
			storage->priv->partitions = NULL;
		}
		g_free (storage->priv);
		storage->priv = NULL;
	}
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
storage_set_property (GObject  *object, guint prop_id, const GValue *value,
		      GParamSpec *spec)
{
	GstDisksStorageDisk *storage;

	g_return_if_fail (GST_IS_DISKS_STORAGE_DISK (object));

	storage = GST_DISKS_STORAGE_DISK (object);

	switch (prop_id) {
	case PROP_PARTITIONS:
		storage->priv->partitions = g_value_get_pointer (value);
		
		break;
	default:
		break;
	}
}

static void
storage_get_property (GObject  *object, guint prop_id, GValue *value,
		      GParamSpec *spec)
{
	GstDisksStorageDisk *storage;

	g_return_if_fail (GST_IS_DISKS_STORAGE_DISK (object));

	storage = GST_DISKS_STORAGE_DISK (object);

	switch (prop_id) {
	case PROP_PARTITIONS:
		g_value_set_pointer (value, storage->priv->partitions);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	}
}

GstDisksStorage*
gst_disks_storage_disk_new (void)
{
	GstDisksStorageDisk *storage;
	
	storage = g_object_new (GST_TYPE_DISKS_STORAGE_DISK, NULL);

	return GST_DISKS_STORAGE (storage);
}

static void
storage_disk_setup_properties_widget (GstDisksStorage *storage)
{
	GstDisksStorageDisk *disk;

	disk = GST_DISKS_STORAGE_DISK (storage);

	gst_disks_gui_setup_disk_properties (disk);
	
	GST_DISKS_STORAGE_GET_CLASS (storage)->setup_common_properties (storage);
}

GstDisksPartition *
gst_disks_storage_disk_get_partition (GstDisksStorageDisk *disk, const gchar *device)
{
	GList *list = NULL;
	GList *partitions = NULL;
	GstDisksPartition *part;
	gchar *part_device;

	g_return_val_if_fail (GST_IS_DISKS_STORAGE_DISK (disk), NULL);

	g_object_get (G_OBJECT (disk), "partitions", &partitions, NULL);

	if (partitions)
		list = g_list_first (partitions);
	
	while (list) {
		part = GST_DISKS_PARTITION (list->data);
		g_object_get (G_OBJECT (part), "device", &part_device, NULL);

		if (g_ascii_strcasecmp (part_device, device) == 0)
			return part;

		list = g_list_next (list);
	}

	return NULL;
}


