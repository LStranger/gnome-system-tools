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

#include "network-iface-isdn.h"
#include "gst.h"

struct _GstIfaceIsdnPriv
{
  GdkPixbuf *pixbuf;

  gchar *login;
  gchar *password;
  gchar *phone_number;
  gchar *dial_prefix;
  gchar *section;
  
  gboolean default_gw;
  gboolean persist;
  gboolean noauth;
};

static void gst_iface_isdn_class_init (GstIfaceIsdnClass *class);
static void gst_iface_isdn_init       (GstIfaceIsdn      *iface);
static void gst_iface_isdn_finalize   (GObject           *object);

static const GdkPixbuf* gst_iface_isdn_get_pixbuf     (GstIface *iface);
static const gchar*     gst_iface_isdn_get_iface_type (GstIface *iface);
static const gchar*     gst_iface_isdn_get_desc       (GstIface *iface);
static gboolean         gst_iface_isdn_has_gateway    (GstIface *iface);
static void             gst_iface_isdn_impl_get_xml   (GstIface *iface,
                                                       xmlNodePtr node);

static void gst_iface_isdn_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
static void gst_iface_isdn_get_property (GObject      *object,
                                         guint         prop_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_LOGIN,
  PROP_PASSWORD,
  PROP_PHONE_NUMBER,
  PROP_DIAL_PREFIX,
  PROP_DEFAULT_GW,
  PROP_PERSIST,
  PROP_SECTION,
  PROP_NOAUTH
};

static gpointer parent_class;

GType
gst_iface_isdn_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_isdn_info =
        {
          sizeof (GstIfaceIsdnClass),
          NULL,		/* base_init */
          NULL,		/* base_finalize */
          (GClassInitFunc) gst_iface_isdn_class_init,
          NULL,		/* class_finalize */
          NULL,		/* class_data */
          sizeof (GstIfaceIsdn),
          0,		/* n_preallocs */
          (GInstanceInitFunc) gst_iface_isdn_init,
        };

      type = g_type_register_static (GST_TYPE_IFACE, "GstIfaceIsdn",
                                     &iface_isdn_info, 0);
    }

  return type;
}

