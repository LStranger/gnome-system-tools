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

#include "gst.h"
#include "gst-network-tool.h"
#include "connection.h"

extern GstTool *tool;

/* helper for getting whether the string or null if it's empty */
static gchar*
get_entry_text (GtkWidget *entry)
{
  gchar *str;

  str = (gchar *) gtk_entry_get_text (GTK_ENTRY (entry));
  return (!str || !*str) ? NULL : str;
}

static void
connection_set_bootproto (GstConnectionDialog *dialog, GstBootProto bootproto)
{
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->bootproto_combo),
			    (bootproto == GST_BOOTPROTO_DHCP) ? 1 : 0);
}

static GstBootProto
connection_get_bootproto (GstConnectionDialog *dialog)
{
  gint ret;

  ret = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->bootproto_combo));
  return (ret == 0) ? GST_BOOTPROTO_STATIC : GST_BOOTPROTO_DHCP;
}

static void
connection_bootproto_init (GstConnectionDialog *dialog)
{
  gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->bootproto_combo), _("Static IP address"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (dialog->bootproto_combo), _("DHCP"));
}

static void
ethernet_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar        *address, *netmask, *gateway;
  GstBootProto  bootproto;
  gboolean      enabled;

  g_object_get (G_OBJECT (dialog->iface),
		"iface-address",   &address,
		"iface-netmask",   &netmask,
		"iface-gateway",   &gateway,
		"iface-bootproto", &bootproto,
		NULL);

  connection_set_bootproto (dialog, bootproto);
  gtk_entry_set_text (GTK_ENTRY (dialog->address), (address) ? address : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->netmask), (netmask) ? netmask : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->gateway), (gateway) ? gateway : "");

  g_free (address);
  g_free (netmask);
  g_free (gateway);
}

static void
ethernet_dialog_save (GstConnectionDialog *dialog)
{
  g_object_set (G_OBJECT (dialog->iface),
		"iface-address",   get_entry_text (dialog->address),
		"iface-netmask",   get_entry_text (dialog->netmask),
		"iface-gateway",   get_entry_text (dialog->gateway),
		"iface-bootproto", connection_get_bootproto (dialog),
		NULL);
}

static gboolean
ethernet_dialog_check_fields (GstConnectionDialog *dialog)
{
  gchar *address, *netmask, *gateway;

  address = get_entry_text (dialog->address);
  netmask = get_entry_text (dialog->netmask);
  gateway = get_entry_text (dialog->gateway);

  return ((!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured))) ||
	  (connection_get_bootproto (dialog) == GST_BOOTPROTO_DHCP) ||
	  ((gst_filter_check_ip_address (address) == GST_ADDRESS_IPV4) &&
	   (gst_filter_check_ip_address (netmask) == GST_ADDRESS_IPV4) &&
	   (!gateway || gst_filter_check_ip_address (gateway) == GST_ADDRESS_IPV4)));
}

static GtkTreeModel*
wireless_essid_get_model (const gchar *dev)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  xmlNodePtr    root, node;
  xmlDoc       *doc;
  GdkPixbuf    *locked, *unlocked, *pix;
  gchar        *essid;
  gboolean      encrypted;

  model = GTK_TREE_MODEL (gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING));
  doc   = gst_tool_run_get_directive (tool, NULL, "detect_essids", dev, NULL);
  locked = gtk_icon_theme_load_icon (GST_NETWORK_TOOL (tool)->icon_theme,
				     "gnome-dev-wavelan-encrypted", 16, 0, NULL);
  unlocked = gtk_icon_theme_load_icon (GST_NETWORK_TOOL (tool)->icon_theme,
				       "gnome-dev-wavelan", 16, 0, NULL);

  g_return_if_fail (doc != NULL);
  root  = gst_xml_doc_get_root (doc);

  for (node = gst_xml_element_find_first (root, "network");
       node; node = gst_xml_element_find_next (node, "network"))
    {
      essid = gst_xml_get_child_content (node, "essid");
      encrypted = gst_xml_element_get_boolean (node, "encrypted");

      pix = (encrypted) ? locked : unlocked;

      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			  0, pix, 1, essid, -1);

      g_free (essid);
    }

  gdk_pixbuf_unref (locked);
  gdk_pixbuf_unref (unlocked);

  return model;
}

