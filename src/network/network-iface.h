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

#ifndef __GST_IFACE_H
#define __GST_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib-object.h>
#include <gdk/gdkpixbuf.h>
#include "gst-xml.h"

#define GST_TYPE_IFACE           (gst_iface_get_type ())
#define GST_IFACE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_IFACE, GstIface))
#define GST_IFACE_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_IFACE, GstIfaceClass))
#define GST_IS_IFACE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_IFACE))
#define GST_IS_IFACE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_IFACE))
#define GST_IFACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_IFACE, GstIfaceClass))

typedef struct _GstIface      GstIface;
typedef struct _GstIfaceClass GstIfaceClass;
typedef struct _GstIfacePriv  GstIfacePriv;

struct _GstIface
{
  GObject parent;

  GstIfacePriv *_priv;
};

struct _GstIfaceClass
{
  GObjectClass parent_class;

  const GdkPixbuf* (*get_iface_pixbuf) (GstIface*);
  const gchar* (*get_iface_type) (GstIface*);
  const gchar* (*get_iface_desc) (GstIface*);
  gboolean (*has_gateway) (GstIface*);
  void (*get_xml) (GstIface*, xmlNodePtr);
};

GType gst_iface_get_type (void);

void         gst_iface_set_config_from_xml (GstIface*, xmlNodePtr);

gboolean     gst_iface_get_auto (GstIface*);
void         gst_iface_set_auto (GstIface*, gboolean);

gboolean     gst_iface_get_enabled (GstIface*);
void         gst_iface_set_enabled (GstIface*, gboolean);

const gchar *gst_iface_get_dev (GstIface*);
void         gst_iface_set_dev (GstIface*, const gchar*);

const gchar *gst_iface_get_hwaddr (GstIface*);
void         gst_iface_set_hwaddr (GstIface*, const gchar*);

gboolean     gst_iface_enable (GstIface*);
void         gst_iface_disable (GstIface*);

void         gst_iface_get_xml (GstIface*, xmlNodePtr);
const GdkPixbuf *gst_iface_get_pixbuf (GstIface *iface);
gchar       *gst_iface_get_desc (GstIface *iface);

gboolean     gst_iface_is_configured (GstIface*);
gboolean     gst_iface_has_gateway (GstIface*);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GST_NETWORK_IFACE_H */
