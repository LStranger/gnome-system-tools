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
#include "disks-storage-floppy.h"
#include "disks-gui.h"

#define PARENT_TYPE GST_TYPE_DISKS_STORAGE

static void storage_floppy_init       (GstDisksStorageFloppy      *storage);
static void storage_floppy_class_init (GstDisksStorageFloppyClass *klass);
static void storage_floppy_finalize   (GObject                    *object);

/*static GtkWidget*  storage_floppy_get_properties_widget   (GstDisksStorage *storage);*/

static GObjectClass *parent_class = NULL;

GType
gst_disks_storage_floppy_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GstDisksStorageFloppyClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) storage_floppy_class_init,
			NULL,
			NULL,
			sizeof (GstDisksStorageFloppy),
			0,
			(GInstanceInitFunc) storage_floppy_init
		};
		type = g_type_register_static (PARENT_TYPE, "GstDisksStorageFloppy",
					       &info, 0);
	   }
	   return type;
}

static void
storage_floppy_init (GstDisksStorageFloppy *storage)
{
	g_return_if_fail (GST_IS_DISKS_STORAGE_FLOPPY (storage));
	
	g_object_set (G_OBJECT (storage),
		      "name", _("Floppy"),
		      "icon_name", "gnome-dev-floppy",
		      NULL);
}

static void
storage_floppy_class_init (GstDisksStorageFloppyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = storage_floppy_finalize;
}

static void
storage_floppy_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/*static GtkWidget *
storage_floppy_get_properties_widget (GstDisksStorage *storage)
{
	return NULL;
}*/

GstDisksStorage*
gst_disks_storage_floppy_new (void)
{
	GstDisksStorageFloppy *storage;
	
	storage = g_object_new (GST_TYPE_DISKS_STORAGE_FLOPPY, NULL);

	return GST_DISKS_STORAGE (storage);
}
