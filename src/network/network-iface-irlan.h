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

#ifndef __GST_IFACE_IRLAN_H
#define __GST_IFACE_IRLAN_H

#include "network-iface-ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GST_TYPE_IFACE_IRLAN           (gst_iface_irlan_get_type ())
#define GST_IFACE_IRLAN(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE_IRLAN, GstIfaceIrlan))
#define GST_IFACE_IRLAN_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE_IRLAN, GstIfaceIrlanClass))
#define GST_IS_IFACE_IRLAN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE_IRLAN))
#define GST_IS_IFACE_IRLAN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE_IRLAN))
#define GST_IFACE_IRLAN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE_IRLAN, GstIfaceIrlanClass))

typedef struct _GstIfaceIrlan      GstIfaceIrlan;
typedef struct _GstIfaceIrlanClass GstIfaceIrlanClass;
typedef struct _GstIfaceIrlanPriv  GstIfaceIrlanPriv;

struct _GstIfaceIrlan
{
  GstIfaceEthernet parent;

  GstIfaceIrlanPriv *_priv;
};

struct _GstIfaceIrlanClass
{
  GstIfaceEthernetClass parent_class;
};

GType gst_iface_irlan_get_type (void);

GstIfaceIrlan* gst_iface_irlan_new_from_xml (xmlNodePtr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GST_IFACE_IRLAN_H */
