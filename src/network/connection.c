/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
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

#include <string.h>

#include <glib/gi18n.h>
#include "gst.h"
#include "network-tool.h"
#include "connection.h"
#include "essid-list.h"
#include "nm-integration.h"

extern GstTool *tool;

typedef struct
{
  const gchar *method;
  const gchar *desc;
} Description;

static Description method_descriptions [] = {
  { "dhcp",   N_("Automatic configuration (DHCP)") },
  { "ipv4ll", N_("Local Zeroconf network (IPv4 LL)") },
  { "static", N_("Static IP address") },
};

static Description key_type_descriptions [] = {
  { "wep-ascii", N_("WEP key (ascii)") },
  { "wep-hex", N_("WEP key (hexadecimal)") },
  { "wpa-psk", N_("WPA") },
};


/* helper for getting whether the string or null if it's empty */
static gchar*
get_entry_text (GtkWidget *entry)
{
  gchar *str;

  str = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
  return (!str || !*str) ? NULL : str;
}

void
connection_combo_set_value (GtkComboBox *combo,
			    const gchar *method)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean valid;
  gchar *elem_method;

  g_return_if_fail (method != NULL);

  model = gtk_combo_box_get_model (combo);
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model, &iter, 1, &elem_method, -1);

      if (strcmp (elem_method, method) == 0)
	{
	  gtk_combo_box_set_active_iter (combo, &iter);
	  valid = FALSE;
	}
      else
	valid = gtk_tree_model_iter_next (model, &iter);

      g_free (elem_method);
    }
}

gchar *
connection_combo_get_value (GtkComboBox *combo)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *ret = NULL;

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      model = gtk_combo_box_get_model (combo);
      gtk_tree_model_get (model, &iter, 1, &ret, -1);
    }

  return ret;
}

static void
connection_essids_combo_init (GtkComboBoxEntry *combo)
{
  GtkTreeModel *model;
  GtkCellRenderer *renderer;

  model = GTK_TREE_MODEL (gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT));
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
  g_object_unref (model);

  /* add "crypted" renderer */
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
			      renderer, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo),
				 renderer, "pixbuf", 0);
  gtk_cell_layout_reorder (GTK_CELL_LAYOUT (combo), renderer, 0);

  /* add "quality" renderer */
  renderer = gtk_cell_renderer_progress_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
			      renderer, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo),
				 renderer, "value", 2);

  /* reuse text cell renderer for the essid */
  gtk_combo_box_entry_set_text_column (combo, 1);
}

static void
ethernet_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar *address, *netmask, *gateway, *config_method;

  g_object_get (G_OBJECT (dialog->iface),
		"ip-address", &address,
		"ip-mask", &netmask,
		"gateway-address", &gateway,
		"config-method", &config_method,
		NULL);

  connection_combo_set_value (GTK_COMBO_BOX (dialog->bootproto_combo), config_method);
  gtk_entry_set_text (GTK_ENTRY (dialog->address), (address) ? address : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->netmask), (netmask) ? netmask : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->gateway), (gateway) ? gateway : "");

  g_free (config_method);
  g_free (address);
  g_free (netmask);
  g_free (gateway);
}

static void
ethernet_dialog_save (GstConnectionDialog *dialog, gboolean active)
{
  gchar *config_method = connection_combo_get_value (GTK_COMBO_BOX (dialog->bootproto_combo));

  g_object_set (G_OBJECT (dialog->iface),
		"ip-address",   get_entry_text (dialog->address),
		"ip-mask",   get_entry_text (dialog->netmask),
		"gateway-address",   get_entry_text (dialog->gateway),
		"config-method", config_method,
		NULL);

  g_free (config_method);
}

