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
#include "network-iface-wireless.h"
#include "gst.h"

struct _GstIfaceWirelessPriv
{
  GdkPixbuf *pixbuf;

  gchar     *essid;
  gchar     *wep_key;
};

static void gst_iface_wireless_class_init (GstIfaceWirelessClass *class);
static void gst_iface_wireless_init       (GstIfaceWireless      *iface);
static void gst_iface_wireless_finalize   (GObject *object);

static const GdkPixbuf* gst_iface_wireless_get_pixbuf     (GstIface *iface);
static const gchar*     gst_iface_wireless_get_iface_type (GstIface *iface);
static const gchar*     gst_iface_wireless_get_desc       (GstIface *iface);
static void             gst_iface_wireless_get_xml        (GstIface *iface, xmlNodePtr root);
static void             gst_iface_wireless_impl_get_xml   (GstIface *iface,
                                                           xmlNodePtr node);

static void gst_iface_wireless_set_property (GObject      *object,
					     guint         prop_id,
					     const GValue *value,
					     GParamSpec   *pspec);
static void gst_iface_wireless_get_property (GObject      *object,
					     guint         prop_id,
					     GValue       *value,
					     GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_ESSID,
  PROP_WEP_KEY
};

static gpointer parent_class;

GType
gst_iface_wireless_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_wireless_info =
        {
	  sizeof (GstIfaceWirelessClass),
	  NULL,		/* base_init */
	  NULL,		/* base_finalize */
	  (GClassInitFunc) gst_iface_wireless_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GstIfaceWireless),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) gst_iface_wireless_init,
	};

      type = g_type_register_static (GST_TYPE_IFACE_ETHERNET,
				     "GstIfaceWireless",
				     &iface_wireless_info, 0);
    }

  return type;
}

static void
gst_iface_wireless_class_init (GstIfaceWirelessClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GstIfaceClass *iface_class = GST_IFACE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->set_property = gst_iface_wireless_set_property;
  object_class->get_property = gst_iface_wireless_get_property;
  object_class->finalize     = gst_iface_wireless_finalize;

  iface_class->get_iface_pixbuf = gst_iface_wireless_get_pixbuf;
  iface_class->get_iface_type   = gst_iface_wireless_get_iface_type;
  iface_class->get_iface_desc   = gst_iface_wireless_get_desc;
  iface_class->get_xml          = gst_iface_wireless_impl_get_xml;

  g_object_class_install_property (object_class,
				   PROP_ESSID,
				   g_param_spec_string ("iface_essid",
							"Iface ESSID",
							"ESSID",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_WEP_KEY,
				   g_param_spec_string ("iface_wep_key",
							"Iface WEP key",
							"WEP key",
							NULL,
							G_PARAM_READWRITE));
}

static void
gst_iface_wireless_init (GstIfaceWireless *iface)
{
  g_return_if_fail (GST_IS_IFACE_WIRELESS (iface));

  iface->_priv = g_new0 (GstIfaceWirelessPriv, 1);
  iface->_priv->pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/wavelan-48.png", NULL);
  iface->_priv->essid = NULL;
  iface->_priv->wep_key = NULL;
}

static void
gst_iface_wireless_finalize (GObject *object)
{
  GstIfaceWireless *iface = GST_IFACE_WIRELESS (object);

  g_return_if_fail (GST_IS_IFACE_WIRELESS (iface));

  if (iface->_priv)
    {
      gdk_pixbuf_unref (iface->_priv->pixbuf);
      g_free (iface->_priv->essid);
      g_free (iface->_priv->wep_key);

      g_free (iface->_priv);
      iface->_priv = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_iface_wireless_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  GstIfaceWireless *iface = GST_IFACE_WIRELESS (object);

  g_return_if_fail (GST_IS_IFACE_WIRELESS (iface));

  switch (prop_id)
    {
    case PROP_ESSID:
      g_free (iface->_priv->essid);
      iface->_priv->essid = g_value_dup_string (value);
      break;
    case PROP_WEP_KEY:
      g_free (iface->_priv->wep_key);
      iface->_priv->wep_key = g_value_dup_string (value);
      break;
    }
}

static void
gst_iface_wireless_get_property (GObject      *object,
				 guint         prop_id,
				 GValue       *value,
				 GParamSpec   *pspec)
{
  GstIfaceWireless *iface = GST_IFACE_WIRELESS (object);

  g_return_if_fail (GST_IS_IFACE_WIRELESS (iface));

  switch (prop_id)
    {
    case PROP_ESSID:
      g_value_set_string (value, iface->_priv->essid);
      break;
    case PROP_WEP_KEY:
      g_value_set_string (value, iface->_priv->wep_key);
      break;
    }
}

static const GdkPixbuf*
gst_iface_wireless_get_pixbuf (GstIface *iface)
{
  return GST_IFACE_WIRELESS (iface)->_priv->pixbuf;
}

static const gchar*
gst_iface_wireless_get_iface_type (GstIface *iface)
{
  return "wireless";
}

static const gchar*
gst_iface_wireless_get_desc (GstIface *iface)
{
  return _("Wireless connection");
}

static void
gst_iface_wireless_impl_get_xml (GstIface *iface, xmlNodePtr node)
{
  xmlNodePtr configuration;
  GstIfaceWireless *iface_wireless;

  g_return_if_fail (GST_IS_IFACE_WIRELESS (iface));
  iface_wireless = GST_IFACE_WIRELESS (iface);

  if (gst_iface_is_configured (iface))
    {
      configuration = gst_xml_element_find_first (node, "configuration");
      if (!configuration)
        configuration = gst_xml_element_add (node, "configuration");

      gst_xml_set_child_content (configuration, "essid", iface_wireless->_priv->essid);
      gst_xml_set_child_content (configuration, "key",   iface_wireless->_priv->wep_key);
    }

  GST_IFACE_CLASS (parent_class)->get_xml (iface, node);
}

void
gst_iface_wireless_set_config_from_xml (GstIfaceWireless *iface,
					xmlNodePtr        node)
{
  xmlNodePtr configuration;
  gchar *essid, *key;
  
  /* set ethernet configuration */
  gst_iface_ethernet_set_config_from_xml (GST_IFACE_ETHERNET (iface), node);

  configuration = gst_xml_element_find_first (node, "configuration");
  if (!configuration)
    return;

  essid = gst_xml_get_child_content (configuration, "essid");
  key   = gst_xml_get_child_content (configuration, "key");
  
  g_object_set (G_OBJECT (iface),
		"iface-essid", essid,
		"iface-wep-key", key,
		NULL);
  
  g_free (essid);
  g_free (key);
}

GstIfaceWireless*
gst_iface_wireless_new_from_xml (xmlNodePtr node)
{
  GstIfaceWireless *iface;

  g_return_val_if_fail (node != NULL, NULL);

  iface = g_object_new (GST_TYPE_IFACE_WIRELESS, NULL);
  gst_iface_wireless_set_config_from_xml (iface, node);

  return iface;
}
