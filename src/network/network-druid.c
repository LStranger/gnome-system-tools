/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2003 Ximian, Inc.
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

#include <config.h>

#include <gnome.h>
#include <glade/glade.h>

#include "gst.h"
#include "network-druid.h"
#include "callbacks.h"
#include "connection.h"

static void
network_druid_fill_wireless_devices_list (GstTool *tool)
{
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog,
						  "network_connection_wireless_device");
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	xmlNodePtr node;
	GList *devices = NULL;
	
	for (node = gst_xml_element_find_first (root, "wireless_device");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "wireless_device"))
	{
		devices = g_list_append (devices, gst_xml_element_get_content (node));
	}

	if (devices)
		gtk_combo_set_popdown_strings (GTK_COMBO (combo), devices);
}

void
network_druid_new (GnomeDruid *druid, GstTool *tool, GstConnectionType type)
{
	NetworkDruidData *druid_data = g_new0 (NetworkDruidData, 1);

	druid_data->tool = tool;
	druid_data->current_page = NETWORK_DRUID_START;
	druid_data->cxn = connection_new_from_type (type, gst_xml_doc_get_root (tool->config));

	if (type == GST_CONNECTION_UNKNOWN)
		druid_data->fixed_type = FALSE;
	else
		druid_data->fixed_type = TRUE;

	g_object_set_data (G_OBJECT (druid), "data", druid_data);
}

void
network_druid_clear (GnomeDruid *druid, gboolean destroy_data)
{
	NetworkDruidData *druid_data = g_object_steal_data (G_OBJECT (druid), "data");
	GstTool *tool = druid_data->tool;
	GtkWidget *widget;
	gchar *entries [] = {
		"network_connection_wireless_device_entry",
		"network_connection_essid",
		"network_connection_other_ip_address",
		"network_connection_other_ip_mask",
		"network_connection_other_gateway",
		"network_connection_plip_local_ip",
		"network_connection_plip_remote_ip",
		"network_connection_ppp_phone",
		"network_connection_ppp_login",
		"network_connection_ppp_passwd1",
		"network_connection_ppp_passwd2",
		"network_connection_name",
		NULL
	};
	gint i;

	for (i = 0; entries[i] != NULL; i++) {
		widget = gst_dialog_get_widget (tool->main_dialog, entries[i]);
		gtk_entry_set_text (GTK_ENTRY (widget), "");
	}

	widget = gst_dialog_get_widget (tool->main_dialog, "network_connection_other_config_type");
	gtk_option_menu_set_history (GTK_OPTION_MENU (widget), IP_MANUAL);

	widget = gst_dialog_get_widget (tool->main_dialog, "network_connection_plip_gateway");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

	widget = gst_dialog_get_widget (tool->main_dialog, "connection_type_modem_option");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

	widget = gst_dialog_get_widget (tool->main_dialog, "network_connection_page1");
	gnome_druid_set_page (druid, GNOME_DRUID_PAGE (widget));

	if (destroy_data)
		connection_free (druid_data->cxn);
}

