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
#include "network-iface-plip.h"
#include "network-iface.h"
#include "gst.h"

struct _GstIfacePlipPriv
{
  GdkPixbuf *pixbuf;
  gchar     *address;
  gchar     *remote_address;
};

static void gst_iface_plip_class_init (GstIfacePlipClass *class);
static void gst_iface_plip_init       (GstIfacePlip      *iface);
static void gst_iface_plip_finalize   (GObject           *object);

static const GdkPixbuf* gst_iface_plip_get_pixbuf     (GstIface *iface);
static const gchar*     gst_iface_plip_get_iface_type (GstIface *iface);
static const gchar*     gst_iface_plip_get_desc       (GstIface *iface);
static gboolean         gst_iface_plip_has_gateway    (GstIface *iface);
static void             gst_iface_plip_impl_get_xml   (GstIface *iface,
						       xmlNodePtr node);

static void gst_iface_plip_set_property (GObject      *object,
					 guint         prop_id,
					 const GValue *value,
					 GParamSpec   *pspec);
static void gst_iface_plip_get_property (GObject      *object,
					 guint         prop_id,
					 GValue       *value,
					 GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_ADDRESS,
  PROP_REMOTE_ADDRESS,
};

static gpointer parent_class;

GType
gst_iface_plip_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_plip_info =
	{
	  sizeof (GstIfacePlipClass),
	  NULL,		/* base_init */
	  NULL,		/* base_finalize */
	  (GClassInitFunc) gst_iface_plip_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GstIfacePlip),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) gst_iface_plip_init,
	};

      type = g_type_register_static (GST_TYPE_IFACE, "GstIfacePlip",
				     &iface_plip_info, 0);
    }

  return type;
}

static void
gst_iface_plip_class_init (GstIfacePlipClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GstIfaceClass *iface_class = GST_IFACE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->set_property = gst_iface_plip_set_property;
  object_class->get_property = gst_iface_plip_get_property;
  object_class->finalize     = gst_iface_plip_finalize;

  iface_class->get_iface_pixbuf = gst_iface_plip_get_pixbuf;
  iface_class->get_iface_type   = gst_iface_plip_get_iface_type;
  iface_class->get_iface_desc   = gst_iface_plip_get_desc;
  iface_class->has_gateway      = gst_iface_plip_has_gateway;
  iface_class->get_xml          = gst_iface_plip_impl_get_xml;

  g_object_class_install_property (object_class,
				   PROP_ADDRESS,
				   g_param_spec_string ("iface_local_address",
							"Iface address",
							"Address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_REMOTE_ADDRESS,
				   g_param_spec_string ("iface_remote_address",
							"Iface remote address",
							"Remote address for the iface",
							NULL,
							G_PARAM_READWRITE));
}

static void
gst_iface_plip_init (GstIfacePlip *iface)
{
  g_return_if_fail (GST_IS_IFACE_PLIP (iface));

  iface->_priv = g_new0 (GstIfacePlipPriv, 1);
  iface->_priv->pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/plip-48.png", NULL);
  iface->_priv->address = NULL;
  iface->_priv->remote_address = NULL;
}

static void
gst_iface_plip_finalize (GObject *object)
{
  GstIfacePlip *iface = GST_IFACE_PLIP (object);

  g_return_if_fail (GST_IS_IFACE_PLIP (iface));

  if (iface->_priv)
    {
      gdk_pixbuf_unref (iface->_priv->pixbuf);
      g_free (iface->_priv->address);
      g_free (iface->_priv->remote_address);

      g_free (iface->_priv);
      iface->_priv = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_iface_plip_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  GstIfacePlip *iface = GST_IFACE_PLIP (object);

  g_return_if_fail (GST_IS_IFACE_PLIP (iface));

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_free (iface->_priv->address);
      iface->_priv->address = g_value_dup_string (value);
      break;
    case PROP_REMOTE_ADDRESS:
      g_free (iface->_priv->remote_address);
      iface->_priv->remote_address = g_value_dup_string (value);
      break;
    }
}

static void
gst_iface_plip_get_property (GObject      *object,
			     guint         prop_id,
			     GValue       *value,
			     GParamSpec   *pspec)
{
  GstIfacePlip *iface = GST_IFACE_PLIP (object);

  g_return_if_fail (GST_IS_IFACE_PLIP (iface));

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_value_set_string (value, iface->_priv->address);
      break;
    case PROP_REMOTE_ADDRESS:
      g_value_set_string (value, iface->_priv->remote_address);
      break;
    }
}

static const GdkPixbuf*
gst_iface_plip_get_pixbuf (GstIface *iface)
{
  return GST_IFACE_PLIP (iface)->_priv->pixbuf;
}

static const gchar*
gst_iface_plip_get_iface_type (GstIface *iface)
{
  return "plip";
}

static const gchar*
gst_iface_plip_get_desc (GstIface *iface)
{
  return _("Parallel port connection");
}

static gboolean
gst_iface_plip_has_gateway (GstIface *iface)
{
  return TRUE;
}

static void
gst_iface_plip_impl_get_xml (GstIface *iface, xmlNodePtr node)
{
  xmlNodePtr configuration;
  GstIfacePlip *iface_plip;

  g_return_if_fail (GST_IS_IFACE_PLIP (iface));
  iface_plip = GST_IFACE_PLIP (iface);

  if (gst_iface_is_configured (iface))
    {
      configuration = gst_xml_element_find_first (node, "configuration");
      if (!configuration)
        configuration = gst_xml_element_add (node, "configuration");

      gst_xml_set_child_content (configuration, "bootproto", "none");
      gst_xml_set_child_content (configuration, "address", iface_plip->_priv->address);
      gst_xml_set_child_content (configuration, "remote_address", iface_plip->_priv->remote_address);
      gst_xml_set_child_content (configuration, "gateway", iface_plip->_priv->remote_address);
    }

  GST_IFACE_CLASS (parent_class)->get_xml (iface, node);
}

void
gst_iface_plip_set_config_from_xml (GstIfacePlip *iface,
				    xmlNodePtr    node)
{
  xmlNodePtr configuration;
  gchar *address, *remote_address;

  g_return_if_fail (iface != NULL);
  g_return_if_fail (node != NULL);

  /* config the parent class data */
  gst_iface_set_config_from_xml (GST_IFACE (iface), node);

  configuration = gst_xml_element_find_first (node, "configuration");
  if (!configuration)
    return;

  address = gst_xml_get_child_content (configuration, "address");
  remote_address = gst_xml_get_child_content (configuration, "remote_address");

  g_object_set (G_OBJECT (iface),
		"iface-local-address",  address,
		"iface-remote-address", remote_address,
		NULL);

  g_free (address);
  g_free (remote_address);
}

GstIfacePlip*
gst_iface_plip_new_from_xml (xmlNodePtr node)
{
  GstIfacePlip *iface;

  iface = g_object_new (GST_TYPE_IFACE_PLIP, NULL);
  gst_iface_plip_set_config_from_xml (iface, node);

  return iface;
}
