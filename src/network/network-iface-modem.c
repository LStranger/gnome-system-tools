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

#include "network-iface-modem.h"
#include "gst.h"

struct _GstIfaceModemPriv
{
  GdkPixbuf *pixbuf;

  gchar *login;
  gchar *password;
  gchar *serial_port;
  gchar *phone_number;
  gchar *dial_prefix;
  gchar *section;

  gint volume;
  gint dial_type;
  
  gboolean default_gw;
  gboolean persist;
};

static void gst_iface_modem_class_init (GstIfaceModemClass *class);
static void gst_iface_modem_init       (GstIfaceModem      *iface);
static void gst_iface_modem_finalize   (GObject            *object);

static const GdkPixbuf* gst_iface_modem_get_pixbuf     (GstIface *iface);
static const gchar*     gst_iface_modem_get_iface_type (GstIface *iface);
static const gchar*     gst_iface_modem_get_desc       (GstIface *iface);
static gboolean         gst_iface_modem_has_gateway    (GstIface *iface);
static void             gst_iface_modem_impl_get_xml   (GstIface *iface,
                                                        xmlNodePtr node);

static void gst_iface_modem_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void gst_iface_modem_get_property (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

enum {
  PROP_0,
  PROP_LOGIN,
  PROP_PASSWORD,
  PROP_SERIAL_PORT,
  PROP_PHONE_NUMBER,
  PROP_DIAL_PREFIX,
  PROP_VOLUME,
  PROP_DIAL_TYPE,
  PROP_DEFAULT_GW,
  PROP_PERSIST,
  PROP_SECTION
};

static gpointer parent_class;

GType
gst_modem_volume_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
        {
          { GST_MODEM_VOLUME_SILENT, "GST_MODEM_VOLUME_SILENT", "silent" },
          { GST_MODEM_VOLUME_LOW,    "GST_MODEM_VOLUME_LOW",    "low" },
          { GST_MODEM_VOLUME_MEDIUM, "GST_MODEM_VOLUME_MEDIUM", "medium" },
          { GST_MODEM_VOLUME_LOUD,   "GST_MODEM_VOLUME_LOUD",   "loud" }
        };

      etype = g_enum_register_static ("GstModemVolume", values);
    }

  return etype;
}

GType
gst_dial_type_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static const GEnumValue values[] =
        {
	  { GST_DIAL_TYPE_TONES,  "GST_DIAL_TYPE_TONES",  "tones" },
	  { GST_DIAL_TYPE_PULSES, "GST_DIAL_TYPE_PULSES", "pulses" }
        };

      etype = g_enum_register_static ("GstDialType", values);
    }

  return etype;
}

GType
gst_iface_modem_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo iface_modem_info =
        {
          sizeof (GstIfaceModemClass),
          NULL,		/* base_init */
          NULL,		/* base_finalize */
          (GClassInitFunc) gst_iface_modem_class_init,
          NULL,		/* class_finalize */
          NULL,		/* class_data */
          sizeof (GstIfaceModem),
          0,		/* n_preallocs */
          (GInstanceInitFunc) gst_iface_modem_init,
        };

      type = g_type_register_static (GST_TYPE_IFACE, "GstIfaceModem",
                                     &iface_modem_info, 0);
    }

  return type;
}

