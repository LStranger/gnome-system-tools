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
 * Authors: Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>

#include "disks-mountable.h"
#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-gui.h"
#include "transfer.h"

#define PARENT_TYPE GST_TYPE_CDROM_DISC

struct _GstCdromDiscDataPriv
{
	gulong size;
	gchar *mount_point;
	gboolean mounted;
};

static void cdrom_disc_data_init                    (GstCdromDiscData      *disc);
static void cdrom_disc_data_class_init              (GstCdromDiscDataClass *klass);
static void cdrom_disc_data_finalize                (GObject                   *object);

static void cdrom_disc_data_set_property (GObject *object, guint prop_id,
					  const GValue *value, GParamSpec *spec);
static void cdrom_disc_data_get_property (GObject *object, guint prop_id,
					  GValue *value, GParamSpec *spec);

static void cdrom_disc_data_mount          (GstDisksMountable *mountable);
static void cdrom_disc_data_mountable_init (GstDisksMountableIface *iface);

static void cdrom_disc_data_setup_gui (GstCdromDisc *disc);

static GObjectClass *parent_class = NULL;

enum {
	PROP_0,
	PROP_SIZE,
	PROP_MOUNT_POINT,
	PROP_MOUNTED
};

GType
gst_cdrom_disc_data_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GstCdromDiscDataClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cdrom_disc_data_class_init,
			NULL,
			NULL,
			sizeof (GstCdromDiscData),
			0,
			(GInstanceInitFunc) cdrom_disc_data_init
		};
		static const GInterfaceInfo mountable_info = {
			(GInterfaceInitFunc) cdrom_disc_data_mountable_init,
			NULL,
			NULL
		};
		type = g_type_register_static (PARENT_TYPE, "GstCdromDiscData",
					       &info, 0);
		g_type_add_interface_static (type,
					     GST_TYPE_DISKS_MOUNTABLE,
					     &mountable_info);
	}
	return type;
}

static void
cdrom_disc_data_mountable_init (GstDisksMountableIface *iface)
{
	iface->mount = cdrom_disc_data_mount;
}

static void
cdrom_disc_data_init (GstCdromDiscData *disc)
{
	g_return_if_fail (GST_IS_CDROM_DISC_DATA (disc));
	
	disc->priv = g_new0 (GstCdromDiscDataPriv, 1);
	disc->priv->size = 0;
	disc->priv->mount_point = NULL;
	disc->priv->mounted = FALSE;
}

static void
cdrom_disc_data_class_init (GstCdromDiscDataClass *klass)
{
	GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
	GstCdromDiscClass    *cdrom_disc_class = GST_CDROM_DISC_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	
	object_class->set_property = cdrom_disc_data_set_property;
	object_class->get_property = cdrom_disc_data_get_property;
	
	cdrom_disc_class->setup_gui = cdrom_disc_data_setup_gui;
	
	g_object_class_install_property (object_class, PROP_SIZE,
					 g_param_spec_ulong ("size", NULL, NULL,
							     0, G_MAXULONG, 0,
							     G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_MOUNT_POINT,
					 g_param_spec_string ("mount-point", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_MOUNTED,
					 g_param_spec_boolean ("mounted", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	
	object_class->finalize = cdrom_disc_data_finalize;
}

static void
cdrom_disc_data_finalize (GObject *object)
{
	GstCdromDiscData *disc = GST_CDROM_DISC_DATA (object);
	g_return_if_fail (GST_IS_CDROM_DISC_DATA (disc));
	
	if (disc->priv) {
		if (disc->priv->mount_point) {
			g_free (disc->priv->mount_point);
			disc->priv->mount_point = NULL;
		}
		
		g_free (disc->priv);
		disc->priv = NULL;
	}
	
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
cdrom_disc_data_setup_gui (GstCdromDisc *disc)
{
	GstCdromDiscData *disc_data;
	
	disc_data = GST_CDROM_DISC_DATA (disc);
	
	gst_disks_gui_setup_cdrom_disc_data (disc_data);
}

static void
cdrom_disc_data_mount (GstDisksMountable *mountable)
{
	g_return_if_fail (GST_IS_CDROM_DISC_DATA (mountable));

	gst_disks_mount_cdrom_disc_data (GST_CDROM_DISC_DATA (mountable));
}

void
gst_disks_cdrom_disc_data_browse (GstCdromDiscData *disc)
{
	gchar *point, *browser;
	gchar *command;
	
	g_return_if_fail (GST_IS_CDROM_DISC_DATA (disc));
	
	g_object_get (G_OBJECT (disc), "mount-point", &point, NULL);
	if (point) {
		if ((browser = g_find_program_in_path ("nautilus"))) {
			command = g_strdup_printf ("%s %s", browser, point);
			g_spawn_command_line_async (command, NULL);
			g_free (browser);
			g_free (command);
		}
	}
}

GstCdromDisc *
gst_cdrom_disc_data_new (void)
{
	GstCdromDiscData *disc;
	
	disc = g_object_new (GST_TYPE_CDROM_DISC_DATA, NULL);
	
	return GST_CDROM_DISC (disc);
}

static void
cdrom_disc_data_set_property (GObject *object, guint prop_id, const GValue *value,
			      GParamSpec *spec)
{
	GstCdromDiscData *disc;
	
	g_return_if_fail (GST_IS_CDROM_DISC_DATA (object));
	
	disc = GST_CDROM_DISC_DATA (object);
	
	switch (prop_id) {
	case PROP_SIZE:
		disc->priv->size = g_value_get_ulong (value);
		break;
	case PROP_MOUNT_POINT:
		if (disc->priv->mount_point) g_free (disc->priv->mount_point);
		disc->priv->mount_point = g_value_dup_string (value);
		break;
	case PROP_MOUNTED:
		disc->priv->mounted = g_value_get_boolean (value);
	default:
		break;
	}
}

static void
cdrom_disc_data_get_property (GObject *object, guint prop_id, GValue *value,
			      GParamSpec *spec)
{
	GstCdromDiscData *disc;
	
	g_return_if_fail (GST_IS_CDROM_DISC_DATA (object));
	
	disc = GST_CDROM_DISC_DATA (object);
	
	switch (prop_id) {
	case PROP_SIZE:
		g_value_set_ulong (value, disc->priv->size);
		break;
	case PROP_MOUNT_POINT:
		g_value_set_string (value, disc->priv->mount_point);
		break;
	case PROP_MOUNTED:
		g_value_set_boolean (value, disc->priv->mounted);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	}
}

