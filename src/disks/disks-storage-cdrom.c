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

#include <libgnome/gnome-i18n.h>

#include "disks-storage.h"
#include "disks-storage-cdrom.h"
#include "disks-gui.h"

#define PARENT_TYPE GST_TYPE_DISKS_STORAGE

struct _GstDisksStorageCdromPriv
{
	GstCdromStatus status;
	/*gboolean automount;*/
	gboolean play_audio;
	gboolean write_cdr;
	gboolean write_cdrw;
	gboolean read_dvd;
	gboolean write_dvdr;
	gboolean write_dvdram;
};

static void storage_cdrom_init                    (GstDisksStorageCdrom      *storage);
static void storage_cdrom_class_init              (GstDisksStorageCdromClass *klass);
static void storage_cdrom_finalize                (GObject                   *object);

static void storage_cdrom_set_property (GObject *object, guint prop_id,
					const GValue *value, GParamSpec *spec);
static void storage_cdrom_get_property (GObject *object, guint prop_id,
					GValue *value, GParamSpec *spec);

static void        storage_cdrom_setup_properties_widget (GstDisksStorage *storage);

static GObjectClass *parent_class = NULL;

enum {
	PROP_0,
	PROP_STATUS,
	PROP_PLAY_AUDIO,
	PROP_WRITE_CDR,
	PROP_WRITE_CDRW,
	PROP_READ_DVD,
	PROP_WRITE_DVDR,
	PROP_WRITE_DVDRAM,
};

#define GST_CDROM_STATUS (gst_disks_storage_cdrom_status_get_type ())

static
GType gst_disks_storage_cdrom_status_get_type (void)
{
	static GType cdrom_status_type = 0;
	static GEnumValue cdrom_status[] = {
		{ CDROM_STATUS_EMPTY, "0", NULL },
		{ CDROM_STATUS_DATA,  "1", NULL },
		{ CDROM_STATUS_AUDIO, "2", NULL },
		{ CDROM_STATUS_MIXED, "3", NULL },
		{ CDROM_STATUS_BLANK, "4", NULL },
	};
	if (!cdrom_status_type) {
		cdrom_status_type = g_enum_register_static ("GstCdromStatus", cdrom_status);
	}
	return cdrom_status_type;
}

GType
gst_disks_storage_cdrom_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GstDisksStorageCdromClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) storage_cdrom_class_init,
			NULL,
			NULL,
			sizeof (GstDisksStorageCdrom),
			0,
			(GInstanceInitFunc) storage_cdrom_init
		};
		type = g_type_register_static (PARENT_TYPE, "GstDisksStorageCdrom",
					       &info, 0);
	   }
	   return type;
}

static void
storage_cdrom_init (GstDisksStorageCdrom *storage)
{
	g_return_if_fail (GST_IS_DISKS_STORAGE_CDROM (storage));
	
	storage->priv = g_new0 (GstDisksStorageCdromPriv, 1);
	
	g_object_set (G_OBJECT (storage), "name", _("CDROM"),
		      "icon_name", "gnome-dev-cdrom", NULL);
}