GstConnection*
network_druid_get_connection_data (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GstTool *tool = druid_data->tool;
	GstConnection *cxn = druid_data->cxn;
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	if (cxn->type == GST_CONNECTION_PPP) {
		GtkWidget *phone = gst_dialog_get_widget (tool->main_dialog, "network_connection_ppp_phone");
		GtkWidget *external_line = gst_dialog_get_widget (tool->main_dialog, "network_connection_external_line");
		GtkWidget *login = gst_dialog_get_widget (tool->main_dialog, "network_connection_ppp_login");
		GtkWidget *passwd = gst_dialog_get_widget (tool->main_dialog, "network_connection_ppp_passwd1");
		GtkWidget *name = gst_dialog_get_widget (tool->main_dialog, "network_connection_name");
		gchar *modem = connection_autodetect_modem ();

		if (strlen (modem) == 0)
			modem = g_strdup_printf ("/dev/modem");

		cxn->serial_port = modem;
		cxn->phone_number = g_strdup (gtk_entry_get_text (GTK_ENTRY (phone)));
		cxn->external_line = g_strdup (gtk_entry_get_text (GTK_ENTRY (external_line)));
		cxn->login = g_strdup (gtk_entry_get_text (GTK_ENTRY (login)));
		cxn->password = g_strdup (gtk_entry_get_text (GTK_ENTRY (passwd)));
		cxn->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (name)));
		cxn->dev = connection_find_new_device (root, cxn->type);
		
		cxn->wvsection = connection_wvsection_name_generate (cxn->dev, root);
		cxn->persist = TRUE;
		cxn->set_default_gw = TRUE;
		cxn->update_dns = TRUE;
		cxn->user = TRUE;
		cxn->autoboot = FALSE;

		return cxn;
	} else if (cxn->type == GST_CONNECTION_PLIP) {
		GtkWidget *local_ip = gst_dialog_get_widget (tool->main_dialog,
							     "network_connection_plip_local_ip");
		GtkWidget *remote_ip = gst_dialog_get_widget (tool->main_dialog,
							     "network_connection_plip_remote_ip");
		GtkWidget *gateway = gst_dialog_get_widget (tool->main_dialog,
							    "network_connection_plip_gateway");
		GtkWidget *name = gst_dialog_get_widget (tool->main_dialog,
							 "network_connection_name");

		cxn->address = g_strdup (gtk_entry_get_text (GTK_ENTRY (local_ip)));
		cxn->remote_address = g_strdup (gtk_entry_get_text (GTK_ENTRY (remote_ip)));
		cxn->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (name)));
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gateway)))
			cxn->gateway = g_strdup (cxn->remote_address);
		cxn->dev = connection_find_new_device (root, cxn->type);

		cxn->user = TRUE;
		cxn->autoboot = FALSE;

		return cxn;
	} else {
		GtkWidget *type = gst_dialog_get_widget (tool->main_dialog,
							 "network_connection_other_config_type");
		GtkWidget *ip_address = gst_dialog_get_widget (tool->main_dialog,
							       "network_connection_other_ip_address");
		GtkWidget *ip_mask = gst_dialog_get_widget (tool->main_dialog,
							    "network_connection_other_ip_mask");
		GtkWidget *gateway = gst_dialog_get_widget (tool->main_dialog,
							    "network_connection_other_gateway");
		GtkWidget *name = gst_dialog_get_widget (tool->main_dialog,
							 "network_connection_name");

		if (cxn->type == GST_CONNECTION_WLAN) {
			GtkWidget *device = gst_dialog_get_widget (tool->main_dialog,
								   "network_connection_wireless_device_entry");
			GtkWidget *essid = gst_dialog_get_widget (tool->main_dialog,
								  "network_connection_essid");
			cxn->dev = g_strdup (gtk_entry_get_text (GTK_ENTRY (device)));
			cxn->essid = g_strdup (gtk_entry_get_text (GTK_ENTRY (essid)));
		} else {
			cxn->dev = connection_find_new_device (root, cxn->type);
		}
			
		cxn->ip_config = gtk_option_menu_get_history (GTK_OPTION_MENU (type));
		cxn->address = g_strdup (gtk_entry_get_text (GTK_ENTRY (ip_address)));
		cxn->netmask = g_strdup (gtk_entry_get_text (GTK_ENTRY (ip_mask)));
		cxn->gateway = g_strdup (gtk_entry_get_text (GTK_ENTRY (gateway)));
		cxn->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (name)));
		connection_set_bcast_and_network (cxn);

		cxn->user = TRUE;
		cxn->autoboot = TRUE;

		return cxn;
	}
	
	return NULL;
}

static GstConnectionType
network_druid_selected_option (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GtkWidget *modem_option = gst_dialog_get_widget (druid_data->tool->main_dialog, "connection_type_modem_option");
	GtkWidget *ethernet_option = gst_dialog_get_widget (druid_data->tool->main_dialog, "connection_type_ethernet_option");
	GtkWidget *wvlan_option = gst_dialog_get_widget (druid_data->tool->main_dialog, "connection_type_wvlan_option");
	GtkWidget *plip_option = gst_dialog_get_widget (druid_data->tool->main_dialog, "connection_type_plip_option");
	GtkWidget *irlan_option = gst_dialog_get_widget (druid_data->tool->main_dialog, "connection_type_irlan_option");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (modem_option)))
		return GST_CONNECTION_PPP;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ethernet_option)))
		return GST_CONNECTION_ETH;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wvlan_option)))
		return GST_CONNECTION_WLAN;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plip_option)))
		return GST_CONNECTION_PLIP;
	else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (irlan_option)))
		return GST_CONNECTION_IRLAN;

	return GST_CONNECTION_UNKNOWN;
}

static gboolean
network_druid_check_phone (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GtkWidget *phone = gst_dialog_get_widget (druid_data->tool->main_dialog, "network_connection_ppp_phone");
	const gchar *text = gtk_entry_get_text (GTK_ENTRY (phone));
	gint i;

	if (strlen (text) == 0)
		return FALSE;

	for (i = 0; text[i] != (const gchar) NULL; i++) {
		if (!g_ascii_isdigit (text[i]))
			return FALSE;
	}

	return TRUE;
}

static gboolean
network_druid_check_login_password (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");

	gchar *login = (gchar*) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (druid_data->tool->main_dialog,
										      "network_connection_ppp_login")));
	gchar *passwd1 = (gchar*) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (druid_data->tool->main_dialog, "network_connection_ppp_passwd1")));
	gchar *passwd2 = (gchar*) gtk_entry_get_text (GTK_ENTRY (gst_dialog_get_widget (druid_data->tool->main_dialog, "network_connection_ppp_passwd2")));

	if ((strcmp (login, "") != 0) && (strcmp (passwd1, passwd2) == 0))
		return TRUE;
	else
		return FALSE;
}

