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

#ifndef __GST_DISKS_CDROM_DISC_DVD_H__
#define __GST_DISKS_CDROM_DISC_DVD_H__

#include <glib-object.h>

#define GST_TYPE_CDROM_DISC_DVD         (gst_cdrom_disc_dvd_get_type ())
#define GST_CDROM_DISC_DVD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_CDROM_DISC_DVD, GstCdromDiscDvd))
#define GST_CDROM_DISC_DVD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GST_TYPE_CDROM_DISC_DVD, GstCdromDiscDvdClass))
#define GST_IS_CDROM_DISC_DVD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_CDROM_DISC_DVD))
#define GST_IS_CDROM_DISC_DVD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GST_TYPE_CDROM_DISC_DVD))
#define GST_CDROM_DISC_DVD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GST_TYPE_CDROM_DISC_DVD, GstCdromDiscDvdClass))

typedef struct _GstCdromDiscDvd      GstCdromDiscDvd;
typedef struct _GstCdromDiscDvdClass GstCdromDiscDvdClass;
typedef struct _GstCdromDiscDvdPriv  GstCdromDiscDvdPriv;

struct _GstCdromDiscDvd {
	   GstCdromDisc      parent;
	   
	   GstCdromDiscDvdPriv *priv;
};

struct _GstCdromDiscDvdClass {
	   GstCdromDiscClass parent_class;
};

GType            gst_cdrom_disc_dvd_get_type (void);
GstCdromDisc    *gst_cdrom_disc_dvd_new      (void);

void             gst_disks_cdrom_disc_dvd_play (GstCdromDiscDvd *disc);

#endif /* __GST_DISKS_CDROM_DISC_DVD_H__ */