static void
wireless_dialog_prepare_essid_entry (GtkComboBoxEntry *combo, const gchar *dev)
{
  GtkTreeModel *model;
  GtkEntryCompletion *completion;
  GtkCellRenderer *renderer;

  model = wireless_essid_get_model (dev);
  g_return_if_fail (model != NULL);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
  g_object_unref (model);

  gtk_combo_box_entry_set_text_column (combo, 1);

  /* Add the pixbuf cell renderer */
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo),
			      renderer, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo),
				 renderer, "pixbuf", 0);
}

static void
wireless_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar *essid, *key, *dev;

  g_object_get (G_OBJECT (dialog->iface),
		"iface-essid", &essid,
		"iface-wep-key",   &key,
		"iface-dev", &dev,
		NULL);

  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dialog->essid)->child), (essid) ? essid : "");
  gtk_entry_set_text (GTK_ENTRY (dialog->wep_key), (key) ? key : "");

  wireless_dialog_prepare_essid_entry (GTK_COMBO_BOX_ENTRY (dialog->essid), dev);

  g_free (essid);
  g_free (key);
  g_free (dev);
}

static void
wireless_dialog_save (GstConnectionDialog *dialog)
{
  g_object_set (G_OBJECT (dialog->iface),
		"iface-essid",   get_entry_text (GTK_BIN (dialog->essid)->child),
		"iface-wep-key", get_entry_text (dialog->wep_key),
		NULL);
}

static gboolean
wireless_dialog_check_fields (GstConnectionDialog *dialog)
{
  return ((!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured))) ||
	  (get_entry_text (GTK_BIN (dialog->essid)->child)));
}

static void
plip_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar *local_address, *remote_address;

  g_object_get (G_OBJECT (dialog->iface),
		"iface-local-address",  &local_address,
		"iface-remote-address", &remote_address,
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
		"iface-local-address",  get_entry_text (dialog->local_address),
		"iface-remote-address", get_entry_text (dialog->remote_address),
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
modem_dialog_prepare (GstConnectionDialog *dialog)
{
  gchar    *login, *password, *serial_port, *phone_number, *dial_prefix;
  gint      volume, dial_type;
  gboolean  default_gw, persist;

  g_object_get (G_OBJECT (dialog->iface),
                "iface-login", &login,
                "iface-password", &password,
                "iface-serial-port", &serial_port,
                "iface-phone-number", &phone_number,
                "iface-dial-prefix", &dial_prefix,
                "iface-volume", &volume,
                "iface-dial-type", &dial_type,
                "iface-default-gw", &default_gw,
                "iface-persist", &persist,
                NULL);

  gtk_entry_set_text (GTK_ENTRY (dialog->login),
                      (login) ? login : "");

  gtk_entry_set_text (GTK_ENTRY (dialog->password),
                      (password) ? password : "");

  gtk_entry_set_text (GTK_ENTRY (GTK_BIN (dialog->serial_port)->child),
                      (serial_port) ? serial_port : "");

  gtk_entry_set_text (GTK_ENTRY (dialog->phone_number),
                      (phone_number) ? phone_number : "");

  gtk_entry_set_text (GTK_ENTRY (dialog->dial_prefix),
                      (dial_prefix) ? dial_prefix : "");

  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->volume), volume);
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->dial_type), dial_type);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->default_gw), default_gw);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->persist), persist);

  g_free (login);
  g_free (password);
  g_free (serial_port);
  g_free (phone_number);
  g_free (dial_prefix);
}

