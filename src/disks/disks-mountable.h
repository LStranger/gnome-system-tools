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

#ifndef __DISKS_MOUNTABLE_H__
#define __DISKS_MOUNTABLE_H__

#include <glib-object.h>

#define GST_TYPE_DISKS_MOUNTABLE          (gst_disks_mountable_get_type ())
#define GST_DISKS_MOUNTABLE(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), GST_TYPE_DISKS_MOUNTABLE, GstDisksMountable))
#define GST_IS_DISKS_MOUNTABLE(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), GST_TYPE_DISKS_MOUNTABLE))
#define GST_DISKS_MOUNTABLE_GET_IFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), GST_TYPE_DISKS_MOUNTABLE, GstDisksMountableIface))

typedef struct _GstDisksMountable      GstDisksMountable;
typedef struct _GstDisksMountableIface GstDisksMountableIface;

struct _GstDisksMountableIface {
	   GTypeInterface  g_iface;

	   void (* mount) (GstDisksMountable *mountable);
};

GType gst_disks_mountable_get_type (void);

void  gst_disks_mountable_mount (GstDisksMountable *mountable);

#endif /* __DISKS_MOUNTABLE_H__ */