static void
storage_cdrom_class_init (GstDisksStorageCdromClass *klass)
{
	GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
	GstDisksStorageClass *storage_class = GST_DISKS_STORAGE_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = storage_cdrom_set_property;
	object_class->get_property = storage_cdrom_get_property;

	storage_class->setup_properties_widget = storage_cdrom_setup_properties_widget;

	g_object_class_install_property (object_class, PROP_STATUS,
					 g_param_spec_enum ("status", NULL, NULL,
							    GST_CDROM_STATUS,
							    0,
							    G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_PLAY_AUDIO,
					 g_param_spec_boolean ("play_audio", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_WRITE_CDR,
					 g_param_spec_boolean ("write_cdr", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_WRITE_CDRW,
					 g_param_spec_boolean ("write_cdrw", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_READ_DVD,
					 g_param_spec_boolean ("read_dvd", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_WRITE_DVDR,
					 g_param_spec_boolean ("write_dvdr", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_WRITE_DVDRAM,
					 g_param_spec_boolean ("write_dvdram", NULL, NULL,
							       FALSE, G_PARAM_READWRITE));
	
	object_class->finalize = storage_cdrom_finalize;
}

static void
storage_cdrom_finalize (GObject *object)
{
	GstDisksStorageCdrom *storage = GST_DISKS_STORAGE_CDROM (object);
	g_return_if_fail (GST_IS_DISKS_STORAGE_CDROM (storage));

	if (storage->priv) {
		g_free (storage->priv);
		storage->priv = NULL;
	}
	
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void 
storage_cdrom_setup_properties_widget (GstDisksStorage *storage)
{
	GstDisksStorageCdrom *cdrom;

	cdrom = GST_DISKS_STORAGE_CDROM (storage);

	gst_disks_gui_setup_cdrom_properties (cdrom);
}

GstDisksStorage*
gst_disks_storage_cdrom_new (void)
{
	GstDisksStorageCdrom *storage;
	
	storage = g_object_new (GST_TYPE_DISKS_STORAGE_CDROM, NULL);

	return GST_DISKS_STORAGE (storage);
}

static void
storage_cdrom_set_property (GObject *object, guint prop_id, const GValue *value,
			    GParamSpec *spec)
{
	GstDisksStorageCdrom *storage;

	g_return_if_fail (GST_IS_DISKS_STORAGE_CDROM (object));

	storage = GST_DISKS_STORAGE_CDROM (object);

	switch (prop_id) {
	case PROP_STATUS:
		storage->priv->status = g_value_get_enum (value);
		break;
	case PROP_PLAY_AUDIO:
		storage->priv->play_audio = g_value_get_boolean (value);
		break;
	case PROP_WRITE_CDR:
		storage->priv->write_cdr = g_value_get_boolean (value);
		break;
	case PROP_WRITE_CDRW:
		storage->priv->write_cdrw = g_value_get_boolean (value);
		break;
	case PROP_READ_DVD:
		storage->priv->read_dvd = g_value_get_boolean (value);
		break;
	case PROP_WRITE_DVDR:
		storage->priv->write_dvdr = g_value_get_boolean (value);
		break;
	case PROP_WRITE_DVDRAM:
		storage->priv->write_dvdram = g_value_get_boolean (value);
		break;
	default:
		break;
	}
}

static void
storage_cdrom_get_property (GObject *object, guint prop_id, GValue *value,
				GParamSpec *spec)
{
	GstDisksStorageCdrom *storage;

	g_return_if_fail (GST_IS_DISKS_STORAGE_CDROM (object));

	storage = GST_DISKS_STORAGE_CDROM (object);

	switch (prop_id) {
	case PROP_STATUS:
		g_value_set_enum (value, storage->priv->status);
		break;
	case PROP_PLAY_AUDIO:
		g_value_set_boolean (value, storage->priv->play_audio);
		break;
	case PROP_WRITE_CDR:
		g_value_set_boolean (value, storage->priv->write_cdr);
		break;
	case PROP_WRITE_CDRW:
		g_value_set_boolean (value, storage->priv->write_cdrw);
		break;
	case PROP_READ_DVD:
		g_value_set_boolean (value, storage->priv->read_dvd);
		break;
	case PROP_WRITE_DVDR:
		g_value_set_boolean (value, storage->priv->write_dvdr);
		break;
	case PROP_WRITE_DVDRAM:
		g_value_set_boolean (value, storage->priv->write_dvdram);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	}
}

gchar *
gst_disks_storage_cdrom_get_human_readable_status (GstCdromStatus status)
{
	gchar *states[] = {
		"No Disc Inserted", "Data Disc Inserted",
		"Audio Disc Inserted",
		"Audio and Data Disc Inserted",
		"Blank Disc Inserted"
	};

	return g_strdup (states[status]);
}

GstCdromStatus
gst_disks_storage_cdrom_get_status_from_name (const gchar *status)
{
	if (g_ascii_strcasecmp (status, "empty") == 0)
		return CDROM_STATUS_EMPTY;
	else if (g_ascii_strcasecmp (status, "data") == 0)
		return CDROM_STATUS_DATA;
	else if (g_ascii_strcasecmp (status, "audio") == 0)
		return CDROM_STATUS_AUDIO;
	else if (g_ascii_strcasecmp (status, "mixed") == 0)
		return CDROM_STATUS_MIXED;
	else /*if (g_ascii_strcasecmp (status, "blank") == 0)*/
		return CDROM_STATUS_BLANK;
}

gchar *
gst_disks_storage_cdrom_get_status (GstCdromStatus status)
{
	switch (status) {
	case CDROM_STATUS_EMPTY:
		return g_strdup ("empty");
	case CDROM_STATUS_DATA:
		return g_strdup ("data");
	case CDROM_STATUS_AUDIO:
		return g_strdup ("audio");
	case CDROM_STATUS_MIXED:
		return g_strdup ("mixed");
		/*case CDROM_STATUS_BLANK:*/
	default:
		return g_strdup ("blank");
	}
}
