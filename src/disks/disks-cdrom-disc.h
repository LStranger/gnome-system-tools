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

#ifndef __GST_DISKS_CDROM_DISC_H__
#define __GST_DISKS_CDROM_DISC_H__

#include <glib-object.h>

#define GST_TYPE_CDROM_DISC         (gst_cdrom_disc_get_type ())
#define GST_CDROM_DISC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_CDROM_DISC, GstCdromDisc))
#define GST_CDROM_DISC_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_CDROM_DISC, GstCdromDiscClass))
#define GST_IS_CDROM_DISC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_CDROM_DISC))
#define GST_IS_CDROM_DISC_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_CDROM_DISC))
#define GST_CDROM_DISC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_CDROM_DISC, GstCdromDiscClass))

typedef struct _GstCdromDisc           GstCdromDisc;
typedef struct _GstCdromDiscClass      GstCdromDiscClass;

struct _GstCdromDisc {
	GObject        parent;

	gpointer cdrom; /* Bidirectional Asociation */
	
};

struct _GstCdromDiscClass {
	GObjectClass parent_class;
	
	void       (* setup_gui) (GstCdromDisc *disc);
};

GType      gst_cdrom_disc_get_type      (void);

void       gst_cdrom_disc_setup_gui (GstCdromDisc *disc);

void       gst_cdrom_disc_set_cdrom (GstCdromDisc *disc, gpointer cdrom);
gpointer   gst_cdrom_disc_get_cdrom (GstCdromDisc *disc);

#endif /* __GST_DISKS_CDROM_DISC_H__ */