static gboolean
ethernet_dialog_check_fields (GstConnectionDialog *dialog)
{
  gchar *address, *netmask, *gateway, *config_method;
  gboolean active, ret;

  address = get_entry_text (dialog->address);
  netmask = get_entry_text (dialog->netmask);
  gateway = get_entry_text (dialog->gateway);
  config_method = connection_combo_get_value (GTK_COMBO_BOX (dialog->bootproto_combo));

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured));

  ret = (!active ||
	 config_method && strcmp (config_method, "static") != 0 ||
	 (gst_filter_check_ip_address (address) == GST_ADDRESS_IPV4 &&
	  gst_filter_check_ip_address (netmask) == GST_ADDRESS_IPV4 &&
	  (!gateway || gst_filter_check_ip_address (gateway) == GST_ADDRESS_IPV4)));

  g_free (config_method);
  return ret;
}

#ifdef HAVE_LIBIW_H
/* perhaps there should be a GstEssidListModel class to hide this
 * stuff, but I'll leave that as a code beautification exercise */
static void
on_essid_list_changed (GstEssidList        *list,
		       GstConnectionDialog *dialog)
{
  GList *elem;
  GstEssidListEntry *entry;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GdkPixbuf *locked, *unlocked, *pixbuf;

  elem = gst_essid_list_get_list (dialog->essid_list);
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (dialog->essid));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  locked = gtk_icon_theme_load_icon (tool->icon_theme, "gnome-dev-wavelan-encrypted", 16, 0, NULL);
  unlocked = gtk_icon_theme_load_icon (tool->icon_theme, "gnome-dev-wavelan", 16, 0, NULL);

  while (elem)
    {
      entry = elem->data;
      pixbuf = (entry->encrypted) ? locked : unlocked;

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			  0, pixbuf,
			  1, entry->essid,
			  2, (gint) (entry->quality * 100),
			  -1);
      elem = elem->next;
    }

  if (locked)
    g_object_unref (locked);

  if (unlocked)
    g_object_unref (unlocked);
}
#endif

static void
wireless_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar *essid, *key, *dev, *key_type;

  g_object_get (G_OBJECT (dialog->iface),
		"device", &dev,
		"essid", &essid,
		"key", &key,
		"key-type", &key_type,
		NULL);

  connection_combo_set_value (GTK_COMBO_BOX (dialog->key_type_combo), key_type);
  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dialog->essid)->child), (essid) ? essid : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->wep_key), (key) ? key : "");

#ifdef HAVE_LIBIW_H
  dialog->essid_list = gst_essid_list_new (dev);
  on_essid_list_changed (dialog->essid_list, dialog);
  g_signal_connect (G_OBJECT (dialog->essid_list), "changed",
		    G_CALLBACK (on_essid_list_changed), dialog);
#endif

  g_free (key_type);
  g_free (essid);
  g_free (key);
  g_free (dev);
}

static void
wireless_dialog_save (GstConnectionDialog *dialog)
{
  gchar *key_type;

  key_type = connection_combo_get_value (GTK_COMBO_BOX (dialog->key_type_combo));
  g_object_set (G_OBJECT (dialog->iface),
		"essid",   get_entry_text (GTK_BIN (dialog->essid)->child),
		"key", get_entry_text (dialog->wep_key),
		"key-type", key_type,
		NULL);
  g_free (key_type);
}

static void
plip_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar *local_address, *remote_address;

  g_object_get (G_OBJECT (dialog->iface),
		"address",  &local_address,
		"remote-address", &remote_address,
		NULL);

  gtk_entry_set_text (GTK_ENTRY (dialog->local_address),
		      (local_address) ? local_address : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->remote_address),
		      (remote_address) ? remote_address : "");

  g_free (local_address);
  g_free (remote_address);
}

static void
plip_dialog_save (GstConnectionDialog *dialog)
{
  g_object_set (G_OBJECT (dialog->iface),
		"address",  get_entry_text (dialog->local_address),
		"remote-address", get_entry_text (dialog->remote_address),
		NULL);
}

