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

#ifndef __GST_IFACE_ETHERNET_H
#define __GST_IFACE_ETHERNET_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "network-iface.h"
#include "gst-xml.h"

typedef enum {
  GST_BOOTPROTO_NONE,
  GST_BOOTPROTO_STATIC,
  GST_BOOTPROTO_DHCP
} GstBootProto;

#define GST_TYPE_BOOTPROTO                (gst_bootproto_get_type ())
#define GST_TYPE_IFACE_ETHERNET           (gst_iface_ethernet_get_type ())
#define GST_IFACE_ETHERNET(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE_ETHERNET, GstIfaceEthernet))
#define GST_IFACE_ETHERNET_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE_ETHERNET, GstIfaceEthernetClass))
#define GST_IS_IFACE_ETHERNET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE_ETHERNET))
#define GST_IS_IFACE_ETHERNET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE_ETHERNET))
#define GST_IFACE_ETHERNET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE_ETHERNET, GstIfaceEthernetClass))

typedef struct _GstIfaceEthernet      GstIfaceEthernet;
typedef struct _GstIfaceEthernetClass GstIfaceEthernetClass;
typedef struct _GstIfaceEthernetPriv  GstIfaceEthernetPriv;

struct _GstIfaceEthernet
{
  GstIface parent;

  GstIfaceEthernetPriv *_priv;
};

struct _GstIfaceEthernetClass
{
  GstIfaceClass parent_class;
};

GType gst_bootproto_get_type (void);
GType gst_iface_ethernet_get_type (void);

GstIfaceEthernet* gst_iface_ethernet_new_from_xml (xmlNodePtr);
void get_iface_ethernet_set_config_from_xml (GstIfaceEthernet *, xmlNodePtr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GST_IFACE_ETHERNET_H */