static void
gst_iface_modem_class_init (GstIfaceModemClass *class)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (class);
  GstIfaceClass *iface_class  = GST_IFACE_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->set_property = gst_iface_modem_set_property;
  object_class->get_property = gst_iface_modem_get_property;
  object_class->finalize = gst_iface_modem_finalize;

  iface_class->get_iface_pixbuf = gst_iface_modem_get_pixbuf;
  iface_class->get_iface_type   = gst_iface_modem_get_iface_type;
  iface_class->get_iface_desc   = gst_iface_modem_get_desc;
  iface_class->has_gateway      = gst_iface_modem_has_gateway;
  iface_class->get_xml          = gst_iface_modem_impl_get_xml;
  
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
                                   PROP_SERIAL_PORT,
                                   g_param_spec_string ("iface_serial_port",
                                                        "Iface serial port",
                                                        "Serial port for the connection",
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
                                   PROP_VOLUME,
                                   g_param_spec_enum ("iface_volume",
                                                      "Iface volume",
                                                      "Volume for the connection",
                                                      GST_MODEM_VOLUME,
                                                      GST_MODEM_VOLUME_SILENT,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_DIAL_TYPE,
                                   g_param_spec_enum ("iface_dial_type",
                                                      "Iface dial type",
                                                      "Dial type for the connection",
                                                      GST_DIAL_TYPE,
                                                      GST_DIAL_TYPE_TONES,
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
}

static void
gst_iface_modem_init (GstIfaceModem *iface)
{
  g_return_if_fail (GST_IS_IFACE_MODEM (iface));

  iface->_priv = g_new0 (GstIfaceModemPriv, 1);
  iface->_priv->pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/ppp.png", NULL);

  iface->_priv->login = NULL;
  iface->_priv->password = NULL;
  iface->_priv->serial_port = NULL;
  iface->_priv->phone_number = NULL;
  iface->_priv->dial_prefix = NULL;
  iface->_priv->section = NULL;
}

static void
gst_iface_modem_finalize (GObject *object)
{
  GstIfaceModem *iface = GST_IFACE_MODEM (object);

  g_return_if_fail (GST_IS_IFACE_MODEM (object));

  if (iface->_priv)
    {
      g_free (iface->_priv->login);
      g_free (iface->_priv->password);
      g_free (iface->_priv->serial_port);
      g_free (iface->_priv->phone_number);
      g_free (iface->_priv->dial_prefix);
      g_free (iface->_priv->section);

      g_free (iface->_priv);
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gst_iface_modem_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GstIfaceModem *iface = GST_IFACE_MODEM (object);

  g_return_if_fail (GST_IS_IFACE_MODEM (iface));
  
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
    case PROP_SERIAL_PORT:
      g_free (iface->_priv->serial_port);
      iface->_priv->serial_port = g_value_dup_string (value);
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
    case PROP_VOLUME:
      iface->_priv->volume = g_value_get_enum (value);
      break;
    case PROP_DIAL_TYPE:
      iface->_priv->dial_type = g_value_get_enum (value);
      break;
    case PROP_DEFAULT_GW:
      iface->_priv->default_gw = g_value_get_boolean (value);
      break;
    case PROP_PERSIST:
      iface->_priv->persist = g_value_get_boolean (value);
      break;
    }
}

static void
gst_iface_modem_get_property (GObject      *object,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GstIfaceModem *iface = GST_IFACE_MODEM (object);

  g_return_if_fail (GST_IS_IFACE_MODEM (iface));
  
  switch (prop_id)
    {
    case PROP_LOGIN:
      g_value_set_string (value, iface->_priv->login);
      break;
    case PROP_PASSWORD:
      g_value_set_string (value, iface->_priv->password);
      break;
    case PROP_SERIAL_PORT:
      g_value_set_string (value, iface->_priv->serial_port);
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
    case PROP_VOLUME:
      g_value_set_enum (value, iface->_priv->volume);
      break;
    case PROP_DIAL_TYPE:
      g_value_set_enum (value, iface->_priv->dial_type);
      break;
    case PROP_DEFAULT_GW:
      g_value_set_boolean (value, iface->_priv->default_gw);
      break;
    case PROP_PERSIST:
      g_value_set_boolean (value, iface->_priv->persist);
      break;
    }
}

static const GdkPixbuf*
gst_iface_modem_get_pixbuf (GstIface *iface)
{
  return GST_IFACE_MODEM (iface)->_priv->pixbuf;
}

static const gchar*
gst_iface_modem_get_iface_type (GstIface *iface)
{
  return "modem";
}

static const gchar*
gst_iface_modem_get_desc (GstIface *iface)
{
  return _("Modem connection");
}

static gboolean
gst_iface_modem_has_gateway (GstIface *iface)
{
  return FALSE;
}

static void
gst_iface_modem_impl_get_xml (GstIface *iface, xmlNodePtr node)
{
  xmlNodePtr     configuration;
  GstIfaceModem *iface_modem;
  gchar         *str;

  g_return_if_fail (GST_IS_IFACE_MODEM (iface));
  iface_modem = GST_IFACE_MODEM (iface);

  if (gst_iface_is_configured (iface))
    {
      configuration = gst_xml_element_find_first (node, "configuration");
      if (!configuration)
        configuration = gst_xml_element_add (node, "configuration");

      gst_xml_set_child_content (configuration, "login",        iface_modem->_priv->login);
      gst_xml_set_child_content (configuration, "password",     iface_modem->_priv->password);
      gst_xml_set_child_content (configuration, "serial_port",  iface_modem->_priv->serial_port);
      gst_xml_set_child_content (configuration, "phone_number", iface_modem->_priv->phone_number);
      gst_xml_set_child_content (configuration, "external_line",  iface_modem->_priv->dial_prefix);

      str = g_strdup_printf ("%i", iface_modem->_priv->volume);
      gst_xml_set_child_content (configuration, "volume", str);
      g_free (str);

      str = (iface_modem->_priv->dial_type == GST_DIAL_TYPE_TONES) ?
	g_strdup ("ATDT") :
	g_strdup ("ATDP");
      gst_xml_set_child_content (configuration, "dial_command", str);
      g_free (str);

      gst_xml_set_child_content (configuration, "section",
                                 (iface_modem->_priv->section) ?
				 iface_modem->_priv->section :
				 gst_iface_get_dev (iface));

      gst_xml_element_set_boolean (configuration, "set_default_gw", iface_modem->_priv->default_gw);
      gst_xml_element_set_boolean (configuration, "persist", iface_modem->_priv->persist);
    }
  
  GST_IFACE_CLASS (parent_class)->get_xml (iface, node);
}

void
gst_iface_modem_set_config_from_xml (GstIfaceModem *iface,
				     xmlNodePtr     node)
{
  gchar      *login, *password, *serial_port, *phone_number, *dial_prefix, *section;
  gint        volume, dial_type;
  gboolean    default_gw, persist;
  xmlNodePtr  configuration;
  gchar      *str;

  /* config the parent class data */
  gst_iface_set_config_from_xml (GST_IFACE (iface), node);

  configuration = gst_xml_element_find_first (node, "configuration");
  if (!configuration)
    return;

  login        = gst_xml_get_child_content (configuration, "login");
  password     = gst_xml_get_child_content (configuration, "password");
  serial_port  = gst_xml_get_child_content (configuration, "serial_port");
  phone_number = gst_xml_get_child_content (configuration, "phone_number");
  dial_prefix  = gst_xml_get_child_content (configuration, "external_line");
  section      = gst_xml_get_child_content (configuration, "section");

  str = gst_xml_get_child_content (configuration, "volume");
  volume = g_strtod (str, NULL);
  g_free (str);

  str = gst_xml_get_child_content (configuration, "dial_command");
  if (str && (g_strcasecmp (str, "atdt") == 0))
    dial_type = GST_DIAL_TYPE_TONES;
  else
    dial_type = GST_DIAL_TYPE_PULSES;

  g_free (str);

  default_gw = gst_xml_element_get_boolean (configuration, "set_default_gw");
  persist    = gst_xml_element_get_boolean (configuration, "persist");

  g_object_set (G_OBJECT (iface),
                "iface-login", login,
                "iface-password", password,
                "iface-serial-port", serial_port,
                "iface-phone-number", phone_number,
                "iface-dial-prefix", dial_prefix,
                "iface-volume", volume,
                "iface-dial-type", dial_type,
                "iface-default-gw", default_gw,
                "iface-persist", persist,
                "iface-section", section,
                NULL);

  g_free (login);
  g_free (password);
  g_free (serial_port);
  g_free (phone_number);
  g_free (dial_prefix);
  g_free (section);
}

GstIfaceModem*
gst_iface_modem_new_from_xml (xmlNodePtr node)
{
  GstIfaceModem *iface;

  iface = g_object_new (GST_TYPE_IFACE_MODEM, NULL);
  gst_iface_modem_set_config_from_xml (iface, node);

  return iface;
}