static gboolean
network_druid_check_entry (GnomeDruid *druid, gchar *widget_name)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");

	GtkWidget *account = gst_dialog_get_widget (druid_data->tool->main_dialog,
						    widget_name);
	gchar *text = (gchar*) gtk_entry_get_text (GTK_ENTRY (account));

	return (strcmp (text, "") != 0);
}

static gboolean
network_druid_check_ip (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GstTool *tool = druid_data->tool;
	GtkWidget *option_menu = gst_dialog_get_widget (tool->main_dialog,
							"network_connection_other_config_type");
	GtkWidget *ip_address = gst_dialog_get_widget (tool->main_dialog,
						       "network_connection_other_ip_address");
	GtkWidget *ip_mask = gst_dialog_get_widget (tool->main_dialog,
						    "network_connection_other_ip_mask");
	const gchar *address, *mask;

	if (gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu)) == IP_MANUAL) {
		address = gtk_entry_get_text (GTK_ENTRY (ip_address));
		mask = gtk_entry_get_text (GTK_ENTRY (ip_mask));
		return ((strlen (address) != 0) && (strlen (mask) != 0));
	} else
		return TRUE;
}

static gboolean
network_druid_check_plip (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GstTool *tool = druid_data->tool;
	GtkWidget *local_ip = gst_dialog_get_widget (tool->main_dialog,
						     "network_connection_plip_local_ip");
	GtkWidget *remote_ip = gst_dialog_get_widget (tool->main_dialog,
						      "network_connection_plip_remote_ip");
	const gchar *local, *remote;

	local = gtk_entry_get_text (GTK_ENTRY (local_ip));
	remote = gtk_entry_get_text (GTK_ENTRY (remote_ip));

	return ((strlen (local) != 0) && (strlen (remote) != 0));
}

void
network_druid_check_page (GnomeDruid *druid, NetworkDruidPages page_number)
{
	gboolean cont;

	switch (page_number) {
	case NETWORK_DRUID_WIRELESS:
		cont = network_druid_check_entry (druid, "network_connection_wireless_device_entry") &&
			network_druid_check_entry (druid, "network_connection_essid");
		break;
	case NETWORK_DRUID_OTHER_1:
		cont = network_druid_check_ip (druid);
		break;
	case NETWORK_DRUID_PLIP_1:
		cont = network_druid_check_plip (druid);
		break;
	case NETWORK_DRUID_PPP_1:
		cont = network_druid_check_phone (druid);
		break;
	case NETWORK_DRUID_PPP_2:
		cont = network_druid_check_login_password (druid);
		break;
	case NETWORK_DRUID_NAME:
		cont = network_druid_check_entry (druid, "network_connection_name");
		break;
	}

	gnome_druid_set_buttons_sensitive (druid, TRUE, cont, TRUE, FALSE);
}

