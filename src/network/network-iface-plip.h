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

#ifndef __GST_IFACE_PLIP_H
#define __GST_IFACE_PLIP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "network-iface.h"
#include "gst-xml.h"

#define GST_TYPE_IFACE_PLIP           (gst_iface_plip_get_type ())
#define GST_IFACE_PLIP(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE_PLIP, GstIfacePlip))
#define GST_IFACE_PLIP_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE_PLIP, GstIfacePlipClass))
#define GST_IS_IFACE_PLIP(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE_PLIP))
#define GST_IS_IFACE_PLIP_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE_PLIP))
#define GST_IFACE_PLIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE_PLIP, GstIfacePlipClass))

typedef struct _GstIfacePlip      GstIfacePlip;
typedef struct _GstIfacePlipClass GstIfacePlipClass;
typedef struct _GstIfacePlipPriv  GstIfacePlipPriv;

struct _GstIfacePlip
{
  GstIface parent;

  GstIfacePlipPriv *_priv;
};

struct _GstIfacePlipClass
{
  GstIfaceClass parent_class;
};

GType gst_iface_plip_get_type (void);

GstIfacePlip* gst_iface_plip_new_from_xml (xmlNodePtr);
void get_iface_plip_set_config_from_xml (GstIfacePlip *, xmlNodePtr);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GST_IFACE_PLIP_H */