static void
modem_dialog_save (GstConnectionDialog *dialog)
{
  g_object_set (G_OBJECT (dialog->iface),
                "iface-login",        get_entry_text (dialog->login),
                "iface-password",     get_entry_text (dialog->password),
                "iface-serial-port",  get_entry_text (GTK_BIN (dialog->serial_port)->child),
		"iface-phone-number", get_entry_text (dialog->phone_number),
                "iface-dial-prefix",  get_entry_text (dialog->dial_prefix),
                "iface-volume",       gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->volume)),
                "iface-dial-type",    gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->dial_type)),
                "iface-default-gw",   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->default_gw)),
                "iface-persist",      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->persist)),
		NULL);
}

static gboolean
modem_dialog_check_fields (GstConnectionDialog *dialog)
{
  return ((!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_ppp_configured))) ||
	  ((get_entry_text (dialog->login)) &&
	   (get_entry_text (dialog->password)) &&
	   (get_entry_text (GTK_BIN (dialog->serial_port)->child)) &&
	   (get_entry_text (dialog->phone_number))));
}

GstConnectionDialog*
connection_dialog_init (void)
{
  GstConnectionDialog *gcd;

  gcd = g_new0 (GstConnectionDialog, 1);

  gcd->standalone = FALSE;
  gcd->iface  = NULL;
  gcd->dialog = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");

  gcd->ok_button = gst_dialog_get_widget (tool->main_dialog, "connection_ok");

  gcd->notebook         = gst_dialog_get_widget (tool->main_dialog, "connection_notebook");
  gcd->general_page     = gst_dialog_get_widget (tool->main_dialog, "connection_general_page");
  gcd->ppp_general_page = gst_dialog_get_widget (tool->main_dialog, "connection_ppp_general_page");
  gcd->account_page     = gst_dialog_get_widget (tool->main_dialog, "connection_account_page");
  gcd->options_page     = gst_dialog_get_widget (tool->main_dialog, "connection_options_page");

  gcd->connection_configured = gst_dialog_get_widget (tool->main_dialog, "connection_device_active");
  gcd->connection_device     = gst_dialog_get_widget (tool->main_dialog, "connection_device");

  gcd->connection_ppp_configured = gst_dialog_get_widget (tool->main_dialog, "connection_ppp_device_active");
  gcd->connection_ppp_device     = gst_dialog_get_widget (tool->main_dialog, "connection_ppp_device");

  /* ethernet */
  gcd->bootproto_combo = gst_dialog_get_widget (tool->main_dialog, "connection_bootproto");
  gcd->address = gst_dialog_get_widget (tool->main_dialog, "connection_address");
  gcd->netmask = gst_dialog_get_widget (tool->main_dialog, "connection_netmask");
  gcd->gateway = gst_dialog_get_widget (tool->main_dialog, "connection_gateway");

  /* wireless */
  gcd->essid   = gst_dialog_get_widget (tool->main_dialog, "connection_essid");
  gcd->wep_key = gst_dialog_get_widget (tool->main_dialog, "connection_wep_key");

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
  gcd->persist      = gst_dialog_get_widget (tool->main_dialog, "connection_persist");

  gcd->wireless_frame = gst_dialog_get_widget (tool->main_dialog, "connection_wireless");
  gcd->ethernet_frame = gst_dialog_get_widget (tool->main_dialog, "connection_ethernet");
  gcd->plip_frame     = gst_dialog_get_widget (tool->main_dialog, "connection_plip");
  gcd->modem_frame    = gst_dialog_get_widget (tool->main_dialog, "connection_modem");

  connection_bootproto_init (gcd);

  return gcd;
}