static void
gst_iface_isdn_class_init (GstIfaceIsdnClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);
  GstIfaceClass *iface_class  = GST_IFACE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->set_property = gst_iface_isdn_set_property;
  object_class->get_property = gst_iface_isdn_get_property;
  object_class->finalize = gst_iface_isdn_finalize;

  iface_class->get_iface_pixbuf = gst_iface_isdn_get_pixbuf;
  iface_class->get_iface_type   = gst_iface_isdn_get_iface_type;
  iface_class->get_iface_desc   = gst_iface_isdn_get_desc;
  iface_class->has_gateway      = gst_iface_isdn_has_gateway;
  iface_class->get_xml          = gst_iface_isdn_impl_get_xml;
  
  g_object_class_install_property (object_class,
                                   PROP_LOGIN,
                                   g_param_spec_string ("iface_login",
                                                        "Iface login",
                                                        "Login for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PASSWORD,
                                   g_param_spec_string ("iface_password",
                                                        "Iface password",
                                                        "Password for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PHONE_NUMBER,
                                   g_param_spec_string ("iface_phone_number",
                                                        "Iface phone number",
                                                        "Phone numberfor the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_PREFIX,
                                   g_param_spec_string ("iface_dial_prefix",
                                                        "Iface dial prefix",
                                                        "Dial prefix for the connection",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_SECTION,
                                   g_param_spec_string ("iface_section",
                                                         "Iface section",
                                                         "Name of the wvdial section or the provider name",
                                                         NULL,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DEFAULT_GW,
                                   g_param_spec_boolean ("iface_default_gw",
                                                         "Iface default gw",
                                                         "Whether to use the iface as the default gateway",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_PERSIST,
                                   g_param_spec_boolean ("iface_persist",
                                                         "Iface persist",
                                                         "Whether to persist if the connection fails",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_NOAUTH,
                                   g_param_spec_boolean ("iface_noauth",
                                                         "Iface no auth",
                                                         "Whether the ISP has to authenticate itself or not",
                                                         TRUE,
                                                         G_PARAM_READWRITE));
}

static void
gst_iface_isdn_init (GstIfaceIsdn *iface)
{
  g_return_if_fail (GST_IS_IFACE_ISDN (iface));

  iface->_priv = g_new0 (GstIfaceIsdnPriv, 1);
  iface->_priv->pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/ppp.png", NULL);

  iface->_priv->login = NULL;
  iface->_priv->password = NULL;
  iface->_priv->phone_number = NULL;
  iface->_priv->dial_prefix = NULL;
  iface->_priv->section = NULL;

  /* FIXME: Set here the default until we add gui for this */
  iface->_priv->noauth = TRUE;
}

static void
gst_iface_isdn_finalize (GObject *object)
{
  GstIfaceIsdn *iface = GST_IFACE_ISDN (object);

  g_return_if_fail (GST_IS_IFACE_ISDN (object));

  if (iface->_priv)
    {
      gdk_pixbuf_unref (iface->_priv->pixbuf);
      g_free (iface->_priv->login);
      g_free (iface->_priv->password);
      g_free (iface->_priv->phone_number);
      g_free (iface->_priv->dial_prefix);
      g_free (iface->_priv->section);

      g_free (iface->_priv);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_iface_isdn_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GstIfaceIsdn *iface = GST_IFACE_ISDN (object);

  g_return_if_fail (GST_IS_IFACE_ISDN (iface));
  
  switch (prop_id)
    {
    case PROP_LOGIN:
      g_free (iface->_priv->login);
      iface->_priv->login = g_value_dup_string (value);
      break;
    case PROP_PASSWORD:
      g_free (iface->_priv->password);
      iface->_priv->password = g_value_dup_string (value);
      break;
    case PROP_PHONE_NUMBER:
      g_free (iface->_priv->phone_number);
      iface->_priv->phone_number = g_value_dup_string (value);
      break;
    case PROP_DIAL_PREFIX:
      g_free (iface->_priv->dial_prefix);
      iface->_priv->dial_prefix = g_value_dup_string (value);
      break;
    case PROP_SECTION:
      iface->_priv->section = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_GW:
      iface->_priv->default_gw = g_value_get_boolean (value);
      break;
    case PROP_PERSIST:
      iface->_priv->persist = g_value_get_boolean (value);
      break;
    case PROP_NOAUTH:
      iface->_priv->noauth = g_value_get_boolean (value);
      break;
    }
}

static void
gst_iface_isdn_get_property (GObject      *object,
                             guint         prop_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  GstIfaceIsdn *iface = GST_IFACE_ISDN (object);

  g_return_if_fail (GST_IS_IFACE_ISDN (iface));
  
  switch (prop_id)
    {
    case PROP_LOGIN:
      g_value_set_string (value, iface->_priv->login);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, iface->_priv->password);
      break;
    case PROP_PHONE_NUMBER:
      g_value_set_string (value, iface->_priv->phone_number);
      break;
    case PROP_DIAL_PREFIX:
      g_value_set_string (value, iface->_priv->dial_prefix);
      break;
    case PROP_SECTION:
      g_value_set_string (value, iface->_priv->section);
      break;
    case PROP_DEFAULT_GW:
      g_value_set_boolean (value, iface->_priv->default_gw);
      break;
    case PROP_PERSIST:
      g_value_set_boolean (value, iface->_priv->persist);
      break;
    case PROP_NOAUTH:
      g_value_set_boolean (value, iface->_priv->noauth);
      break;
    }
}

static const GdkPixbuf*
gst_iface_isdn_get_pixbuf (GstIface *iface)
{
  return GST_IFACE_ISDN (iface)->_priv->pixbuf;
}

static const gchar*
gst_iface_isdn_get_iface_type (GstIface *iface)
{
  return "isdn";
}

static const gchar*
gst_iface_isdn_get_desc (GstIface *iface)
{
  return _("ISDN connection");
}

static gboolean
gst_iface_isdn_has_gateway (GstIface *iface)
{
  return FALSE;
}

static void
gst_iface_isdn_impl_get_xml (GstIface *iface, xmlNodePtr node)
{
  xmlNodePtr    configuration;
  GstIfaceIsdn *iface_isdn;
  gchar        *str;

  g_return_if_fail (GST_IS_IFACE_ISDN (iface));
  iface_isdn = GST_IFACE_ISDN (iface);

  if (gst_iface_is_configured (iface))
    {
      configuration = gst_xml_element_find_first (node, "configuration");
      if (!configuration)
        configuration = gst_xml_element_add (node, "configuration");

      gst_xml_set_child_content (configuration, "login",        iface_isdn->_priv->login);
      gst_xml_set_child_content (configuration, "password",     iface_isdn->_priv->password);
      gst_xml_set_child_content (configuration, "phone_number", iface_isdn->_priv->phone_number);
      gst_xml_set_child_content (configuration, "external_line",  iface_isdn->_priv->dial_prefix);

      gst_xml_set_child_content (configuration, "section",
                                 (iface_isdn->_priv->section) ?
				 iface_isdn->_priv->section :
				 gst_iface_get_dev (iface));

      gst_xml_element_set_boolean (configuration, "set_default_gw", iface_isdn->_priv->default_gw);
      gst_xml_element_set_boolean (configuration, "persist", iface_isdn->_priv->persist);
      gst_xml_element_set_boolean (configuration, "noauth",  iface_isdn->_priv->noauth);
    }
  
  GST_IFACE_CLASS (parent_class)->get_xml (iface, node);
}

void
gst_iface_isdn_set_config_from_xml (GstIfaceIsdn *iface,
				    xmlNodePtr    node)
{
  gchar      *login, *password, *phone_number, *dial_prefix, *section;
  gboolean    default_gw, persist;
  xmlNodePtr  configuration;

  /* config the parent class data */
  gst_iface_set_config_from_xml (GST_IFACE (iface), node);

  configuration = gst_xml_element_find_first (node, "configuration");
  if (!configuration)
    return;

  login        = gst_xml_get_child_content (configuration, "login");
  password     = gst_xml_get_child_content (configuration, "password");
  phone_number = gst_xml_get_child_content (configuration, "phone_number");
  dial_prefix  = gst_xml_get_child_content (configuration, "external_line");
  section      = gst_xml_get_child_content (configuration, "section");

  default_gw = gst_xml_element_get_boolean (configuration, "set_default_gw");
  persist    = gst_xml_element_get_boolean (configuration, "persist");

  g_object_set (G_OBJECT (iface),
                "iface-login", login,
                "iface-password", password,
                "iface-phone-number", phone_number,
                "iface-dial-prefix", dial_prefix,
                "iface-default-gw", default_gw,
                "iface-persist", persist,
                "iface-section", section,
		/* FIXME: until we add some gui for noauth, it will
		   be the most sane default, I guess */
		"iface-noauth", TRUE, 
                NULL);

  g_free (login);
  g_free (password);
  g_free (phone_number);
  g_free (dial_prefix);
  g_free (section);
}

GstIfaceIsdn*
gst_iface_isdn_new_from_xml (xmlNodePtr node)
{
  GstIfaceIsdn *iface;

  iface = g_object_new (GST_TYPE_IFACE_ISDN, NULL);
  gst_iface_isdn_set_config_from_xml (iface, node);

  return iface;
}
