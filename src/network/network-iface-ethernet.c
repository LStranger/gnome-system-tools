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
#include "network-iface-ethernet.h"
#include "network-iface.h"
#include "gst.h"

struct _GstIfaceEthernetPriv
{
  GdkPixbuf *pixbuf;
  gchar     *address;
  gchar     *netmask;
  gchar     *gateway;

  gchar     *network;
  gchar     *broadcast;

  GstBootProto bootproto;
};

static void gst_iface_ethernet_class_init (GstIfaceEthernetClass *class);
static void gst_iface_ethernet_init       (GstIfaceEthernet      *iface);
static void gst_iface_ethernet_finalize   (GObject               *object);

static const GdkPixbuf* gst_iface_ethernet_get_pixbuf     (GstIface *iface);
static const gchar*     gst_iface_ethernet_get_iface_type (GstIface *iface);
static const gchar*     gst_iface_ethernet_get_desc       (GstIface *iface);
static gboolean         gst_iface_ethernet_has_gateway    (GstIface *iface);
static void             gst_iface_ethernet_impl_get_xml   (GstIface *iface,
                                                           xmlNodePtr node);

static void gst_iface_ethernet_set_property (GObject      *object,
					     guint         prop_id,
					     const GValue *value,
					     GParamSpec   *pspec);
static void gst_iface_ethernet_get_property (GObject      *object,
					     guint         prop_id,
					     GValue       *value,
					     GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_ADDRESS,
  PROP_NETMASK,
  PROP_GATEWAY,
  PROP_NETWORK,
  PROP_BROADCAST,
  PROP_BOOTPROTO
};

static gpointer parent_class;

GType
gst_bootproto_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
	{
	  { GST_BOOTPROTO_NONE, "GST_BOOTPROTO_NONE", "none" },
	  { GST_BOOTPROTO_STATIC, "GST_BOOTPROTO_STATIC", "static" },
	  { GST_BOOTPROTO_DHCP, "GST_BOOTPROTO_DHCP", "dhcp" },
	};

      etype = g_enum_register_static ("GstBootProto", values);
    }

  return etype;
}

GType
gst_iface_ethernet_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_ethernet_info =
	{
	  sizeof (GstIfaceEthernetClass),
	  NULL,		/* base_init */
	  NULL,		/* base_finalize */
	  (GClassInitFunc) gst_iface_ethernet_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GstIfaceEthernet),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) gst_iface_ethernet_init,
	};

      type = g_type_register_static (GST_TYPE_IFACE, "GstIfaceEthernet",
				     &iface_ethernet_info, 0);
    }

  return type;
}