void
connection_dialog_prepare (GstConnectionDialog *dialog, GstIface *iface)
{
  dialog->iface = g_object_ref (iface);
  
  if (GST_IS_IFACE_MODEM (iface))
    {
      gtk_widget_hide (dialog->general_page);
      gtk_widget_show (dialog->ppp_general_page);
      gtk_widget_show (dialog->account_page);
      gtk_widget_show (dialog->options_page);

      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dialog->notebook), TRUE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (dialog->notebook), TRUE);

      gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 6);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->connection_ppp_configured),
				    gst_iface_is_configured (iface));

      gtk_label_set_text (GTK_LABEL (dialog->connection_ppp_device),
			  gst_iface_get_dev (iface));

      modem_dialog_prepare (dialog);
    }
  else
    {
      gtk_widget_show (dialog->general_page);
      gtk_widget_hide (dialog->ppp_general_page);
      gtk_widget_hide (dialog->account_page);
      gtk_widget_hide (dialog->options_page);

      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (dialog->notebook), FALSE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (dialog->notebook), FALSE);

      gtk_container_set_border_width (GTK_CONTAINER (dialog->general_page), 0);
      gtk_container_set_border_width (GTK_CONTAINER (dialog->notebook), 0);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->connection_configured),
				    gst_iface_is_configured (iface));

      gtk_label_set_text (GTK_LABEL (dialog->connection_device),
			  gst_iface_get_dev (iface));

      if (GST_IS_IFACE_WIRELESS (iface))
        {
	  gtk_widget_show (dialog->wireless_frame);
	  gtk_widget_show (dialog->ethernet_frame);
	  gtk_widget_hide (dialog->plip_frame);

	  wireless_dialog_prepare (dialog);
	  ethernet_dialog_prepare (dialog);
	}
      else if (GST_IS_IFACE_ETHERNET (iface)
	       || GST_IS_IFACE_IRLAN (iface))
        {
	  gtk_widget_hide (dialog->wireless_frame);
	  gtk_widget_show (dialog->ethernet_frame);
	  gtk_widget_hide (dialog->plip_frame);

	  ethernet_dialog_prepare (dialog);
	}
      else if (GST_IS_IFACE_PLIP (iface))
        {
	  gtk_widget_hide (dialog->wireless_frame);
	  gtk_widget_hide (dialog->ethernet_frame);
	  gtk_widget_show (dialog->plip_frame);

	  plip_dialog_prepare (dialog);
	}
    }

  dialog->changed = FALSE;
}

void
connection_save (GstConnectionDialog *dialog)
{
  gboolean active;

  if (GST_IS_IFACE_MODEM (dialog->iface))
    {
      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_ppp_configured));
      modem_dialog_save (dialog);
    }
  else
    {
      active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->connection_configured));

      if (GST_IS_IFACE_WIRELESS (dialog->iface))
        {
          wireless_dialog_save (dialog);
          ethernet_dialog_save (dialog);
        }
      else if (GST_IS_IFACE_ETHERNET (dialog->iface)
               || GST_IS_IFACE_IRLAN (dialog->iface))
        ethernet_dialog_save (dialog);
      else if (GST_IS_IFACE_PLIP (dialog->iface))
        plip_dialog_save (dialog);
    }

  gst_iface_set_configured (dialog->iface, active);
}

void
connection_check_fields (GstConnectionDialog *dialog)
{
  gboolean active;

  if (GST_IS_IFACE_WIRELESS (dialog->iface))
    active = (wireless_dialog_check_fields (dialog) &&
	      ethernet_dialog_check_fields (dialog));
  else if (GST_IS_IFACE_ETHERNET (dialog->iface) ||
	   GST_IS_IFACE_IRLAN    (dialog->iface))
    active = ethernet_dialog_check_fields (dialog);
  else if (GST_IS_IFACE_PLIP (dialog->iface))
    active = plip_dialog_check_fields (dialog);
  else
    active = modem_dialog_check_fields (dialog);

  gtk_widget_set_sensitive (dialog->ok_button, active);
}

gchar*
connection_detect_modem (void)
{
  xmlNodePtr  root;
  xmlDoc     *doc;
  gchar      *device;

  doc= gst_tool_run_get_directive (tool, NULL, "detect_modem", NULL);
  g_return_val_if_fail (doc != NULL, NULL);

  root   = gst_xml_doc_get_root (doc);
  device = gst_xml_get_child_content (root, "device");
  xmlFreeDoc (doc);
	
  return device;
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