gboolean
network_druid_set_page_next (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GstTool *tool = druid_data->tool;
	GnomeDruidPage *next_page;
	GtkWidget *selected_option;
	gchar *image_path;
	GdkPixbuf *image;

	g_return_val_if_fail (druid_data != NULL, FALSE);

	switch (druid_data->current_page) {
	case NETWORK_DRUID_START:
	case NETWORK_DRUID_CONNECTION_TYPE:
		if (((druid_data->current_page == NETWORK_DRUID_START) && (druid_data->fixed_type)) ||
		    (druid_data->current_page == NETWORK_DRUID_CONNECTION_TYPE)) {
			if (!druid_data->fixed_type)
				druid_data->cxn->type = network_druid_selected_option (druid);
		
			if (druid_data->cxn->type == GST_CONNECTION_PPP) {
				druid_data->current_page = NETWORK_DRUID_PPP_1;
				next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
										     "network_connection_ppp_page1"));

				gnome_druid_set_page (druid, next_page);
			
				return TRUE;
			} else if (druid_data->cxn->type == GST_CONNECTION_PLIP) {
				druid_data->current_page = NETWORK_DRUID_PLIP_1;
				next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
										     "network_connection_plip_page1"));
				gnome_druid_set_page (druid, next_page);
			
				return TRUE;
			} else if (druid_data->cxn->type == GST_CONNECTION_WLAN) {
				network_druid_fill_wireless_devices_list (druid_data->tool);
				
				druid_data->current_page = NETWORK_DRUID_WIRELESS;

				next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
										     "network_connection_wireless_page"));
				gnome_druid_set_page (druid, next_page);

				return TRUE;
			} else {
				druid_data->current_page = NETWORK_DRUID_OTHER_1;
				next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
										     "network_connection_other_page1"));
				
				switch (druid_data->cxn->type) {
				case GST_CONNECTION_ETH:
					image_path = g_strdup_printf (PIXMAPS_DIR "/connection-ethernet.png");
					break;
				case GST_CONNECTION_IRLAN:
					image_path = g_strdup_printf (PIXMAPS_DIR "/irda-48.png");
					break;
				}

				image = gdk_pixbuf_new_from_file (image_path, NULL);
				gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD (next_page), image);
				g_free (image_path);
				gdk_pixbuf_unref (image);

				gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD (next_page),
								     _("Address configuration (Step 1 of 2)"));
				gnome_druid_set_page (druid, next_page);
			
				return TRUE;
			}
		}

		break;
	case NETWORK_DRUID_WIRELESS:
		druid_data->current_page = NETWORK_DRUID_OTHER_1;
		next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
								     "network_connection_other_page1"));
		gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD (next_page),
						     _("Address configuration (Step 2 of 3)"));

		/* set the wireless image to the druid */
		image_path = g_strdup_printf (PIXMAPS_DIR "/wavelan-48.png");
		image = gdk_pixbuf_new_from_file (image_path, NULL);
		gnome_druid_page_standard_set_logo (GNOME_DRUID_PAGE_STANDARD (next_page), image);
				
		g_free (image_path);
		gdk_pixbuf_unref (image);

		gnome_druid_set_page (druid, next_page);

		return TRUE;
		break;
	case NETWORK_DRUID_OTHER_1:
	case NETWORK_DRUID_PLIP_1:
		druid_data->current_page = NETWORK_DRUID_NAME;
		next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
								     "network_connection_page_name"));
		if (druid_data->cxn->type != GST_CONNECTION_WLAN)
			gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD (next_page),
							     _("Connection name (Step 2 of 2)"));
		else
			gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD (next_page),
							     _("Connection name (Step 3 of 3)"));

		gnome_druid_set_page (druid, next_page);
		
		return TRUE;
		break;
	case NETWORK_DRUID_PPP_2:
		druid_data->current_page = NETWORK_DRUID_NAME;
		next_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
								     "network_connection_page_name"));
		gnome_druid_page_standard_set_title (GNOME_DRUID_PAGE_STANDARD (next_page),
						     _("Connection name (Step 3 of 3)"));

		return FALSE;
		break;
	}

	druid_data->current_page++;

	return FALSE;
}

gboolean
network_druid_set_page_back (GnomeDruid *druid)
{
	NetworkDruidData *druid_data = g_object_get_data (G_OBJECT (druid), "data");
	GnomeDruidPage *back_page;

	g_return_val_if_fail (druid_data != NULL, FALSE);

	if ((druid_data->current_page == NETWORK_DRUID_OTHER_1) &&
	    (druid_data->cxn->type == GST_CONNECTION_WLAN)) {
		druid_data->current_page = NETWORK_DRUID_WIRELESS;
		back_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
								     "network_connection_wireless_page"));
		gnome_druid_set_page (druid, back_page);
		
		return TRUE;
	} else if ((druid_data->current_page == NETWORK_DRUID_PPP_1) ||
		   (druid_data->current_page == NETWORK_DRUID_PLIP_1) ||
		   (druid_data->current_page == NETWORK_DRUID_WIRELESS) ||
		   ((druid_data->current_page == NETWORK_DRUID_OTHER_1) &&
		    (druid_data->cxn->type != GST_CONNECTION_WLAN))) {
		if (druid_data->fixed_type) {
			druid_data->current_page = NETWORK_DRUID_START;
		
			back_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
									     "network_connection_page1"));
		} else {
			druid_data->current_page = NETWORK_DRUID_CONNECTION_TYPE;
		
			back_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
									     "network_connection_page2"));
		}

		gnome_druid_set_page (druid, back_page);

		return TRUE;
	} else if ((druid_data->current_page == NETWORK_DRUID_NAME) &&
		   (druid_data->cxn->type == GST_CONNECTION_PLIP)) {
		druid_data->current_page = NETWORK_DRUID_PLIP_1;
		back_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
								     "network_connection_plip_page1"));
		gnome_druid_set_page (druid, back_page);
		
		return TRUE;
	} else if ((druid_data->current_page == NETWORK_DRUID_NAME) &&
		   (druid_data->cxn->type != GST_CONNECTION_PPP)) {
		druid_data->current_page = NETWORK_DRUID_OTHER_1;
		back_page = GNOME_DRUID_PAGE (gst_dialog_get_widget (druid_data->tool->main_dialog,
								     "network_connection_other_page1"));

		gnome_druid_set_page (druid, back_page);

		return TRUE;
	}

	druid_data->current_page--;

	return FALSE;
}
