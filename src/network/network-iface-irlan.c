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

#include <glib.h>
#include <gdk/gdkpixbuf.h>
#include "network-iface-irlan.h"
#include "gst.h"

struct _GstIfaceIrlanPriv {
  GdkPixbuf *pixbuf;
};

static void gst_iface_irlan_class_init (GstIfaceIrlanClass *class);
static void gst_iface_irlan_init       (GstIfaceIrlan      *iface);
static void gst_iface_irlan_finalize   (GObject *object);

static const GdkPixbuf* gst_iface_irlan_get_pixbuf     (GstIface *iface);
static const gchar*     gst_iface_irlan_get_iface_type (GstIface *iface);
static const gchar*     gst_iface_irlan_get_desc       (GstIface *iface);

static gpointer parent_class;

GType
gst_iface_irlan_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_irlan_info =
        {
	  sizeof (GstIfaceIrlanClass),
	  NULL,		/* base_init */
	  NULL,		/* base_finalize */
	  (GClassInitFunc) gst_iface_irlan_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GstIfaceIrlan),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) gst_iface_irlan_init,
	};

      type = g_type_register_static (GST_TYPE_IFACE_ETHERNET,
				     "GstIfaceIrlan",
				     &iface_irlan_info, 0);
    }

  return type;
}

static void
gst_iface_irlan_class_init (GstIfaceIrlanClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GstIfaceClass *iface_class = GST_IFACE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->finalize        = gst_iface_irlan_finalize;
  iface_class->get_iface_pixbuf = gst_iface_irlan_get_pixbuf;
  iface_class->get_iface_type   = gst_iface_irlan_get_iface_type;
  iface_class->get_iface_desc   = gst_iface_irlan_get_desc;
}

static void
gst_iface_irlan_init (GstIfaceIrlan *iface)
{
  g_return_if_fail (GST_IS_IFACE_IRLAN (iface));

  iface->_priv = g_new0 (GstIfaceIrlanPriv, 1);
  iface->_priv->pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/irda-48.png", NULL);
}

static void
gst_iface_irlan_finalize (GObject *object)
{
  GstIfaceIrlan *iface = GST_IFACE_IRLAN (object);

  g_return_if_fail (GST_IS_IFACE_IRLAN (iface));

  if (iface->_priv)
    {
      gdk_pixbuf_unref (iface->_priv->pixbuf);

      g_free (iface->_priv);
      iface->_priv = NULL;
    }
  
  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static const GdkPixbuf*
gst_iface_irlan_get_pixbuf (GstIface *iface)
{
  return GST_IFACE_IRLAN (iface)->_priv->pixbuf;
}

static const gchar*
gst_iface_irlan_get_iface_type (GstIface *iface)
{
  return "irlan";
}

static const gchar*
gst_iface_irlan_get_desc (GstIface *iface)
{
  return _("Infrared connection");
}

GstIfaceIrlan*
gst_iface_irlan_new_from_xml (xmlNodePtr node)
{
  GstIfaceIrlan *iface;

  g_return_val_if_fail (node != NULL, NULL);

  iface = g_object_new (GST_TYPE_IFACE_IRLAN, NULL);
  gst_iface_ethernet_set_config_from_xml (GST_IFACE_ETHERNET (iface), node);

  return iface;
}
