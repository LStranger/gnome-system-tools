/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2006 Carlos Garnacho
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

#include <oobs/oobs.h>
#include <string.h>
#include "network-locations.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MONITOR_TIMEOUT 500
#define GST_NETWORK_LOCATIONS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_NETWORK_LOCATIONS, GstNetworkLocationsPrivate))

typedef struct _GstNetworkLocationsPrivate GstNetworkLocationsPrivate;

struct _GstNetworkLocationsPrivate
{
  OobsSession *session;

  gchar *dot_dir;
  guint monitor_timeout;
  time_t last_mtime;
};

enum {
	CHANGED,
	LAST_SIGNAL
};

static gint signals [LAST_SIGNAL] = { 0 };

enum {
  TYPE_INT,
  TYPE_BOOLEAN,
  TYPE_STRING
};

typedef struct _PropType PropType;

struct _PropType {
  const gchar *key;
  gint type;
};

/* FIXME: oh, introspection, where are thou? */
PropType ethernet_properties[] = {
  { "auto", TYPE_BOOLEAN },
  { "active", TYPE_BOOLEAN },
  { "configured", TYPE_BOOLEAN },
  { "config-method", TYPE_INT },
  { "ip-address", TYPE_STRING },
  { "ip-mask", TYPE_STRING },
  { "gateway-address", TYPE_STRING },
  { "network-address", TYPE_STRING },
  { "broadcast-address", TYPE_STRING },
  { NULL }
};

PropType wireless_properties[] = {
  { "auto", TYPE_BOOLEAN },
  { "active", TYPE_BOOLEAN },
  { "configured", TYPE_BOOLEAN },
  { "essid", TYPE_STRING },
  { "key", TYPE_STRING },
  { "key-type", TYPE_INT },
  { "ip_address", TYPE_STRING },
  { "ip_mask", TYPE_STRING },
  { "gateway-address", TYPE_STRING },
  { "network-address", TYPE_STRING },
  { "broadcast-address", TYPE_STRING },
  { NULL }
};

PropType isdn_properties[] = {
  { "auto", TYPE_BOOLEAN },
  { "active", TYPE_BOOLEAN },
  { "configured", TYPE_BOOLEAN },
  { "login", TYPE_STRING },
  { "password", TYPE_STRING },
  { "phone-number", TYPE_STRING },
  { "phone-prefix", TYPE_STRING },
  { "default-gw", TYPE_BOOLEAN },
  { "peer-dns", TYPE_BOOLEAN },
  { "persistent", TYPE_BOOLEAN },
  { "peer-noauth", TYPE_BOOLEAN },
  { NULL }
};

PropType modem_properties[] = {
  { "auto", TYPE_BOOLEAN },
  { "active", TYPE_BOOLEAN },
  { "configured", TYPE_BOOLEAN },
  { "login", TYPE_STRING },
  { "password", TYPE_STRING },
  { "phone-number", TYPE_STRING },
  { "phone-prefix", TYPE_STRING },
  { "default-gw", TYPE_BOOLEAN },
  { "peer-dns", TYPE_BOOLEAN },
  { "persistent", TYPE_BOOLEAN },
  { "peer-noauth", TYPE_BOOLEAN },
  { "serial-port", TYPE_STRING },
  { "volume", TYPE_INT },
  { "dial-type", TYPE_INT },
  { NULL }
};

PropType plip_properties[] = {
  { "auto", TYPE_BOOLEAN },
  { "active", TYPE_BOOLEAN },
  { "configured", TYPE_BOOLEAN },
  { "address", TYPE_STRING },
  { "remote-address", TYPE_STRING },
  { NULL }
};


static void   gst_network_locations_class_init (GstNetworkLocationsClass *class);
static void   gst_network_locations_init       (GstNetworkLocations *locations);
static void   gst_network_locations_finalize   (GObject *object);


G_DEFINE_TYPE (GstNetworkLocations, gst_network_locations, G_TYPE_OBJECT);