static gboolean
plip_dialog_check_fields (GstConnectionDialog *dialog)
{
  gchar *local_address, *remote_address;

  local_address  = get_entry_text (dialog->local_address);
  remote_address = get_entry_text (dialog->remote_address);

  return ((!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured))) ||
	  ((gst_filter_check_ip_address (local_address) == GST_ADDRESS_IPV4) &&
	   (gst_filter_check_ip_address (remote_address) == GST_ADDRESS_IPV4)));
}

static void
isdn_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar    *login, *password, *phone_number, *phone_prefix;
  gboolean  default_gw, peerdns, persist;

  g_object_get (G_OBJECT (dialog->iface),
                "login", &login,
                "password", &password,
                "phone-number", &phone_number,
                "phone-prefix", &phone_prefix,
                "default-gw", &default_gw,
                "peer-dns", &peerdns,
                "persistent", &persist,
                NULL);

  gtk_entry_set_text (GTK_ENTRY (dialog->login),
                      (login) ? login : "");

  gtk_entry_set_text (GTK_ENTRY (dialog->password),
                      (password) ? password : "");

  gtk_entry_set_text (GTK_ENTRY (dialog->phone_number),
                      (phone_number) ? phone_number : "");

  gtk_entry_set_text (GTK_ENTRY (dialog->dial_prefix),
                      (phone_prefix) ? phone_prefix : "");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->default_gw), default_gw);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->peerdns), peerdns);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->persist), persist);

  g_free (login);
  g_free (password);
  g_free (phone_number);
  g_free (phone_prefix);
}

static void
isdn_dialog_save (GstConnectionDialog *dialog)
{
  g_object_set (G_OBJECT (dialog->iface),
                "login",        get_entry_text (dialog->login),
                "password",     get_entry_text (dialog->password),
                "phone-number", get_entry_text (dialog->phone_number),
                "phone-prefix", get_entry_text (dialog->dial_prefix),
                "default-gw",   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->default_gw)),
                "peer-dns",     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->peerdns)),
                "persistent",   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->persist)),
                NULL);
}

static gboolean
isdn_dialog_check_fields (GstConnectionDialog *dialog)
{
  return ((!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured))) ||
	  ((get_entry_text (dialog->login)) &&
	   (get_entry_text (dialog->phone_number))));
}

static void
modem_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar *serial_port;
  gint   volume, dial_type;

  g_object_get (G_OBJECT (dialog->iface),
                "serial-port", &serial_port,
                "volume", &volume,
                "dial-type", &dial_type,
                NULL);

  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dialog->serial_port)->child),
                      (serial_port) ? serial_port : "");

  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->volume), volume);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->dial_type), dial_type);

  g_free (serial_port);
}

static void
modem_dialog_save (GstConnectionDialog *dialog)
{
  g_object_set (G_OBJECT (dialog->iface),
                "serial-port",  get_entry_text (GTK_BIN (dialog->serial_port)->child),
                "volume",       gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->volume)),
                "dial-type",    gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->dial_type)),
		NULL);
}

static gboolean
modem_dialog_check_fields (GstConnectionDialog *dialog)
{
  return ((!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured))) ||
	  (get_entry_text (GTK_BIN (dialog->serial_port)->child)));
}

static int
compare_methods (Description *m1, Description *m2)
{
  return strcmp (m1->method, m2->method);
}

static const gchar*
method_to_desc (const Description  descriptions[],
		gint               n_elems,
		const gchar       *method)
{
  Description d, *ret;

  d.method = (gchar*) method;
  ret = (Description *) bsearch (&d, descriptions, n_elems,
				 sizeof (Description), compare_methods);

  return (ret) ? ret->desc : method;
}

static void
populate_combo (GtkComboBox  *combo,
		GList        *list,
		Description   descriptions[],
		gint          n_elems)
{
  GtkListStore *store;
  GtkTreeIter iter;
  const gchar *desc;

  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  while (list)
    {
      desc = method_to_desc (descriptions, n_elems, list->data);
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, _(desc),
			  1, list->data,
			  -1);
      list = list->next;
    }

  gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));
  g_object_unref (store);
}

