/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Hans Petter Jansson <hpj@ximian.com>.
 *          Arturo Espinosa <arturo@ximian.com>
 */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>

#include "gst.h"

#include "transfer.h"
#include "connection.h"
#include "callbacks.h"
#include "hosts.h"
#include "network-druid.h"

GstTool *tool;

GstDialogSignal signals[] = {
	{ "network_admin_notebook",  "switch_page",     G_CALLBACK (on_network_notebook_switch_page) },
	{ "hostname",                "changed",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "samba_use",               "toggled",         G_CALLBACK (on_samba_use_toggled) },
	{ "smbdesc",                 "changed",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "workgroup",               "changed",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "winsserver",              "changed",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "wins_use",                "toggled",         G_CALLBACK (on_wins_use_toggled) },
	{ "connection_add",          "clicked",         G_CALLBACK (on_connection_add_clicked) },
	{ "connection_delete",       "clicked",         G_CALLBACK (on_connection_delete_clicked) },
	{ "connection_configure",    "clicked",         G_CALLBACK (on_connection_configure_clicked) },
	{ "connection_activate",     "clicked",         G_CALLBACK (on_connection_activate_button_clicked) },
	{ "connection_deactivate",   "clicked",         G_CALLBACK (on_connection_deactivate_button_clicked) },
	{ "connection_def_gw_omenu", "changed",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "domain",                  "changed",         G_CALLBACK (gst_dialog_modify_cb) },
/*	{ "ip",                      "changed",         G_CALLBACK (gst_dialog_modify_cb) },*/
	{ "ip",                      "changed",         G_CALLBACK (on_hosts_ip_changed) },
	{ "statichost_add",          "clicked",         G_CALLBACK (on_hosts_add_clicked) },
	{ "statichost_add",          "clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "statichost_delete",       "clicked",         G_CALLBACK (on_hosts_delete_clicked) },
	{ "statichost_delete",       "clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "dns_server_add_button",   "clicked",         G_CALLBACK (on_dns_search_add_button_clicked) },
	{ "dns_server_add_button",   "clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "search_domain_add_button","clicked",         G_CALLBACK (on_dns_search_add_button_clicked) },
	{ "search_domain_add_button","clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "dns_server_del_button",   "clicked",         G_CALLBACK (on_dns_search_del_button_clicked) },
	{ "dns_server_del_button",   "clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "search_domain_del_button","clicked",         G_CALLBACK (on_dns_search_del_button_clicked) },
	{ "search_domain_del_button","clicked",         G_CALLBACK (gst_dialog_modify_cb) },

/*	{ "dns_list",                "focus_in_event",  G_CALLBACK (update_hint) },*/
/*	{ "search_list",             "focus_in_event",  G_CALLBACK (update_hint) },*/
/*      { "ip",                      "focus_in_event",  G_CALLBACK (update_hint) },*/
/*	{ "alias",                   "focus_in_event",  G_CALLBACK (update_hint) },*/

	/* Network druid callbacks */
	{ "network_connection_window",       "delete_event",  G_CALLBACK (on_network_druid_hide) },
	{ "network_connection_druid",        "cancel", G_CALLBACK (on_network_druid_hide) },
	{ "network_connection_page1",        "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_page2",        "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_wireless_page",  "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_other_page1",  "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_plip_page1",   "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_ppp_page1",    "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_ppp_page2",    "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_page_activate", "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_page2",        "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_wireless_page",  "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_other_page1",  "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_plip_page1",  "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_ppp_page1",    "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_ppp_page2",    "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_page_activate", "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_page_finish",  "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_wireless_device", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_essid", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_other_config_type", "changed", G_CALLBACK (on_network_druid_config_type_changed) },
	{ "network_connection_other_ip_address", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_other_ip_mask", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_other_gateway", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_plip_local_ip", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_plip_remote_ip", "changed", G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_ppp_phone",    "changed",   G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_ppp_login",    "changed",   G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_ppp_passwd1",  "changed",   G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_ppp_passwd2",  "changed",   G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_page_finish",  "finish",    G_CALLBACK (on_network_druid_finish) },
	{ "network_connection_other_ip_address", "focus_out_event", G_CALLBACK (on_network_druid_ip_address_focus_out) },

	/* Network profiles callbacks */
	{ "network_profiles_menu", "changed", G_CALLBACK (on_network_profile_option_selected) },
	{ "network_profiles_button", "clicked", G_CALLBACK (on_network_profiles_button_clicked) },
	{ "network_profile_new", "clicked", G_CALLBACK (on_network_profile_new_clicked) },
	{ "network_profile_delete", "clicked", G_CALLBACK (on_network_profile_delete_clicked) },

	/* Interface properties callbacks */
	{ "connection_config_dialog", "delete_event", G_CALLBACK (on_connection_delete_event) },
	{ "connection_cancel", "clicked", G_CALLBACK (on_connection_cancel_clicked) },
	{ "connection_ok", "clicked", G_CALLBACK (on_connection_ok_clicked) },
	{ "ppp_autodetect_modem", "clicked", G_CALLBACK (on_ppp_autodetect_modem_clicked) },
	{ "ppp_volume", "format_value", G_CALLBACK (on_volume_format_value) },
	{ "ip_address", "focus_out_event", G_CALLBACK (on_ip_address_focus_out) },
	{ "ip_netmask", "focus_out_event", G_CALLBACK (on_ip_address_focus_out) },
	{ "ppp_update_dns", "toggled", G_CALLBACK (on_ppp_update_dns_toggled) },
	{ "status_autoboot", "toggled", G_CALLBACK (on_connection_modified) },
	{ "status_user", "toggled", G_CALLBACK (on_connection_modified) },
	{ "wlan_essid", "changed", G_CALLBACK (on_connection_modified) },
	{ "connection_config", "changed", G_CALLBACK (on_connection_ip_config_changed) },
	{ "connection_config", "changed", G_CALLBACK (on_connection_modified) },
	{ "ip_address", "changed", G_CALLBACK (on_connection_modified) },
	{ "ip_netmask", "changed", G_CALLBACK (on_connection_modified) },
	{ "ip_gateway", "changed", G_CALLBACK (on_connection_modified) },
	{ "ip_update_dns", "toggled", G_CALLBACK (on_connection_modified) },
	{ "ptp_address", "changed", G_CALLBACK (on_connection_modified) },
	{ "ptp_remote_address", "changed", G_CALLBACK (on_connection_modified) },
	{ "ptp_remote_is_gateway", "toggled", G_CALLBACK (on_connection_modified) },
	{ "ppp_serial_port", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_dial_command", "clicked", G_CALLBACK (on_connection_modified) },
	{ "ppp_volume", "value_changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_persist", "toggled", G_CALLBACK (on_connection_modified) },
	{ "ppp_phone_number", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_external_line", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_login", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_password", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_ppp_options", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_stupid", "toggled", G_CALLBACK (on_connection_modified) },
	{ "ppp_set_default_gw", "toggled", G_CALLBACK (on_connection_modified) },
	{ "ppp_dns1", "changed", G_CALLBACK (on_connection_modified) },
	{ "ppp_dns2", "changed", G_CALLBACK (on_connection_modified) },
	{ NULL }
};

GstDialogSignal signals_after[] = {
	{ "network_connection_page2",         "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_wireless_page", "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_other_page1",   "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_plip_page1",    "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_ppp_page1",     "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_ppp_page2",     "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_page_activate", "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_page_finish",   "prepare", G_CALLBACK (on_network_druid_page_prepare) },
	{ NULL }
};

static GstReportHookEntry report_hooks[] = {
	{ "file_locate_tool_failed", callbacks_tool_not_found_hook,  GST_REPORT_HOOK_LOAD, TRUE,  NULL },
	{ NULL,                      NULL,                           -1,                   FALSE, NULL }
};

static void
connect_signals (GstDialog *main_dialog, GstDialogSignal *sigs, GstDialogSignal *sigs_after)
{
	GtkWidget *widget = gst_dialog_get_widget (main_dialog, "network_connection_wireless_device");
	
	gst_dialog_connect_signals (main_dialog, sigs);
	gst_dialog_connect_signals_after (main_dialog, sigs_after);

	g_signal_connect (GTK_BIN (GTK_COMBO_BOX (widget))->child,
			  "changed", G_CALLBACK (on_network_druid_entry_changed), NULL);
}

int
main (int argc, char *argv[])
{
	gboolean connection = FALSE;
	gboolean ppp_connection = FALSE;
	gboolean eth_connection = FALSE;
	gboolean wlan_connection = FALSE;
	gboolean plip_connection = FALSE;
	gboolean irlan_connection = FALSE;
	gchar *interface = NULL;
	
	struct poptOption options[] =
	{
		{ "new-connection", '\0', 0, &connection, 0, _("Run the connection assistant."), NULL },
		{ "ppp-connection", '\0', 0, &ppp_connection, 0, _("Run the PPP connection assistant."), NULL },
		{ "eth-connection", '\0', 0, &eth_connection, 0, _("Run the ethernet connection assistant."), NULL },
		{ "wlan-connection", '\0', 0, &wlan_connection, 0, _("Run the wlan connection assistant."), NULL },
		{ "plip-connection", '\0', 0, &plip_connection, 0, _("Run the parallel port connection assistant."), NULL },
		{ "irlan-connection", '\0', 0, &irlan_connection, 0, _("Run the irlan connection assistant."), NULL },
		{ "configure", '\0', POPT_ARG_STRING, &interface, 0, _("Configure a network interface"), _("interface") },
		
		{NULL, '\0', 0, NULL, 0}
	};

	gst_init ("network-admin", argc, argv, options);
	tool = gst_tool_new ();
	gst_tool_construct (tool , "network", _("Network Settings"));

	if (connection || ppp_connection || eth_connection || wlan_connection || plip_connection || irlan_connection) {
		GtkWidget *add_button = gst_dialog_get_widget (tool->main_dialog, "connection_add");
		GtkWidget *druid_window = gst_dialog_get_widget (tool->main_dialog, "network_connection_window");
		GnomeDruid *druid = GNOME_DRUID (gst_dialog_get_widget (tool->main_dialog, "network_connection_druid"));
		GstConnectionType type;

		if (connection)
			type = GST_CONNECTION_UNKNOWN;
		else if (ppp_connection)
			type = GST_CONNECTION_PPP;
		else if (eth_connection)
			type = GST_CONNECTION_ETH;
		else if (wlan_connection)
			type = GST_CONNECTION_WLAN;
		else if (plip_connection)
			type = GST_CONNECTION_PLIP;
		else if (irlan_connection)
			type = GST_CONNECTION_IRLAN;

		g_object_set_data (G_OBJECT (druid), "standalone", GINT_TO_POINTER (TRUE));

		connect_signals (tool->main_dialog, signals, signals_after);
		init_editable_filters (tool->main_dialog);
		network_druid_new (druid, druid_window, tool, type);
		gtk_widget_show_all (druid_window);

		gst_tool_main_with_hidden_dialog (tool, FALSE);
	} else if (interface != NULL) {
		GtkWidget *window = gst_dialog_get_widget (tool->main_dialog, "connection_config_dialog");
		GstConnection *connection;
		xmlNodePtr root;

		connect_signals (tool->main_dialog, signals, signals_after);
		init_editable_filters (tool->main_dialog);
		on_network_admin_show (NULL, tool);

		gst_tool_main_with_hidden_dialog (tool, TRUE);

		g_object_set_data (G_OBJECT (window), "standalone", GINT_TO_POINTER (TRUE));
		root = gst_xml_doc_get_root (tool->config);
		transfer_misc_xml_to_tool (tool, root);
		connection_configure_device (root, interface);

		gtk_main ();
	} else {
		connect_signals (tool->main_dialog, signals, signals_after);
		
		gst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_hostname_hook,     NULL);
		/*gst_dialog_add_apply_hook (tool->main_dialog, callbacks_update_connections_hook, NULL);*/
		gst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_dialer_hook,       tool);
		gst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_gateway_hook,      tool);
		gst_tool_add_report_hooks (tool, report_hooks);
		gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);

		init_editable_filters (tool->main_dialog);
		on_network_admin_show (NULL, tool);

		gst_tool_main (tool, TRUE);
		
		if (connection_get_count (tool) == 0)
			g_signal_emit_by_name (G_OBJECT (gst_dialog_get_widget (tool->main_dialog, "connection_add")),
					       "clicked",
					       NULL);
		on_network_notebook_switch_page (gst_dialog_get_widget (tool->main_dialog, "network_admin_notebook"),
						 NULL, 0, NULL);
		gtk_main ();
	}
		
	return 0;
}

