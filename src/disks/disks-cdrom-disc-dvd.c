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
#include "disks-cdrom-disc-dvd.h"
#include "disks-gui.h"

#define PARENT_TYPE GST_TYPE_CDROM_DISC

struct _GstCdromDiscDvdPriv
{
};

static void cdrom_disc_dvd_init                    (GstCdromDiscDvd      *disc);
static void cdrom_disc_dvd_class_init              (GstCdromDiscDvdClass *klass);
static void cdrom_disc_dvd_finalize                (GObject              *object);

/*static void cdrom_disc_dvd_set_property (GObject *object, guint prop_id,
								 const GValue *value, GParamSpec *spec);
static void cdrom_disc_dvd_get_property (GObject *object, guint prop_id,
GValue *value, GParamSpec *spec);*/

static void cdrom_disc_dvd_setup_gui (GstCdromDisc *disc);

static GObjectClass *parent_class = NULL;

/*enum {
	   PROP_0,
	   PROP_NUM_TRACKS,
	   PROP_DURATION
	   };*/

GType
gst_cdrom_disc_dvd_get_type (void)
{
	   static GType type = 0;

	   if (!type) {
			 static const GTypeInfo info = {
				    sizeof (GstCdromDiscDvdClass),
				    (GBaseInitFunc) NULL,
				    (GBaseFinalizeFunc) NULL,
				    (GClassInitFunc) cdrom_disc_dvd_class_init,
				    NULL,
				    NULL,
				    sizeof (GstCdromDiscDvd),
				    0,
				    (GInstanceInitFunc) cdrom_disc_dvd_init
			 };
			 type = g_type_register_static (PARENT_TYPE, "GstCdromDiscDvd",
									  &info, 0);
	   }
	   return type;
}

static void
cdrom_disc_dvd_init (GstCdromDiscDvd *disc)
{
	   g_return_if_fail (GST_IS_CDROM_DISC_DVD (disc));

	   disc->priv = g_new0 (GstCdromDiscDvdPriv, 1);
}

static void
cdrom_disc_dvd_class_init (GstCdromDiscDvdClass *klass)
{
	   GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
	   GstCdromDiscClass    *cdrom_disc_class = GST_CDROM_DISC_CLASS (klass);

	   parent_class = g_type_class_peek_parent (klass);

	   /*object_class->set_property = cdrom_disc_dvd_set_property;
		object_class->get_property = cdrom_disc_dvd_get_property;*/

	   cdrom_disc_class->setup_gui = cdrom_disc_dvd_setup_gui;

	   object_class->finalize = cdrom_disc_dvd_finalize;
}

static void
cdrom_disc_dvd_finalize (GObject *object)
{
	   GstCdromDiscDvd *disc = GST_CDROM_DISC_DVD (object);
	   g_return_if_fail (GST_IS_CDROM_DISC_DVD (disc));

	   if (disc->priv) {
			 g_free (disc->priv);
			 disc->priv = NULL;
	   }


	   if (G_OBJECT_CLASS (parent_class)->finalize)
			 (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
cdrom_disc_dvd_setup_gui (GstCdromDisc *disc)
{
	   GstCdromDiscDvd *disc_dvd;

	   disc_dvd = GST_CDROM_DISC_DVD (disc);

	   gst_disks_gui_setup_cdrom_disc_dvd (disc_dvd);
}

void
gst_disks_cdrom_disc_dvd_play (GstCdromDiscDvd *disc)
{
	   gchar *player;
	   gchar *command;

	   g_return_if_fail (GST_IS_CDROM_DISC_DVD (disc));

	   if ((player = g_find_program_in_path ("totem"))) {
			 command = g_strdup_printf ("%s dvd://", player);
			 g_spawn_command_line_async (command, NULL);
			 g_free (player);
			 g_free (command);
	   }
}

GstCdromDisc *
gst_cdrom_disc_dvd_new (void)
{
	   GstCdromDiscDvd *disc;

	   disc = g_object_new (GST_TYPE_CDROM_DISC_DVD, NULL);

	   return GST_CDROM_DISC (disc);
}

/*static void
cdrom_disc_audio_set_property (GObject *object, guint prop_id, const GValue *value,
						 GParamSpec *spec)
{
	   GstCdromDiscAudio *disc;

	   g_return_if_fail (GST_IS_CDROM_DISC_AUDIO (object));

	   disc = GST_CDROM_DISC_AUDIO (object);

	   switch (prop_id) {
	   case PROP_NUM_TRACKS:
			 disc->priv->num_tracks = g_value_get_uint (value);
			 break;
	   case PROP_DURATION:
			 if (disc->priv->duration) g_free (disc->priv->duration);
			 disc->priv->duration = g_value_dup_string (value);
			 break;
	   default:
			 break;
	   }
}

static void
cdrom_disc_audio_get_property (GObject *object, guint prop_id, GValue *value,
						 GParamSpec *spec)
{
	   GstCdromDiscAudio *disc;

	   g_return_if_fail (GST_IS_CDROM_DISC_AUDIO (object));

	   disc = GST_CDROM_DISC_AUDIO (object);

	   switch (prop_id) {
	   case PROP_NUM_TRACKS:
			 g_value_set_uint (value, disc->priv->num_tracks);
			 break;
	   case PROP_DURATION:
			 g_value_set_string (value, disc->priv->duration);
			 break;
	   default:
			 G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	   }
}*/