GstConnectionDialog*
connection_dialog_init (GstTool *tool)
{
  GstConnectionDialog *gcd;

  gcd = g_new0 (GstConnectionDialog, 1);

  gcd->standalone = FALSE;
  gcd->iface  = NULL;
  gcd->dialog = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");

  gcd->ok_button = gst_dialog_get_widget (tool->main_dialog, "connection_ok");

  gcd->notebook = gst_dialog_get_widget (tool->main_dialog, "connection_notebook");
  gcd->general_page = gst_dialog_get_widget (tool->main_dialog, "connection_general_page");
  gcd->modem_page = gst_dialog_get_widget (tool->main_dialog, "connection_modem_page");
  gcd->options_page = gst_dialog_get_widget (tool->main_dialog, "connection_options_page");

  gcd->connection_configured = gst_dialog_get_widget (tool->main_dialog, "connection_device_active");
  gcd->roaming_configured = gst_dialog_get_widget (tool->main_dialog, "connection_device_roaming");

  /* ethernet */
  gcd->bootproto_combo = gst_dialog_get_widget (tool->main_dialog, "connection_bootproto");
  gcd->address = gst_dialog_get_widget (tool->main_dialog, "connection_address");
  gcd->netmask = gst_dialog_get_widget (tool->main_dialog, "connection_netmask");
  gcd->gateway = gst_dialog_get_widget (tool->main_dialog, "connection_gateway");
  populate_combo (GTK_COMBO_BOX (gcd->bootproto_combo),
		  oobs_ifaces_config_get_available_configuration_methods (GST_NETWORK_TOOL (tool)->ifaces_config),
		  method_descriptions, G_N_ELEMENTS (method_descriptions));

  /* wireless */
  gcd->essid   = gst_dialog_get_widget (tool->main_dialog, "connection_essid");
  gcd->wep_key = gst_dialog_get_widget (tool->main_dialog, "connection_wep_key");
  gcd->key_type_combo = gst_dialog_get_widget (tool->main_dialog, "connection_wep_key_type");
  populate_combo (GTK_COMBO_BOX (gcd->key_type_combo),
		  oobs_ifaces_config_get_available_key_types (GST_NETWORK_TOOL (tool)->ifaces_config),
		  key_type_descriptions, G_N_ELEMENTS (key_type_descriptions));

  /* plip */
  gcd->local_address  = gst_dialog_get_widget (tool->main_dialog, "connection_local_address");
  gcd->remote_address = gst_dialog_get_widget (tool->main_dialog, "connection_remote_address");

  /* modem */
  gcd->login        = gst_dialog_get_widget (tool->main_dialog, "connection_login");
  gcd->password     = gst_dialog_get_widget (tool->main_dialog, "connection_password");
  gcd->serial_port  = gst_dialog_get_widget (tool->main_dialog, "connection_serial_port");
  gcd->detect_modem = gst_dialog_get_widget (tool->main_dialog, "connection_detect_modem");
  gcd->phone_number = gst_dialog_get_widget (tool->main_dialog, "connection_phone_number");
  gcd->dial_prefix  = gst_dialog_get_widget (tool->main_dialog, "connection_dial_prefix");
  gcd->volume       = gst_dialog_get_widget (tool->main_dialog, "connection_volume");
  gcd->dial_type    = gst_dialog_get_widget (tool->main_dialog, "connection_dial_type");
  gcd->default_gw   = gst_dialog_get_widget (tool->main_dialog, "connection_default_gw");
  gcd->peerdns      = gst_dialog_get_widget (tool->main_dialog, "connection_peerdns");
  gcd->persist      = gst_dialog_get_widget (tool->main_dialog, "connection_persist");

  gcd->wireless_frame = gst_dialog_get_widget (tool->main_dialog, "connection_wireless");
  gcd->ethernet_frame = gst_dialog_get_widget (tool->main_dialog, "connection_ethernet");
  gcd->plip_frame     = gst_dialog_get_widget (tool->main_dialog, "connection_plip");
  gcd->isp_frame      = gst_dialog_get_widget (tool->main_dialog, "isp_data");
  gcd->account_frame  = gst_dialog_get_widget (tool->main_dialog, "isp_account_data");

  connection_essids_combo_init (GTK_COMBO_BOX_ENTRY (gcd->essid));

  return gcd;
}

