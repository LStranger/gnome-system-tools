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

#include "disks-storage-cdrom.h"
#include "disks-cdrom-disc.h"

#define PARENT_TYPE G_TYPE_OBJECT

static void cdrom_disc_init       (GstCdromDisc      *disc);
static void cdrom_disc_class_init (GstCdromDiscClass *klass);
static void cdrom_disc_finalize   (GObject           *object);

static GObjectClass *parent_class = NULL;

GType
gst_cdrom_disc_get_type (void)
{
	static GType type = 0;
	
	if (!type) {
		static const GTypeInfo info = {
			sizeof (GstCdromDiscClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) cdrom_disc_class_init,
			NULL,
			NULL,
			sizeof (GstCdromDisc),
			0,
			(GInstanceInitFunc) cdrom_disc_init
		};
		type = g_type_register_static (PARENT_TYPE, "GstCdromDisc",
					       &info, G_TYPE_FLAG_ABSTRACT);
	}
	return type;
}

static void
cdrom_disc_init (GstCdromDisc *disc)
{
	g_return_if_fail (GST_IS_CDROM_DISC (disc));

	disc->cdrom = NULL;
}

static void
cdrom_disc_class_init (GstCdromDiscClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = cdrom_disc_finalize;
}

static void
cdrom_disc_finalize (GObject *object)
{
	GstCdromDisc *disc = GST_CDROM_DISC (object);
	g_return_if_fail (GST_IS_CDROM_DISC (disc));
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

void
gst_cdrom_disc_setup_gui (GstCdromDisc *disc)
{
	g_return_if_fail (GST_IS_CDROM_DISC (disc));
	
	if (GST_CDROM_DISC_GET_CLASS (disc)->setup_gui) {
		return GST_CDROM_DISC_GET_CLASS (disc)->setup_gui (disc);
	} else {
		return;
	}
}

void
gst_cdrom_disc_set_cdrom (GstCdromDisc *disc, gpointer cdrom)
{
	g_return_if_fail (GST_IS_CDROM_DISC (disc));

	disc->cdrom = cdrom;
}

gpointer
gst_cdrom_disc_get_cdrom (GstCdromDisc *disc)
{
	g_return_val_if_fail (GST_IS_CDROM_DISC (disc), NULL);

	return (gpointer) disc->cdrom;
}
