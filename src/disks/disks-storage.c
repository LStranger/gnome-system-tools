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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "gst.h"
#include "disks-storage.h"

extern GstTool *tool;

#define PARENT_TYPE G_TYPE_OBJECT

enum {
	PROP_0,
	PROP_NAME,
	PROP_MODEL,
	PROP_SIZE,
	PROP_ICON_NAME,
	PROP_DEVICE,
	PROP_SPEED,
	PROP_CHILDREN
};

struct _GstDisksStoragePriv
{
	gchar *name;
	gchar *model;
	gulong size;
	gchar *icon_name;
	gchar *device;
	gchar *speed;
	GList *children;
};

static void storage_init       (GstDisksStorage      *storage);
static void storage_class_init (GstDisksStorageClass *klass);
static void storage_finalize   (GObject              *object);

static void storage_set_property (GObject  *object, guint prop_id,
				  const GValue *value, GParamSpec *spec);
static void storage_get_property (GObject  *object, guint prop_id,
				  GValue *value, GParamSpec *spec);

static GObjectClass *parent_class = NULL;

GType
gst_disks_storage_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GstDisksStorageClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) storage_class_init,
			NULL,
			NULL,
			sizeof (GstDisksStorage),
			0,
			(GInstanceInitFunc) storage_init
		};
		type = g_type_register_static (PARENT_TYPE, "GstDisksStorage",
					       &info, 0);
	   }
	   return type;
}

static void
storage_init (GstDisksStorage *storage)
{
	g_return_if_fail (GST_IS_DISKS_STORAGE (storage));
	
	storage->priv = g_new0 (GstDisksStoragePriv, 1);
	storage->priv->icon_name = g_strdup ("gnome-dev-harddisk");
	storage->priv->name = g_strdup (_("Unknown Storage"));
	storage->priv->model = g_strdup (_("Unknown"));
	storage->priv->size = 0;
	storage->priv->speed = NULL;
	storage->priv->children = NULL;
}

static void
storage_class_init (GstDisksStorageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = storage_set_property;
	object_class->get_property = storage_get_property;

	g_object_class_install_property (object_class, PROP_NAME,
					 g_param_spec_string ("name", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_MODEL,
					 g_param_spec_string ("model", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_SIZE,
					 g_param_spec_ulong ("size", NULL, NULL,
							     0, G_MAXULONG, 0,
							     G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_ICON_NAME,
					 g_param_spec_string ("icon_name", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_DEVICE,
					 g_param_spec_string ("device", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_SPEED,
					 g_param_spec_string ("speed", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_CHILDREN,
					 g_param_spec_pointer ("children", NULL, NULL,
							       G_PARAM_READWRITE));
	
	object_class->finalize = storage_finalize;
}

static void
storage_finalize (GObject *object)
{
	GstDisksStorage *storage = GST_DISKS_STORAGE (object);
	g_return_if_fail (GST_IS_DISKS_STORAGE (storage));

	if (storage->priv) {
		if (storage->priv->children) {
			g_list_free (storage->priv->children);
			storage->priv->children = NULL;
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
	GstDisksStorage *storage;

	g_return_if_fail (GST_IS_DISKS_STORAGE (object));

	storage = GST_DISKS_STORAGE (object);

	switch (prop_id) {
	case PROP_NAME:
		if (storage->priv->name) g_free (storage->priv->name);
		storage->priv->name = g_value_dup_string (value);
		break;
	case PROP_MODEL:
		if (storage->priv->model) g_free (storage->priv->model);
		storage->priv->model = g_value_dup_string (value);
		break;
	case PROP_SIZE:
		storage->priv->size = g_value_get_ulong (value);
		break;
	case PROP_ICON_NAME:
		if (storage->priv->icon_name) g_free (storage->priv->icon_name);
		storage->priv->icon_name = g_value_dup_string (value);
		break;
	case PROP_DEVICE:
		if (storage->priv->device) g_free (storage->priv->device);
		storage->priv->device = g_value_dup_string (value);
		break;
	case PROP_SPEED:
		if (storage->priv->speed) g_free (storage->priv->speed);
		storage->priv->speed = g_value_dup_string (value);
		break;
	case PROP_CHILDREN:
		storage->priv->children = g_value_get_pointer (value);
		break;
	default:
		break;
	}
}

static void
storage_get_property (GObject  *object, guint prop_id, GValue *value,
		      GParamSpec *spec)
{
	GstDisksStorage *storage;
	
	g_return_if_fail (GST_IS_DISKS_STORAGE (object));
	
	storage = GST_DISKS_STORAGE (object);
	
	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, storage->priv->name);
		break;
	case PROP_MODEL:
		g_value_set_string (value, storage->priv->model);
		break;
	case PROP_SIZE:
		g_value_set_ulong (value, storage->priv->size);
		break;
	case PROP_ICON_NAME:
		g_value_set_string (value, storage->priv->icon_name);
		break;
	case PROP_DEVICE:
		g_value_set_string (value, storage->priv->device);
		break;
	case PROP_SPEED:
		g_value_set_string (value, storage->priv->speed);
		break;
	case PROP_CHILDREN:
		g_value_set_pointer (value, storage->priv->children);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	}
}

GstDisksStorage*
gst_disks_storage_new (void)
{
	GstDisksStorage *storage;
	
	storage = g_object_new (GST_TYPE_DISKS_STORAGE, NULL);

	return storage;
}

void
gst_disks_storage_add_child (GstDisksStorage *storage, GstDisksStorage *child)
{
	g_return_if_fail (GST_IS_DISKS_STORAGE (storage));
	g_return_if_fail (GST_IS_DISKS_STORAGE (child));

	storage->priv->children = g_list_append (storage->priv->children, child);
}

GtkWidget*
gst_disks_storage_get_properties_widget (GstDisksStorage *storage)
{
	g_return_val_if_fail (GST_IS_DISKS_STORAGE (storage), NULL);
	
	if (GST_DISKS_STORAGE_GET_CLASS (storage)->get_properties_widget) {
		return GST_DISKS_STORAGE_GET_CLASS (storage)->get_properties_widget (storage);
	} else {
		return NULL;
	}
}

void
gst_disks_storage_setup_properties_widget (GstDisksStorage *storage)
{
	g_return_if_fail (GST_IS_DISKS_STORAGE (storage));

	if (GST_DISKS_STORAGE_GET_CLASS (storage)->setup_properties_widget) {
		return GST_DISKS_STORAGE_GET_CLASS (storage)->setup_properties_widget (storage);
	} else {
		return;
	}
}

