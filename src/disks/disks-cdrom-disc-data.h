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

#ifndef __GST_DISKS_CDROM_DISC_DATA_H__
#define __GST_DISKS_CDROM_DISC_DATA_H__

#include <glib-object.h>

#define GST_TYPE_CDROM_DISC_DATA         (gst_cdrom_disc_data_get_type ())
#define GST_CDROM_DISC_DATA(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_CDROM_DISC_DATA, GstCdromDiscData))
#define GST_CDROM_DISC_DATA_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_CDROM_DISC_DATA, GstCdromDiscDataClass))
#define GST_IS_CDROM_DISC_DATA(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_CDROM_DISC_DATA))
#define GST_IS_CDROM_DISC_DATA_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_CDROM_DISC_DATA))
#define GST_CDROM_DISC_DATA_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_CDROM_DISC_DATA, GstCdromDiscDataClass))

typedef struct _GstCdromDiscData      GstCdromDiscData;
typedef struct _GstCdromDiscDataClass GstCdromDiscDataClass;
typedef struct _GstCdromDiscDataPriv  GstCdromDiscDataPriv;

struct _GstCdromDiscData {
	GstCdromDisc      parent;
	
	GstCdromDiscDataPriv *priv;
};

struct _GstCdromDiscDataClass {
	GstCdromDiscClass parent_class;
};

GType            gst_cdrom_disc_data_get_type (void);
GstCdromDisc    *gst_cdrom_disc_data_new      (void);

void             gst_disks_cdrom_disc_data_browse (GstCdromDiscData *disc);


#endif /* __GST_DISKS_CDROM_DISC_DATA_H__ */
