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
#include <glib/gi18n.h>
#include <gdk/gdkpixbuf.h>
#include "gst-xml.h"
#include "gst-tool.h"
#include "network-iface.h"

extern GstTool *tool;

struct _GstIfacePriv
{
  gboolean  is_auto;
  gboolean  is_enabled;
  gboolean  is_configured;
  gchar    *dev;
  gchar    *hwaddr;
  gchar    *file;
};

static void gst_iface_class_init (GstIfaceClass *class);
static void gst_iface_init       (GstIface      *iface);
static void gst_iface_finalize   (GObject       *object);

static void gst_iface_impl_get_xml (GstIface   *iface,
                                    xmlNodePtr  node);

static void gst_iface_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec);
static void gst_iface_get_property (GObject      *object,
				    guint         prop_id,
				    GValue       *value,
				    GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_AUTO,
  PROP_ENABLED,
  PROP_CONFIGURED,
  PROP_DEV,
  PROP_FILE,
  PROP_HWADDR
};

static gpointer parent_class;

GType
gst_iface_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_info = {
	sizeof (GstIfaceClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gst_iface_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GstIface),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gst_iface_init,
      };

      type = g_type_register_static (G_TYPE_OBJECT, "GstIface",
				     &iface_info, 0);
    }

  return type;
}

static void
gst_iface_class_init (GstIfaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->set_property = gst_iface_set_property;
  object_class->get_property = gst_iface_get_property;
  object_class->finalize     = gst_iface_finalize;

  class->get_xml = gst_iface_impl_get_xml;

  g_object_class_install_property (object_class,
				   PROP_AUTO,
				   g_param_spec_boolean ("iface_auto",
							 "Iface is auto",
							 "Whether the interface starts at boot time",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_ENABLED,
				   g_param_spec_boolean ("iface_enabled",
							 "Iface is enabled",
							 "Whether the interface is enabled",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_CONFIGURED,
				   g_param_spec_boolean ("iface_configured",
							 "Iface is configured",
							 "Whether the interface is configured",
							 FALSE,
							 G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_DEV,
				   g_param_spec_string ("iface_dev",
							"Iface dev",
							"Device name of the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_FILE,
				   g_param_spec_string ("iface_file",
							"Iface file",
							"Filename of the iface",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_HWADDR,
				   g_param_spec_string ("iface_hwaddr",
							"Iface hwaddr",
							"MAC address of the iface",
							NULL,
							G_PARAM_READWRITE));
}

static void
gst_iface_init (GstIface *iface)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  iface->_priv = g_new0 (GstIfacePriv, 1);

  iface->_priv->dev = NULL;
}

static void
gst_iface_finalize (GObject *object)
{
  GstIface *iface = GST_IFACE (object);

  g_return_if_fail (GST_IS_IFACE (iface));

  if (iface->_priv)
    {
      g_free (iface->_priv->dev);
      g_free (iface->_priv->hwaddr);
      g_free (iface->_priv->file);

      g_free (iface->_priv);
      iface->_priv = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_iface_set_property (GObject      *object,
			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  GstIface *iface = GST_IFACE (object);

  g_return_if_fail (GST_IS_IFACE (iface));

  switch (prop_id)
    {
    case PROP_AUTO:
      iface->_priv->is_auto = g_value_get_boolean (value);
      break;
    case PROP_ENABLED:
      iface->_priv->is_enabled = g_value_get_boolean (value);
      break;
    case PROP_CONFIGURED:
      iface->_priv->is_configured = g_value_get_boolean (value);
      break;
    case PROP_DEV:
      g_free (iface->_priv->dev);
      iface->_priv->dev = g_value_dup_string (value);
      break;
    case PROP_FILE:
      g_free (iface->_priv->file);
      iface->_priv->file = g_value_dup_string (value);
      break;
    case PROP_HWADDR:
      g_free (iface->_priv->hwaddr);
      iface->_priv->hwaddr = g_value_dup_string (value);
      break;
    }
}

static void
gst_iface_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  GstIface *iface = GST_IFACE (object);

  g_return_if_fail (GST_IS_IFACE (iface));

  switch (prop_id)
    {
    case PROP_AUTO:
      g_value_set_boolean (value, iface->_priv->is_auto);
      break;
    case PROP_ENABLED:
      g_value_set_boolean (value, iface->_priv->is_enabled);
      break;
    case PROP_CONFIGURED:
      g_value_set_boolean (value, iface->_priv->is_configured);
      break;
    case PROP_DEV:
      g_value_set_string (value, iface->_priv->dev);
      break;
    case PROP_FILE:
      g_value_set_string (value, iface->_priv->file);
      break;
    case PROP_HWADDR:
      g_value_set_string (value, iface->_priv->hwaddr);
      break;
    }
}

static void
gst_iface_impl_get_xml (GstIface *iface, xmlNodePtr node)
{
  xmlNodePtr configuration;

  g_return_if_fail (GST_IS_IFACE (iface));

  gst_xml_set_child_content   (node, "dev",     iface->_priv->dev);
  gst_xml_set_child_content   (node, "hwaddr",  iface->_priv->hwaddr);
  gst_xml_element_set_boolean (node, "enabled", iface->_priv->is_enabled);

  if (iface->_priv->is_configured)
    {
      configuration = gst_xml_element_find_first (node, "configuration");

      if (!configuration)
        configuration = gst_xml_element_add (node, "configuration");

      gst_xml_element_set_boolean (configuration, "auto", iface->_priv->is_auto);

      if (iface->_priv->file)
        gst_xml_set_child_content (configuration, "file", iface->_priv->file);
      else
        gst_xml_set_child_content (configuration, "file", iface->_priv->dev);
    }
}

void
gst_iface_set_config_from_xml (GstIface   *iface,
			       xmlNodePtr  node)
{
  gchar      *dev, *file, *hwaddr;
  gboolean    enabled;
  xmlNodePtr  configuration;

  g_return_if_fail (iface != NULL);
  g_return_if_fail (node != NULL);

  dev = gst_xml_get_child_content (node, "dev");
  file = gst_xml_get_child_content (node, "file");
  hwaddr = gst_xml_get_child_content (node, "hwaddr");
  enabled = gst_xml_element_get_boolean (node, "enabled");
  configuration = gst_xml_element_find_first (node, "configuration");

  g_object_set (G_OBJECT (iface),
		"iface-dev",        dev,
		"iface-file",       file,
		"iface-hwaddr",     hwaddr,
		"iface-enabled",    enabled,
		"iface-auto",       enabled,
		"iface-configured", (configuration != NULL),
		NULL);

  g_free (dev);
  g_free (file);
  g_free (hwaddr);
}

gboolean
gst_iface_get_auto (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), FALSE);

  return iface->_priv->is_auto;
}

void
gst_iface_set_auto (GstIface *iface, gboolean is_auto)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  iface->_priv->is_auto = is_auto;
}

gboolean
gst_iface_get_enabled (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), FALSE);

  return iface->_priv->is_enabled;
}

