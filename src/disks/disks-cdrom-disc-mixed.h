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

#ifndef __GST_DISKS_CDROM_DISC_MIXED_H__
#define __GST_DISKS_CDROM_DISC_MIXED_H__

#include <glib-object.h>

#define GST_TYPE_CDROM_DISC_MIXED         (gst_cdrom_disc_mixed_get_type ())
#define GST_CDROM_DISC_MIXED(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_CDROM_DISC_MIXED, GstCdromDiscMixed))
#define GST_CDROM_DISC_MIXED_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_CDROM_DISC_MIXED, GstCdromDiscMixedClass))
#define GST_IS_CDROM_DISC_MIXED(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_CDROM_DISC_MIXED))
#define GST_IS_CDROM_DISC_MIXED_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_CDROM_DISC_MIXED))
#define GST_CDROM_DISC_MIXED_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_CDROM_DISC_MIXED, GstCdromDiscMixedClass))

typedef struct _GstCdromDiscMixed      GstCdromDiscMixed;
typedef struct _GstCdromDiscMixedClass GstCdromDiscMixedClass;
typedef struct _GstCdromDiscMixedPriv  GstCdromDiscMixedPriv;

struct _GstCdromDiscMixed {
	   GstCdromDisc      parent;

	   GstCdromDiscMixedPriv *priv;
};

struct _GstCdromDiscMixedClass {
	   GstCdromDiscClass parent_class;
};

GType            gst_cdrom_disc_mixed_get_type (void);
GstCdromDisc    *gst_cdrom_disc_mixed_new      (void);

void             gst_disks_cdrom_disc_mixed_browse (GstCdromDiscMixed *disc);
void             gst_disks_cdrom_disc_mixed_play   (GstCdromDiscMixed *disc, const gchar *device);

#endif /* __GST_DISKS_CDROM_DISC_MIXED_H__ */