static void
prepare_dialog_layout (GstConnectionDialog *dialog,
		       OobsIface           *iface)
{
  if (OOBS_IS_IFACE_ISDN (iface))
    {
      gtk_widget_hide (dialog->wireless_frame);
      gtk_widget_hide (dialog->ethernet_frame);
      gtk_widget_hide (dialog->plip_frame);
      gtk_widget_show (dialog->options_page);
      gtk_widget_show (dialog->isp_frame);
      gtk_widget_show (dialog->account_frame);

      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dialog->notebook), TRUE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (dialog->notebook), TRUE);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (dialog->notebook), 0);

      gtk_container_set_border_width (GTK_CONTAINER (dialog->general_page), 12);
      gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 6);

      if (OOBS_IS_IFACE_MODEM (iface))
	gtk_widget_show (dialog->modem_page);
      else
	gtk_widget_hide (dialog->modem_page);
    }
  else
    {
      gtk_widget_show (dialog->general_page);
      gtk_widget_hide (dialog->modem_page);
      gtk_widget_hide (dialog->options_page);
      gtk_widget_hide (dialog->isp_frame);
      gtk_widget_hide (dialog->account_frame);

      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dialog->notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (dialog->notebook), FALSE);

      gtk_container_set_border_width (GTK_CONTAINER (dialog->general_page), 0);
      gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 0);

      if (OOBS_IS_IFACE_ETHERNET (iface))
	{
	  gtk_widget_show (dialog->ethernet_frame);
	  gtk_widget_hide (dialog->plip_frame);

	  if (OOBS_IS_IFACE_WIRELESS (iface))
	    gtk_widget_show (dialog->wireless_frame);
	  else
	    gtk_widget_hide (dialog->wireless_frame);
	}
      else if (OOBS_IS_IFACE_PLIP (iface))
	{
	  gtk_widget_hide (dialog->wireless_frame);
	  gtk_widget_hide (dialog->ethernet_frame);
	  gtk_widget_show (dialog->plip_frame);
	}
    }
}

void
general_prepare (GstConnectionDialog *dialog,
		 OobsIface           *iface)
{
  NMState state;

  state = nm_integration_get_state (GST_NETWORK_TOOL (tool));

  if (nm_integration_iface_supported (iface) && state != NM_STATE_UNKNOWN)
    {
      gtk_widget_show (dialog->roaming_configured);
      gtk_widget_hide (dialog->connection_configured);
    }
  else
    {
      gtk_widget_hide (dialog->roaming_configured);
      gtk_widget_show (dialog->connection_configured);
    }
}

void
connection_dialog_prepare (GstConnectionDialog *dialog, OobsIface *iface)
{
  gboolean active;
  gchar *title;

  dialog->iface = g_object_ref (iface);
  active = oobs_iface_get_configured (dialog->iface);

  title = g_strdup_printf ("Settings for interface %s\n",
			   oobs_iface_get_device_name (dialog->iface));
  gtk_window_set_title (GTK_WINDOW (dialog->dialog), title);
  g_free (title);

  prepare_dialog_layout (dialog, iface);
  connection_dialog_set_sensitive (dialog, active);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->roaming_configured), !active);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->connection_configured), active);

  general_prepare (dialog, iface);

  if (OOBS_IS_IFACE_ISDN (iface) || OOBS_IS_IFACE_MODEM (iface))
    {
      isdn_dialog_prepare (dialog);

      if (OOBS_IS_IFACE_MODEM (iface))
	modem_dialog_prepare (dialog);
    }
  else
    {
      if (OOBS_IS_IFACE_WIRELESS (iface))
        {
	  wireless_dialog_prepare (dialog);
	  ethernet_dialog_prepare (dialog);
	}
      else if (OOBS_IS_IFACE_ETHERNET (iface)
	       || OOBS_IS_IFACE_IRLAN (iface))
	ethernet_dialog_prepare (dialog);
      else if (OOBS_IS_IFACE_PLIP (iface))
	plip_dialog_prepare (dialog);
    }

  dialog->changed = FALSE;
  g_object_unref (dialog->iface);
}