static void
gst_iface_ethernet_class_init (GstIfaceEthernetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GstIfaceClass *iface_class = GST_IFACE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->set_property = gst_iface_ethernet_set_property;
  object_class->get_property = gst_iface_ethernet_get_property;
  object_class->finalize     = gst_iface_ethernet_finalize;

  iface_class->get_iface_pixbuf = gst_iface_ethernet_get_pixbuf;
  iface_class->get_iface_type   = gst_iface_ethernet_get_iface_type;
  iface_class->get_iface_desc   = gst_iface_ethernet_get_desc;
  iface_class->has_gateway      = gst_iface_ethernet_has_gateway;
  iface_class->get_xml          = gst_iface_ethernet_impl_get_xml;

  g_object_class_install_property (object_class,
				   PROP_ADDRESS,
				   g_param_spec_string ("iface_address",
							"Iface address",
							"Address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_NETMASK,
				   g_param_spec_string ("iface_netmask",
							"Iface netmask",
							"Netmask for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_BROADCAST,
				   g_param_spec_string ("iface_gateway",
							"Iface gateway",
							"Gateway address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_NETWORK,
				   g_param_spec_string ("iface_network",
							"Iface network",
							"Network address for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_BROADCAST,
				   g_param_spec_string ("iface_broadcast",
							"Iface broadcast",
							"Network broadcast for the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_BOOTPROTO,
				   g_param_spec_enum ("iface_bootproto",
						      "Iface bootproto",
						      "Network bootproto for the iface",
						      GST_TYPE_BOOTPROTO,
						      GST_BOOTPROTO_NONE,
						      G_PARAM_READWRITE));
}

static void
gst_iface_ethernet_init (GstIfaceEthernet *iface)
{
  g_return_if_fail (GST_IS_IFACE_ETHERNET (iface));

  iface->_priv = g_new0 (GstIfaceEthernetPriv, 1);
  iface->_priv->pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/connection-ethernet.png", NULL);
  iface->_priv->address = NULL;
  iface->_priv->netmask = NULL;
  iface->_priv->gateway = NULL;
  iface->_priv->network = NULL;
  iface->_priv->broadcast = NULL;
}

static void
gst_iface_ethernet_finalize (GObject *object)
{
  GstIfaceEthernet *iface = GST_IFACE_ETHERNET (object);

  g_return_if_fail (GST_IS_IFACE_ETHERNET (iface));

  if (iface->_priv)
    {
      gdk_pixbuf_unref (iface->_priv->pixbuf);
      g_free (iface->_priv->address);
      g_free (iface->_priv->netmask);
      g_free (iface->_priv->gateway);
      g_free (iface->_priv->network);
      g_free (iface->_priv->broadcast);

      g_free (iface->_priv);
      iface->_priv = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_iface_ethernet_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  GstIfaceEthernet *iface = GST_IFACE_ETHERNET (object);

  g_return_if_fail (GST_IS_IFACE_ETHERNET (iface));

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_free (iface->_priv->address);
      iface->_priv->address = g_value_dup_string (value);
      break;
    case PROP_NETMASK:
      g_free (iface->_priv->netmask);
      iface->_priv->netmask = g_value_dup_string (value);
      break;
    case PROP_GATEWAY:
      g_free (iface->_priv->gateway);
      iface->_priv->gateway = g_value_dup_string (value);
      break;
    case PROP_NETWORK:
      g_free (iface->_priv->network);
      iface->_priv->network = g_value_dup_string (value);
      break;
    case PROP_BROADCAST:
      g_free (iface->_priv->network);
      iface->_priv->broadcast = g_value_dup_string (value);
      break;
    case PROP_BOOTPROTO:
      iface->_priv->bootproto = g_value_get_enum (value);
      break;
    }
}

static void
gst_iface_ethernet_get_property (GObject      *object,
				 guint         prop_id,
				 GValue       *value,
				 GParamSpec   *pspec)
{
  GstIfaceEthernet *iface = GST_IFACE_ETHERNET (object);

  g_return_if_fail (GST_IS_IFACE_ETHERNET (iface));

  switch (prop_id)
    {
    case PROP_ADDRESS:
      g_value_set_string (value, iface->_priv->address);
      break;
    case PROP_NETMASK:
      g_value_set_string (value, iface->_priv->netmask);
      break;
    case PROP_GATEWAY:
      g_value_set_string (value, iface->_priv->gateway);
      break;
    case PROP_NETWORK:
      g_value_set_string (value, iface->_priv->network);
      break;
    case PROP_BROADCAST:
      g_value_set_string (value, iface->_priv->broadcast);
      break;
    case PROP_BOOTPROTO:
      g_value_set_enum (value, iface->_priv->bootproto);
      break;
    }
}

static const GdkPixbuf*
gst_iface_ethernet_get_pixbuf (GstIface *iface)
{
  return GST_IFACE_ETHERNET (iface)->_priv->pixbuf;
}

static const gchar*
gst_iface_ethernet_get_iface_type (GstIface *iface)
{
  return "ethernet";
}

static const gchar*
gst_iface_ethernet_get_desc (GstIface *iface)
{
  return _("Ethernet connection");
}

static gboolean
gst_iface_ethernet_has_gateway (GstIface *iface)
{
  GstIfaceEthernet *ethernet_iface = GST_IFACE_ETHERNET (iface);
  
  return ((ethernet_iface->_priv->bootproto == GST_BOOTPROTO_DHCP) ||
	  (ethernet_iface->_priv->gateway));
}

static void
gst_iface_ethernet_impl_get_xml (GstIface *iface, xmlNodePtr node)
{
  xmlNodePtr configuration;
  GstIfaceEthernet *iface_ethernet;

  g_return_if_fail (GST_IS_IFACE_ETHERNET (iface));
  iface_ethernet = GST_IFACE_ETHERNET (iface);

  if (gst_iface_is_configured (iface))
    {
      configuration = gst_xml_element_find_first (node, "configuration");
      if (!configuration)
        configuration = gst_xml_element_add (node, "configuration");

      if (iface_ethernet->_priv->bootproto == GST_BOOTPROTO_DHCP)
        gst_xml_set_child_content (configuration, "bootproto", "dhcp");
      else
        {
          gst_xml_set_child_content (configuration, "bootproto", "none");
          gst_xml_set_child_content (configuration, "address",   iface_ethernet->_priv->address);
          gst_xml_set_child_content (configuration, "netmask",   iface_ethernet->_priv->netmask);
          gst_xml_set_child_content (configuration, "gateway",   iface_ethernet->_priv->gateway);
          gst_xml_set_child_content (configuration, "network",   iface_ethernet->_priv->network);
          gst_xml_set_child_content (configuration, "broadcast", iface_ethernet->_priv->broadcast);
        }
    }

  GST_IFACE_CLASS (parent_class)->get_xml (iface, node);
}

void
gst_iface_ethernet_set_config_from_xml (GstIfaceEthernet *iface,
					xmlNodePtr        node)
{
  xmlNodePtr configuration;
  gchar *address, *netmask, *bootproto;
  GstBootProto proto;

  g_return_if_fail (iface != NULL);
  g_return_if_fail (node != NULL);

  /* config the parent class data */
  gst_iface_set_config_from_xml (GST_IFACE (iface), node);

  configuration = gst_xml_element_find_first (node, "configuration");
  if (!configuration)
    return;

  bootproto = gst_xml_get_child_content (configuration, "bootproto");

  if (strcmp (bootproto, "none") == 0)
    {
      proto = GST_BOOTPROTO_STATIC;
      address = gst_xml_get_child_content (configuration, "address");
      netmask = gst_xml_get_child_content (configuration, "netmask");
    }
  else
    {
      proto = GST_BOOTPROTO_DHCP;
      address = NULL;
      netmask = NULL;
    }

  g_object_set (G_OBJECT (iface),
		"iface-address",   address,
		"iface-netmask",   netmask,
		"iface-bootproto", proto,
		NULL);

  g_free (bootproto);
  g_free (address);
  g_free (netmask);
}

GstIfaceEthernet*
gst_iface_ethernet_new_from_xml (xmlNodePtr node)
{
  GstIfaceEthernet *iface;

  iface = g_object_new (GST_TYPE_IFACE_ETHERNET, NULL);
  gst_iface_ethernet_set_config_from_xml (iface, node);

  return iface;
}
