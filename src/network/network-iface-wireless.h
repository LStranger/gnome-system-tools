/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifndef __GST_IFACE_WIRELESS_H
#define __GST_IFACE_WIRELESS_H

#include "network-iface-ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GST_TYPE_IFACE_WIRELESS           (gst_iface_wireless_get_type ())
#define GST_IFACE_WIRELESS(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE_WIRELESS, GstIfaceWireless))
#define GST_IFACE_WIRELESS_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE_WIRELESS, GstIfaceWirelessClass))
#define GST_IS_IFACE_WIRELESS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE_WIRELESS))
#define GST_IS_IFACE_WIRELESS_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE_WIRELESS))
#define GST_IFACE_WIRELESS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE_WIRELESS, GstIfaceWirelessClass))

typedef struct _GstIfaceWireless      GstIfaceWireless;
typedef struct _GstIfaceWirelessClass GstIfaceWirelessClass;
typedef struct _GstIfaceWirelessPriv  GstIfaceWirelessPriv;

struct _GstIfaceWireless
{
  GstIfaceEthernet parent;

  GstIfaceWirelessPriv *_priv;
};

struct _GstIfaceWirelessClass
{
  GstIfaceEthernetClass parent_class;
};

GType gst_iface_wireless_get_type (void);

GstIfaceWireless* gst_iface_wireless_new_from_xml (xmlNodePtr);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GST_IFACE_WIRELESS_H */