void
gst_iface_set_enabled (GstIface *iface, gboolean is_enabled)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  iface->_priv->is_enabled = is_enabled;
}

const gchar*
gst_iface_get_dev (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), NULL);

  return iface->_priv->dev;
}

void
gst_iface_set_dev (GstIface *iface, const gchar *dev)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  iface->_priv->dev = g_strdup (dev);
}

const gchar*
gst_iface_get_hwaddr (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), NULL);

  return iface->_priv->hwaddr;
}

void
gst_iface_set_hwaddr (GstIface *iface, const gchar *hwaddr)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  iface->_priv->hwaddr = g_strdup (hwaddr);
}

gboolean
gst_iface_is_configured (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), FALSE);

  return iface->_priv->is_configured;
}

void
gst_iface_set_configured (GstIface *iface, gboolean is_configured)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  iface->_priv->is_configured = is_configured;
}

gboolean
gst_iface_has_gateway (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), FALSE);

  if (GST_IFACE_GET_CLASS (iface)->has_gateway == NULL)
    return FALSE;

  return GST_IFACE_GET_CLASS (iface)->has_gateway (iface);
}

gboolean
gst_iface_enable (GstIface *iface)
{
  xmlDoc     *doc, *ret_doc;
  xmlNodePtr  root;
  gboolean    ret;
  gchar      *report;

  doc = gst_xml_doc_create ("interface");
  root = gst_xml_doc_get_root (doc);
  report = g_strdup_printf (_("Activating interface \"%s\""), gst_iface_get_dev (iface));

  /* get the interface xml directly in the root */
  GST_IFACE_GET_CLASS (iface)->get_xml (iface, root);

  ret_doc = gst_tool_run_set_directive (tool, doc, report,
					"enable_iface_with_config", NULL);

  /* check that it has succeeded */
  root = gst_xml_doc_get_root (ret_doc);
  ret  = gst_xml_element_get_boolean (root, "success");

  iface->_priv->is_enabled = (ret == TRUE);
  
  gst_xml_doc_destroy (ret_doc);
  gst_xml_doc_destroy (doc);
  g_free (report);

  return ret;
}

void
gst_iface_disable (GstIface *iface)
{
  g_return_if_fail (GST_IS_IFACE (iface));

  gst_tool_run_set_directive (tool, NULL, NULL,
			      "enable_iface", iface->_priv->dev, "0", NULL);

  iface->_priv->is_enabled = FALSE;
}

static const gchar*
gst_iface_get_iface_type (GstIface *iface)
{
  if (GST_IFACE_GET_CLASS (iface)->get_iface_type == NULL)
    return NULL;

  return GST_IFACE_GET_CLASS (iface)->get_iface_type (iface);
}

void
gst_iface_get_xml (GstIface *iface, xmlNodePtr root)
{
  xmlNodePtr   interface;
  const gchar *type;

  if (GST_IFACE_GET_CLASS (iface)->get_xml == NULL)
    return;

  interface = gst_xml_element_add (root, "interface");
  type = gst_iface_get_iface_type (iface);

  if (type)
    gst_xml_element_set_attribute (interface, "type", type);

  GST_IFACE_GET_CLASS (iface)->get_xml (iface, interface);
}

const GdkPixbuf*
gst_iface_get_pixbuf (GstIface *iface)
{
  g_return_val_if_fail (GST_IS_IFACE (iface), NULL);

  if (GST_IFACE_GET_CLASS (iface)->get_iface_pixbuf == NULL)
    return NULL;

  return GST_IFACE_GET_CLASS (iface)->get_iface_pixbuf (iface);
}

gchar*
gst_iface_get_desc (GstIface *iface)
{
  gchar *secondary, *text, *message;
  const gchar *primary;

  g_return_val_if_fail (GST_IS_IFACE (iface), NULL);

  if (GST_IFACE_GET_CLASS (iface)->get_iface_desc == NULL)
    return NULL;

  primary = GST_IFACE_GET_CLASS (iface)->get_iface_desc (iface);

  if (!iface->_priv->is_configured)
    text = N_("The interface %s is not configured");
  else if (!iface->_priv->is_enabled)
    text = N_("The interface %s is not active");
  else
    text = N_("The interface %s is active");

  secondary = g_strdup_printf (text, iface->_priv->dev);
  message = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s",
			     primary, secondary );

  g_free (secondary);
  return message;
}
