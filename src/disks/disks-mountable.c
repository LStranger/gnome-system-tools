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

#include "disks-mountable.h"

static void mountable_base_init (gpointer g_class);

GType
gst_disks_mountable_get_type (void)
{
	   static GType type = 0;

	   if (!type) {
			 static const GTypeInfo info = {
				    sizeof (GstDisksMountableIface),
				    (GBaseInitFunc) mountable_base_init,
				    (GBaseFinalizeFunc) NULL,
				    (GClassInitFunc) NULL,
				    NULL,
				    NULL,
				    0,
				    0,
				    (GInstanceInitFunc) NULL
			 };
			 type = g_type_register_static (G_TYPE_INTERFACE, "GstDisksMountable",
									  &info, 0);
			 g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	   }
	   return type;
}

static void
mountable_base_init (gpointer g_class)
{
	   static gboolean initialized = FALSE;

	   if (!initialized)
	   {
			 initialized = TRUE;
	   }
}

void
gst_disks_mountable_mount (GstDisksMountable *mountable)
{
	   g_return_if_fail (GST_IS_DISKS_MOUNTABLE (mountable));

	   if (GST_DISKS_MOUNTABLE_GET_IFACE (mountable)->mount) {
			 return GST_DISKS_MOUNTABLE_GET_IFACE (mountable)->mount (mountable);
	   } else {
			 return;
	   }
}
