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

#ifndef __GST_IFACE_ISDN_
#define __GST_IFACE_ISDN_

#ifdef cplusplus
extern "C" {
#endif

#include "network-iface.h"
#include "gst-xml.h"

#define GST_TYPE_IFACE_ISDN           (gst_iface_isdn_get_type ())
#define GST_IFACE_ISDN(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE_ISDN, GstIfaceIsdn))
#define GST_IFACE_ISDN_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE_ISDN, GstIfaceIsdnClass))
#define GST_IS_IFACE_ISDN(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE_ISDN))
#define GST_IS_IFACE_ISDN_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE_ISDN))
#define GST_IFACE_ISDN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE_ISDN, GstIfaceIsdnClass))

typedef struct _GstIfaceIsdn      GstIfaceIsdn;
typedef struct _GstIfaceIsdnClass GstIfaceIsdnClass;
typedef struct _GstIfaceIsdnPriv  GstIfaceIsdnPriv;

struct _GstIfaceIsdn
{
  GstIface parent;

  GstIfaceIsdnPriv *_priv;
};

struct _GstIfaceIsdnClass
{
  GstIfaceClass parent_class;
};

GType gst_iface_isdn_get_type ();

GstIfaceIsdn* gst_iface_isdn_new_from_xml (xmlNodePtr);
  
#ifdef cplusplus
}
#endif

#endif /* __GST_IFACE_ISDN_ */
