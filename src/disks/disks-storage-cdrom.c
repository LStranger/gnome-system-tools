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

#include <gnome.h>

#include "gst.h"
#include "disks-storage.h"
#include "disks-storage-cdrom.h"

extern GstTool *tool;

#define PARENT_TYPE GST_TYPE_DISKS_STORAGE

struct _GstDisksStorageCdromPriv
{
	gboolean automount;
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

static GtkWidget*  storage_cdrom_get_properties_widget   (GstDisksStorage *storage);
static void        storage_cdrom_setup_properties_widget (GstDisksStorage *storage);

static GObjectClass *parent_class = NULL;

enum {
	PROP_0,
	PROP_PLAY_AUDIO,
	PROP_WRITE_CDR,
	PROP_WRITE_CDRW,
	PROP_READ_DVD,
	PROP_WRITE_DVDR,
	PROP_WRITE_DVDRAM,
};


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

	storage_class->get_properties_widget   = storage_cdrom_get_properties_widget;
	storage_class->setup_properties_widget = storage_cdrom_setup_properties_widget;

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

static GtkWidget *
storage_cdrom_get_properties_widget (GstDisksStorage *storage)
{
	GtkWidget *widget;

	widget = gst_dialog_get_widget (tool->main_dialog, "cdrom_properties");
	
	return widget;
}

static void 
storage_cdrom_setup_properties_widget (GstDisksStorage *storage)
{
	GstDisksStorageCdrom *cdrom;
	GtkWidget *speed_label;
	gchar *speed;

	cdrom = GST_DISKS_STORAGE_CDROM (storage);

	speed_label = gst_dialog_get_widget (tool->main_dialog, "cdrom_speed_label");
	g_object_get (G_OBJECT (storage), "speed", &speed, NULL);
	if (speed == NULL) {
		gst_disks_gui_set_device_speed (storage);
	} else {
		gtk_label_set_text (GTK_LABEL (speed_label), speed);
	}
		

	if (cdrom->priv->play_audio) {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "play_audio_image")),
			GTK_STOCK_YES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "play_audio_image")),
			GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}
	
	if (cdrom->priv->write_cdr) {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_cdr_image")),
			GTK_STOCK_YES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_cdr_image")),
			GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}

	if (cdrom->priv->write_cdrw) {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_cdrw_image")),
			GTK_STOCK_YES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_cdrw_image")),
			GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}

	if (cdrom->priv->read_dvd) {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "read_dvd_image")),
			GTK_STOCK_YES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "read_dvd_image")),
			GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}

	if (cdrom->priv->write_dvdr) {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_dvdr_image")),
			GTK_STOCK_YES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_dvdr_image")),
			GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}

	if (cdrom->priv->write_dvdram) {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_dvdram_image")),
			GTK_STOCK_YES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		gtk_image_set_from_stock (
			GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_dvdram_image")),
			GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR);
	}
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