void
connection_save (GstConnectionDialog *dialog)
{
  gboolean active;

  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured));

  if (OOBS_IS_IFACE_MODEM (dialog->iface) ||
      OOBS_IS_IFACE_ISDN (dialog->iface))
    {
      if (OOBS_IS_IFACE_MODEM (dialog->iface))
	modem_dialog_save (dialog);

      isdn_dialog_save (dialog);
    }
  else if (OOBS_IS_IFACE_ETHERNET (dialog->iface))
    {
      if (OOBS_IS_IFACE_WIRELESS (dialog->iface))
	wireless_dialog_save (dialog);

      ethernet_dialog_save (dialog, active);
    }
  else if (OOBS_IS_IFACE_PLIP (dialog->iface))
    plip_dialog_save (dialog);

  oobs_iface_set_configured (dialog->iface, active);
}

void
connection_check_fields (GstConnectionDialog *dialog)
{
  gboolean active = FALSE;

  if (OOBS_IS_IFACE_ETHERNET (dialog->iface) ||
      OOBS_IS_IFACE_WIRELESS (dialog->iface) ||
      OOBS_IS_IFACE_IRLAN    (dialog->iface))
    active = ethernet_dialog_check_fields (dialog);
  else if (OOBS_IS_IFACE_PLIP (dialog->iface))
    active = plip_dialog_check_fields (dialog);
  else if (OOBS_IS_IFACE_MODEM (dialog->iface))
    active = (isdn_dialog_check_fields (dialog) &&
	      modem_dialog_check_fields (dialog));
  else
    active = isdn_dialog_check_fields (dialog);

  gtk_widget_set_sensitive (dialog->ok_button, active);
}

gchar*
connection_detect_modem (void)
{
  /* FIXME: Detect modem */
  return NULL;
}

void
connection_check_netmask (GtkWidget *address_widget, GtkWidget *netmask_widget)
{
  gchar *address, *netmask;
  guint32 ip1;

  address = (gchar *) gtk_entry_get_text (GTK_ENTRY (address_widget));
  netmask = (gchar *) gtk_entry_get_text (GTK_ENTRY (netmask_widget));

  if ((sscanf (address, "%d.", &ip1) == 1) && (strlen (netmask) == 0))
    {
      if (ip1 < 127)
        gtk_entry_set_text (GTK_ENTRY (netmask_widget), "255.0.0.0");
      else if (ip1 < 192)
        gtk_entry_set_text (GTK_ENTRY (netmask_widget), "255.255.0.0");
      else
        gtk_entry_set_text (GTK_ENTRY (netmask_widget), "255.255.255.0");
    }
}

void
connection_dialog_set_sensitive (GstConnectionDialog *dialog, gboolean active)
{
  gtk_widget_set_sensitive (dialog->wireless_frame, active);
  gtk_widget_set_sensitive (dialog->ethernet_frame, active);
  gtk_widget_set_sensitive (dialog->plip_frame,     active);
  gtk_widget_set_sensitive (dialog->modem_page,     active);
  gtk_widget_set_sensitive (dialog->options_page,   active);
  gtk_widget_set_sensitive (dialog->isp_frame,      active);
  gtk_widget_set_sensitive (dialog->account_frame,  active);
}

void
connection_dialog_hide (GstConnectionDialog *dialog)
{
  gtk_widget_hide (dialog->dialog);

#ifdef HAVE_LIBIW_H
  /* get rid of the essid list, if any */
  if (dialog->essid_list)
    {
      g_object_unref (dialog->essid_list);
      dialog->essid_list = NULL;
    }
#endif
}