static void
gst_network_locations_class_init (GstNetworkLocationsClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gst_network_locations_finalize;

  signals [CHANGED] =
    g_signal_new ("changed",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GstNetworkLocationsClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_type_class_add_private (object_class,
			    sizeof (GstNetworkLocationsPrivate));
}

#define GNOME_DOT_GNOME ".gnome2/"

static gchar*
create_dot_dir ()
{
  gchar *dir;

  dir = g_build_filename (g_get_home_dir (),
			  GNOME_DOT_GNOME,
			  "network-admin-locations",
			  NULL);

  if (!g_file_test (dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
    mkdir (dir, 0700);

  return dir;
}

static gboolean
on_monitor_timeout (gpointer data)
{
  GstNetworkLocationsPrivate *priv;
  struct stat st;

  priv = GST_NETWORK_LOCATIONS (data)->_priv;
  stat (priv->dot_dir, &st);

  if (st.st_mtime != priv->last_mtime)
    {
      priv->last_mtime = st.st_mtime;
      g_signal_emit (data, signals[CHANGED], 0);
    }

  return TRUE;
}

static void
gst_network_locations_init (GstNetworkLocations *locations)
{
  GstNetworkLocationsPrivate *priv;

  locations->_priv = priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);

  priv->session = oobs_session_get ();
  locations->ifaces_config = oobs_ifaces_config_get (priv->session);
  locations->hosts_config = oobs_hosts_config_get (priv->session);

  priv->dot_dir = create_dot_dir ();

  priv->monitor_timeout = g_timeout_add (MONITOR_TIMEOUT, on_monitor_timeout, locations);
}

static void
gst_network_locations_finalize (GObject *object)
{
  GstNetworkLocationsPrivate *priv;

  priv = GST_NETWORK_LOCATIONS (object)->_priv;

  g_free (priv->dot_dir);
  g_source_remove (priv->monitor_timeout);
  priv->monitor_timeout = 0;
}

GstNetworkLocations*
gst_network_locations_get (void)
{
  static GObject *locations = NULL;

  if (!locations)
    locations = g_object_new (GST_TYPE_NETWORK_LOCATIONS, NULL);

  return GST_NETWORK_LOCATIONS (locations);
}

GList*
gst_network_locations_get_names (GstNetworkLocations *locations)
{
  GstNetworkLocationsPrivate *priv;
  const gchar *name;
  GList *list = NULL;
  GDir *dir;

  g_return_val_if_fail (GST_IS_NETWORK_LOCATIONS (locations), NULL);

  priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);
  dir = g_dir_open (priv->dot_dir, 0, NULL);

  if (!dir)
    return NULL;

  while ((name = g_dir_read_name (dir)) != NULL)
    list = g_list_prepend (list, strdup (name));

  g_dir_close (dir);

  return list;
}

static GKeyFile*
get_location_key_file (GstNetworkLocations *locations,
		       const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  GKeyFile *key_file;
  gchar *path;

  priv = (GstNetworkLocationsPrivate *) locations->_priv;
  key_file = g_key_file_new ();
  g_key_file_set_list_separator (key_file, ',');
  path = g_build_filename (priv->dot_dir, name, NULL);

  if (!g_key_file_load_from_file (key_file, path, 0, NULL))
    {
      g_key_file_free (key_file);
      return NULL;
    }

  return key_file;
}

static GList*
array_to_list (const gchar **str_list)
{
  GList *list = NULL;
  gchar **pos = (gchar**) str_list;

  while (*pos)
    {
      list = g_list_prepend (list, g_strdup (*pos));
      pos++;
    }

  list = g_list_reverse (list);
  return list;
}

static gboolean
compare_string (const gchar *str1,
		const gchar *str2)
{
  if (!str1)
    str1 = "";

  if (!str2)
    str2 = "";

  return (strcmp (str1, str2) == 0);
}

static gboolean
compare_list (GList *list1,
	      GList *list2)
{
  GList *elem1, *elem2;

  elem1 = list1;
  elem2 = list2;

  while (elem1 && elem2)
    {
      if (!compare_string (elem1->data, elem2->data))
	return FALSE;

      elem1 = elem1->next;
      elem2 = elem2->next;
    }

  if (!elem1 && !elem2)
    return TRUE;

  return FALSE;
}

static gboolean
compare_string_list (GKeyFile    *key_file,
		     const gchar *key,
		     GList       *list)
{
  GList *list1;
  gchar **str_list;
  gboolean result;
  
  str_list = g_key_file_get_string_list (key_file, "general", key, NULL, NULL);
  list1 = array_to_list ((const gchar**) str_list);
  g_strfreev (str_list);

  result = compare_list (list1, list);

  g_list_foreach (list1, (GFunc) g_free, NULL);
  g_list_free (list1);

  return result;
}

static gboolean
compare_static_host (const gchar    *str,
		     OobsStaticHost *static_host)
{
  gchar **split, *ip_address;
  GList *list1, *list2;
  gboolean equal = TRUE;

  split = g_strsplit (str, ";", -1);
  ip_address = split[0];
  list1 = array_to_list ((const gchar**) &split[1]);

  equal = compare_string (ip_address, oobs_static_host_get_ip_address (static_host));

  if (equal)
    {
      list2 = oobs_static_host_get_aliases (static_host);
      equal = compare_list (list1, list2);
    }

  g_strfreev (split);
  g_list_foreach (list1, (GFunc) g_free, NULL);
  g_list_free (list1);
  g_list_free (list2);

  return equal;
}

static gboolean
compare_static_hosts (OobsHostsConfig *hosts_config,
		      GKeyFile        *key_file)
{
  gchar **str_list, **elem;
  OobsList *static_hosts;
  OobsListIter iter;
  OobsStaticHost *static_host;
  gboolean valid, equal = TRUE;

  elem = str_list = g_key_file_get_string_list (key_file, "general", "static-hosts", NULL, NULL);
  static_hosts = oobs_hosts_config_get_static_hosts (hosts_config);
  valid = oobs_list_get_iter_first (static_hosts, &iter);

  while (*elem && valid && equal)
    {
      static_host = OOBS_STATIC_HOST (oobs_list_get (static_hosts, &iter));
      equal = compare_static_host (*elem, static_host);
      g_object_unref (static_host);

      valid = oobs_list_iter_next (static_hosts, &iter);
      elem++;
    }

  if (*elem || valid)
    equal = FALSE;

  g_strfreev (str_list);

  return equal;
}

static gboolean
compare_hosts_config (OobsHostsConfig *hosts_config,
		      GKeyFile        *key_file)
{
  GList *list;
  gboolean result;
  gchar *str;

  str = g_key_file_get_string (key_file, "general", "hostname", NULL);
  result = compare_string (str, oobs_hosts_config_get_hostname (hosts_config));
  g_free (str);

  if (!result)
    return FALSE;

  str = g_key_file_get_string (key_file, "general", "domainname", NULL);
  result = compare_string (str, oobs_hosts_config_get_domainname (hosts_config));
  g_free (str);

  if (!result)
    return FALSE;

  list = oobs_hosts_config_get_dns_servers (hosts_config);
  result = compare_string_list (key_file, "dns-servers", list);
  g_list_free (list);

  if (!result)
    return FALSE;

  list = oobs_hosts_config_get_search_domains (hosts_config);
  result = compare_string_list (key_file, "search-domains", list);
  g_list_free (list);

  if (!result)
    return FALSE;

  if (!compare_static_hosts (hosts_config, key_file))
    return FALSE;

  return TRUE;
}

static gboolean
compare_interface (GObject  *iface,
		   PropType  props[],
		   GKeyFile *key_file)
{
  gchar *name, *value1, *value2;
  gint int_value1, int_value2, i = 0;
  gboolean bool_value1, bool_value2, equal = TRUE;

  g_object_get (iface, "device", &name, NULL);

  while (props[i].key && equal)
    {
      if (props[i].type == TYPE_STRING)
	{
	  value1 = g_key_file_get_string (key_file, name, props[i].key, NULL);
	  g_object_get (iface, props[i].key, &value2, NULL);

	  equal = compare_string (value1, value2);
	  g_free (value1);
	}
      else if (props[i].type == TYPE_INT)
	{
	  int_value1 = g_key_file_get_integer (key_file, name, props[i].key, NULL);
	  g_object_get (iface, props[i].key, &int_value2, NULL);
	  equal = (int_value1 == int_value2);
	}
      else
	{
	  bool_value1 = g_key_file_get_boolean (key_file, name, props[i].key, NULL);
	  g_object_get (iface, props[i].key, &bool_value2, NULL);
	  equal = ((bool_value1 == TRUE) == (bool_value2 == TRUE));
	}

      i++;
    }

  return equal;
}

/* FIXME: merge with save_interfaces_list */
static gboolean
compare_interfaces_list (OobsIfacesConfig *config,
			 gint              iface_type,
			 PropType          props[],
			 GKeyFile         *key_file)
{

  OobsList *list;
  OobsListIter iter;
  gboolean valid, equal = TRUE;
  GObject *iface;

  list = oobs_ifaces_config_get_ifaces (config, iface_type);
  valid = oobs_list_get_iter_first (list, &iter);

  while (valid && equal)
    {
      iface = oobs_list_get (list, &iter);
      equal = compare_interface (iface, props, key_file);
      g_object_unref (iface);

      valid = oobs_list_iter_next (list, &iter);
    }

  return equal;
}

static gboolean
compare_interfaces (OobsIfacesConfig *config,
		    GKeyFile         *key_file)
{
  if (!compare_interfaces_list (config, OOBS_IFACE_TYPE_ETHERNET, ethernet_properties, key_file) ||
      !compare_interfaces_list (config, OOBS_IFACE_TYPE_WIRELESS, wireless_properties, key_file) ||
      !compare_interfaces_list (config, OOBS_IFACE_TYPE_IRLAN, ethernet_properties, key_file) ||
      !compare_interfaces_list (config, OOBS_IFACE_TYPE_PLIP, plip_properties, key_file) ||
      !compare_interfaces_list (config, OOBS_IFACE_TYPE_MODEM, modem_properties, key_file) ||
      !compare_interfaces_list (config, OOBS_IFACE_TYPE_ISDN, isdn_properties, key_file))
    return FALSE;

  return TRUE;
}

static gboolean
compare_location (GstNetworkLocations *locations,
		  const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  GKeyFile *key_file;

  priv = (GstNetworkLocationsPrivate *) locations->_priv;
  key_file = get_location_key_file (locations, name);

  if (!key_file)
    return FALSE;

  if (compare_hosts_config (OOBS_HOSTS_CONFIG (locations->hosts_config), key_file) &&
      compare_interfaces (OOBS_IFACES_CONFIG (locations->ifaces_config), key_file))
    {
      g_key_file_free (key_file);
      return TRUE;
    }

  g_key_file_free (key_file);
  return FALSE;
}

gchar*
gst_network_locations_get_current (GstNetworkLocations *locations)
{
  GList *names, *elem;
  gchar *location = NULL;

  names = elem = gst_network_locations_get_names (locations);

  while (elem && !location)
    {
      if (compare_location (locations, elem->data))
	location = g_strdup (elem->data);

      elem = elem->next;
    }

  g_list_foreach (names, (GFunc) g_free, NULL);
  g_list_free (names);

  return location;
}

static void
set_static_hosts (OobsHostsConfig *hosts_config,
		  GKeyFile        *key_file)
{
  gchar **str_list, **elem, **split, *ip_address;
  OobsList *static_hosts;
  OobsListIter iter;
  OobsStaticHost *static_host;
  GList *list;

  elem = str_list = g_key_file_get_string_list (key_file, "general", "static-hosts", NULL, NULL);
  static_hosts = oobs_hosts_config_get_static_hosts (hosts_config);
  oobs_list_clear (static_hosts);

  while (*elem)
    {
      split = g_strsplit (*elem, ";", -1);
      ip_address = split[0];
      list = array_to_list ((const gchar**) &split[1]);

      static_host = oobs_static_host_new (ip_address, list);
      oobs_list_append (static_hosts, &iter);
      oobs_list_set (static_hosts, &iter, static_host);
      g_strfreev (split);

      elem++;
    }

  g_strfreev (str_list);
}

static void
set_hosts_config (OobsHostsConfig *hosts_config,
		  GKeyFile        *key_file)
{
  GList *list;
  gchar **str_list;
  gchar *str;

  str = g_key_file_get_string (key_file, "general", "hostname", NULL);
  oobs_hosts_config_set_hostname (hosts_config, str);
  g_free (str);

  str = g_key_file_get_string (key_file, "general", "domainname", NULL);
  oobs_hosts_config_set_domainname (hosts_config, str);
  g_free (str);

  str_list = g_key_file_get_string_list (key_file, "general", "dns-servers", NULL, NULL);
  list = array_to_list ((const gchar**) str_list);
  oobs_hosts_config_set_dns_servers (hosts_config, list);
  g_strfreev (str_list);

  str_list = g_key_file_get_string_list (key_file, "general", "search-domains", NULL, NULL);
  list = array_to_list ((const gchar**) str_list);
  oobs_hosts_config_set_search_domains (hosts_config, list);
  g_strfreev (str_list);

  set_static_hosts (hosts_config, key_file);
}

static void
set_interface (GObject  *iface,
	       PropType  props[],
	       GKeyFile *key_file)
{
  gchar *name, *value;
  gint int_value;
  gboolean bool_value;
  gint i = 0;

  if (!props)
    return;

  g_object_get (iface, "device", &name, NULL);

  while (props[i].key)
    {
      if (props[i].type == TYPE_STRING)
	{
	  value = g_key_file_get_string (key_file, name, props[i].key, NULL);
	  g_object_set (iface, props[i].key, value, NULL);
	}
      else if (props[i].type == TYPE_INT)
	{
	  int_value = g_key_file_get_integer (key_file, name, props[i].key, NULL);
	  g_object_set (iface, props[i].key, int_value, NULL);
	}
      else
	{
	  bool_value = g_key_file_get_boolean (key_file, name, props[i].key, NULL);
	  g_object_set (iface, props[i].key, bool_value, NULL);
	}

      i++;
    }
}

/* FIXME: merge with save_interfaces_list */
static void
set_interfaces_list (OobsIfacesConfig *config,
		     gint              iface_type,
		     PropType          props[],
		     GKeyFile         *key_file)
{
  OobsList *list;
  OobsListIter iter;
  gboolean valid;
  GObject *iface;

  list = oobs_ifaces_config_get_ifaces (config, iface_type);
  valid = oobs_list_get_iter_first (list, &iter);

  while (valid)
    {
      iface = oobs_list_get (list, &iter);
      set_interface (iface, props, key_file);
      g_object_unref (iface);

      valid = oobs_list_iter_next (list, &iter);
    }
}

static void
set_interfaces_config (OobsIfacesConfig *config,
		       GKeyFile         *key_file)
{
  set_interfaces_list (config, OOBS_IFACE_TYPE_ETHERNET, ethernet_properties, key_file);
  set_interfaces_list (config, OOBS_IFACE_TYPE_WIRELESS, wireless_properties, key_file);
  set_interfaces_list (config, OOBS_IFACE_TYPE_IRLAN, ethernet_properties, key_file);
  set_interfaces_list (config, OOBS_IFACE_TYPE_PLIP, plip_properties, key_file);
  set_interfaces_list (config, OOBS_IFACE_TYPE_MODEM, modem_properties, key_file);
  set_interfaces_list (config, OOBS_IFACE_TYPE_ISDN, isdn_properties, key_file);
}

gboolean
gst_network_locations_set_location (GstNetworkLocations *locations,
				    const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  GKeyFile *key_file;

  priv = (GstNetworkLocationsPrivate *) locations->_priv;
  key_file = get_location_key_file (locations, name);

  if (!key_file)
    return FALSE;

  set_hosts_config (OOBS_HOSTS_CONFIG (locations->hosts_config), key_file);
  set_interfaces_config (OOBS_IFACES_CONFIG (locations->ifaces_config), key_file);
  g_key_file_free (key_file);

  return TRUE;
}

static gchar**
list_to_array (GList *list)
{
  gchar **arr;
  gint i = 0;

  arr = g_new0 (gchar*, g_list_length (list) + 1);

  while (list)
    {
      arr[i] = g_strdup (list->data);
      list = list->next;
      i++;
    }

  return arr;
}

static gchar*
concatenate_aliases (OobsStaticHost *static_host)
{
  GList *list, *elem;
  GString *str;
  gchar *s;

  str = g_string_new ("");
  list = elem = oobs_static_host_get_aliases (OOBS_STATIC_HOST (static_host));

  while (elem)
    {
      g_string_append_printf (str, ";%s", elem->data);
      elem = elem->next;
    }

  s = str->str;
  g_string_free (str, FALSE);
  g_list_free (list);

  return s;
}

static void
save_static_hosts (OobsHostsConfig *config,
		   GKeyFile        *key_file)
{
  OobsList *list;
  OobsListIter iter;
  gboolean valid;
  GObject *static_host;
  GString *str;
  gchar **arr;
  gint i = 0;

  list = oobs_hosts_config_get_static_hosts (config);
  valid = oobs_list_get_iter_first (list, &iter);
  arr = g_new0 (gchar*, oobs_list_get_n_items (list) + 1);

  while (valid)
    {
      static_host = oobs_list_get (list, &iter);

      str = g_string_new (oobs_static_host_get_ip_address (OOBS_STATIC_HOST (static_host)));
      g_string_append (str, concatenate_aliases (OOBS_STATIC_HOST (static_host)));
      arr[i] = str->str;

      g_string_free (str, FALSE);
      g_object_unref (static_host);
      valid = oobs_list_iter_next (list, &iter);
      i++;
    }

  g_key_file_set_string_list (key_file, "general", "static-hosts",
			      (const gchar**) arr, g_strv_length (arr));
}

static void
save_hosts_config (OobsHostsConfig *config,
		   GKeyFile        *key_file)
{
  GList *list;
  gchar **arr;
  const gchar *str;

  str = oobs_hosts_config_get_hostname (config);
  g_key_file_set_string (key_file, "general", "hostname", (str) ? str : "");

  str = oobs_hosts_config_get_domainname (config);
  g_key_file_set_string (key_file, "general", "domainname", (str) ? str : "");

  list = oobs_hosts_config_get_dns_servers (config);
  arr = list_to_array (list);
  g_key_file_set_string_list (key_file, "general", "dns-servers",
			      (const gchar**) arr, g_strv_length (arr));
  g_list_free (list);
  g_strfreev (arr);

  list = oobs_hosts_config_get_search_domains (config);
  arr = list_to_array (list);
  g_key_file_set_string_list (key_file, "general", "search-domains",
			      (const gchar**) arr, g_strv_length (arr));
  g_list_free (list);
  g_strfreev (arr);

  save_static_hosts (config, key_file);
}

static void
save_interface (GObject     *iface,
		PropType     props[],
		GKeyFile    *key_file)
{
  gchar *name, *value;
  gint int_value;
  gboolean bool_value;
  gint i = 0;

  if (!props)
    return;

  g_object_get (iface, "device", &name, NULL);

  while (props[i].key)
    {
      if (props[i].type == TYPE_STRING)
	{
	  g_object_get (iface, props[i].key, &value, NULL);
	  g_key_file_set_string (key_file, name, props[i].key, (value) ? value : "");
	}
      else if (props[i].type == TYPE_INT)
	{
	  g_object_get (iface, props[i].key, &int_value, NULL);
	  g_key_file_set_integer (key_file, name, props[i].key, int_value);
	}
      else
	{
	  g_object_get (iface, props[i].key, &bool_value, NULL);
	  g_key_file_set_boolean (key_file, name, props[i].key, bool_value);
	}

      i++;
    }
}

static void
save_interfaces_list (OobsIfacesConfig *config,
		      gint              iface_type,
		      PropType          props[],
		      GKeyFile         *key_file)
{
  OobsList *list;
  OobsListIter iter;
  gboolean valid;
  GObject *iface;

  list = oobs_ifaces_config_get_ifaces (config, iface_type);
  valid = oobs_list_get_iter_first (list, &iter);

  while (valid)
    {
      iface = oobs_list_get (list, &iter);
      save_interface (iface, props, key_file);
      g_object_unref (iface);

      valid = oobs_list_iter_next (list, &iter);
    }
}

static void
save_interfaces (OobsIfacesConfig *config,
		 GKeyFile         *key_file)
{
  save_interfaces_list (config, OOBS_IFACE_TYPE_ETHERNET, ethernet_properties, key_file);
  save_interfaces_list (config, OOBS_IFACE_TYPE_WIRELESS, wireless_properties, key_file);
  save_interfaces_list (config, OOBS_IFACE_TYPE_IRLAN, ethernet_properties, key_file);
  save_interfaces_list (config, OOBS_IFACE_TYPE_PLIP, plip_properties, key_file);
  save_interfaces_list (config, OOBS_IFACE_TYPE_MODEM, modem_properties, key_file);
  save_interfaces_list (config, OOBS_IFACE_TYPE_ISDN, isdn_properties, key_file);
}

static gboolean
save_current (GstNetworkLocations *locations,
	      const gchar         *name)
{
  GKeyFile *key_file;
  GstNetworkLocationsPrivate *priv;
  gchar *contents, *filename, *path;

  priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);

  key_file = g_key_file_new ();
  g_key_file_set_list_separator (key_file, ',');
  save_hosts_config (OOBS_HOSTS_CONFIG (locations->hosts_config), key_file);
  save_interfaces (OOBS_IFACES_CONFIG (locations->ifaces_config), key_file);

  contents = g_key_file_to_data (key_file, NULL, NULL);
  g_key_file_free (key_file);

  if (!contents)
    return FALSE;

  filename = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
  path = g_build_filename (priv->dot_dir, filename, NULL);

  return g_file_set_contents (path, contents, -1, NULL);
}

gboolean
gst_network_locations_save_current (GstNetworkLocations *locations,
				    const gchar         *name)
{
  g_return_val_if_fail (GST_IS_NETWORK_LOCATIONS (locations), FALSE);
  g_return_val_if_fail (name && *name, FALSE);

  /* Unset the previous configuration with the same name, if any */
  gst_network_locations_delete_location (locations, name);

  return save_current (locations, name);
}

gboolean
gst_network_locations_delete_location (GstNetworkLocations *locations,
				       const gchar         *name)
{
  GstNetworkLocationsPrivate *priv;
  gchar *location_path;
  gboolean success;

  g_return_val_if_fail (GST_IS_NETWORK_LOCATIONS (locations), FALSE);

  priv = GST_NETWORK_LOCATIONS_GET_PRIVATE (locations);
  location_path = g_build_filename (priv->dot_dir, name, NULL);

  g_unlink (location_path);
  g_free (location_path);

  return success;
}
