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

#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-cdrom-disc-audio.h"
#include "disks-cdrom-disc-mixed.h"
#include "disks-gui.h"

#define PARENT_TYPE GST_TYPE_CDROM_DISC

struct _GstCdromDiscMixedPriv
{
	   GstCdromDiscData  *data;
	   GstCdromDiscAudio *audio;
};

static void cdrom_disc_mixed_init                    (GstCdromDiscMixed      *disc);
static void cdrom_disc_mixed_class_init              (GstCdromDiscMixedClass *klass);
static void cdrom_disc_mixed_finalize                (GObject                *object);

static void cdrom_disc_mixed_set_property (GObject *object, guint prop_id,
								   const GValue *value, GParamSpec *spec);
static void cdrom_disc_mixed_get_property (GObject *object, guint prop_id,
								   GValue *value, GParamSpec *spec);

static void cdrom_disc_mixed_setup_gui (GstCdromDisc *disc);

static GObjectClass *parent_class = NULL;

enum {
	   PROP_0,
	   PROP_DATA,
	   PROP_AUDIO
};

GType
gst_cdrom_disc_mixed_get_type (void)
{
	   static GType type = 0;

	   if (!type) {
			 static const GTypeInfo info = {
				    sizeof (GstCdromDiscMixedClass),
				    (GBaseInitFunc) NULL,
				    (GBaseFinalizeFunc) NULL,
				    (GClassInitFunc) cdrom_disc_mixed_class_init,
				    NULL,
				    NULL,
				    sizeof (GstCdromDiscMixed),
				    0,
				    (GInstanceInitFunc) cdrom_disc_mixed_init
			 };
			 type = g_type_register_static (PARENT_TYPE, "GstCdromDiscMixed",
									  &info, 0);
	   }
	   return type;
}

static void
cdrom_disc_mixed_init (GstCdromDiscMixed *disc)
{
	   g_return_if_fail (GST_IS_CDROM_DISC_MIXED (disc));

	   disc->priv = g_new0 (GstCdromDiscMixedPriv, 1);
	   disc->priv->data  = GST_CDROM_DISC_DATA (gst_cdrom_disc_data_new ());
	   disc->priv->audio = GST_CDROM_DISC_AUDIO (gst_cdrom_disc_audio_new ());
}

static void
cdrom_disc_mixed_class_init (GstCdromDiscMixedClass *klass)
{
	   GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
	   GstCdromDiscClass    *cdrom_disc_class = GST_CDROM_DISC_CLASS (klass);

	   parent_class = g_type_class_peek_parent (klass);

	   object_class->set_property = cdrom_disc_mixed_set_property;
	   object_class->get_property = cdrom_disc_mixed_get_property;

	   cdrom_disc_class->setup_gui = cdrom_disc_mixed_setup_gui;

	   g_object_class_install_property (object_class, PROP_DATA,
								 g_param_spec_pointer ("data", NULL, NULL,
												   G_PARAM_READWRITE));
	   g_object_class_install_property (object_class, PROP_AUDIO,
								 g_param_spec_pointer ("audio", NULL, NULL,
												   G_PARAM_READWRITE));

	   object_class->finalize = cdrom_disc_mixed_finalize;
}

static void
cdrom_disc_mixed_finalize (GObject *object)
{
	   GstCdromDiscMixed *disc = GST_CDROM_DISC_MIXED (object);
	   g_return_if_fail (GST_IS_CDROM_DISC_MIXED (disc));

	   if (disc->priv) {
			 if (disc->priv->data) {
				    g_object_unref (G_OBJECT (disc->priv->data));
				    disc->priv->data = NULL;
			 }

			 if (disc->priv->audio) {
				    g_object_unref (G_OBJECT (disc->priv->audio));
				    disc->priv->audio = NULL;
			 }
			 
			 g_free (disc->priv);
			 disc->priv = NULL;
	   }


	   if (G_OBJECT_CLASS (parent_class)->finalize)
			 (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
cdrom_disc_mixed_setup_gui (GstCdromDisc *disc)
{
	   GstCdromDiscMixed *disc_mixed;

	   disc_mixed = GST_CDROM_DISC_MIXED (disc);

	   gst_disks_gui_setup_cdrom_disc_mixed (disc_mixed);
}

void
gst_disks_cdrom_disc_mixed_browse (GstCdromDiscMixed *disc)
{
	   if (disc->priv->data) 
			 gst_disks_cdrom_disc_data_browse (disc->priv->data);
}

void
gst_disks_cdrom_disc_mixed_play (GstCdromDiscMixed *disc, const gchar *device)
{
	   if (disc->priv->audio)
			 gst_disks_cdrom_disc_audio_play (disc->priv->audio, device);
}

GstCdromDisc *
gst_cdrom_disc_mixed_new (void)
{
	   GstCdromDiscMixed *disc;

	   disc = g_object_new (GST_TYPE_CDROM_DISC_MIXED, NULL);

	   return GST_CDROM_DISC (disc);
}

static void
cdrom_disc_mixed_set_property (GObject *object, guint prop_id, const GValue *value,
						 GParamSpec *spec)
{
	   GstCdromDiscMixed *disc;

	   g_return_if_fail (GST_IS_CDROM_DISC_MIXED (object));

	   disc = GST_CDROM_DISC_MIXED (object);

	   switch (prop_id) {
	   case PROP_DATA:
			 disc->priv->data = g_value_get_pointer (value);
			 break;
	   case PROP_AUDIO:
			 disc->priv->audio = g_value_get_pointer (value);
			 break;
	   default:
			 break;
	   }
}

static void
cdrom_disc_mixed_get_property (GObject *object, guint prop_id, GValue *value,
						 GParamSpec *spec)
{
	   GstCdromDiscMixed *disc;

	   g_return_if_fail (GST_IS_CDROM_DISC_MIXED (object));

	   disc = GST_CDROM_DISC_MIXED (object);

	   switch (prop_id) {
	   case PROP_DATA:
			 g_value_set_pointer (value, disc->priv->data);
			 break;
	   case PROP_AUDIO:
			 g_value_set_pointer (value, disc->priv->audio);
			 break;
	   default:
			 G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	   }
}
