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
	{ "connection_def_gw_omenu", "changed",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "domain",                  "changed",         G_CALLBACK (gst_dialog_modify_cb) },
/*	{ "ip",                      "changed",         G_CALLBACK (gst_dialog_modify_cb) },*/
	{ "ip",                      "changed",         G_CALLBACK (on_hosts_ip_changed) },
	{ "statichost_add",          "clicked",         G_CALLBACK (on_hosts_add_clicked) },
	{ "statichost_add",          "clicked",         G_CALLBACK (gst_dialog_modify_cb) },
	{ "statichost_delete",       "clicked",         G_CALLBACK (on_hosts_delete_clicked) },
	{ "statichost_delete",       "clicked",         G_CALLBACK (gst_dialog_modify_cb) },

/*	{ "dns_list",                "focus_in_event",  G_CALLBACK (update_hint) },*/
/*	{ "search_list",             "focus_in_event",  G_CALLBACK (update_hint) },*/
/*      { "ip",                      "focus_in_event",  G_CALLBACK (update_hint) },*/
/*	{ "alias",                   "focus_in_event",  G_CALLBACK (update_hint) },*/

	/* Network druid callbacks */
	{ "network_connection_window",       "delete_event",  G_CALLBACK (on_network_druid_hide) },
	{ "network_connection_druid",        "cancel", G_CALLBACK (on_network_druid_hide) },
	{ "network_connection_page1",        "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_page2",        "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_other_page1",  "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_plip_page1",   "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_ppp_page1",    "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_ppp_page2",    "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_page_name",         "next",   G_CALLBACK (on_network_druid_page_next) },
	{ "network_connection_page2",        "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_other_page1",  "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_plip_page1",  "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_ppp_page1",    "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_ppp_page2",    "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_page_name",    "back",   G_CALLBACK (on_network_druid_page_back) },
	{ "network_connection_page_finish",  "back",   G_CALLBACK (on_network_druid_page_back) },
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
	{ "network_connection_name",         "changed",   G_CALLBACK (on_network_druid_entry_changed) },
	{ "network_connection_page_finish",  "finish",    G_CALLBACK (on_network_druid_finish) },
	{ "network_connection_other_ip_address", "focus_out_event", G_CALLBACK (on_network_druid_ip_address_focus_out) },

	/* Network profiles callbacks */
	{ "network_profiles_button", "clicked", G_CALLBACK (on_network_profiles_button_clicked) },
	{ "network_profile_new", "clicked", G_CALLBACK (on_network_profile_new_clicked) },
	{ "network_profile_delete", "clicked", G_CALLBACK (on_network_profile_delete_clicked) },
	{ NULL }
};

GstDialogSignal signals_after[] = {
	{ "network_connection_other_page1",  "prepare",   G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_plip_page1",  "prepare",   G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_ppp_page1",    "prepare",   G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_ppp_page2",    "prepare",   G_CALLBACK (on_network_druid_page_prepare) },
	{ "network_connection_page_name",    "prepare",   G_CALLBACK (on_network_druid_page_prepare) },
	{ NULL }
};

static const GstWidgetPolicy policies[] = {
	/* Name                      Basic                        Advanced           Require-Root   Default */
	{ "connections_bbox",        GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "connection_delete",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "connection_configure",    GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
/*	{ "connection_activate",     GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "connection_deactivate",   GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },*/
	{ "connection_def_gw_hbox",  GST_WIDGET_MODE_HIDDEN,      GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "statichost_table",        GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "statichost_add",          GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "statichost_delete",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "samba_use",               GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "description_label",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE, FALSE },
	{ "smbdesc",                 GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE, FALSE },
	{ "workgroup_label",         GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE, FALSE },
	{ "workgroup",               GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE, FALSE },
	{ "wins_use",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE, FALSE },
/*	{ "samba_frame",             GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },*/
	{ "winsserver",              GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "dns_table",               GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "domain",                  GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "domain_label",            GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "dns_list",                GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "dns_list_label",          GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "search_list",             GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "search_list_label",       GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "network_profiles_button", GST_WIDGET_MODE_SENSITIVE,   GST_WIDGET_MODE_SENSITIVE, TRUE, TRUE  },
	{ NULL }
};

static GstReportHookEntry report_hooks[] = {
	{ "file_locate_tool_failed", callbacks_tool_not_found_hook,  GST_REPORT_HOOK_LOAD, TRUE,  NULL },
	{ NULL,                      NULL,                           -1,                   FALSE, NULL }
};

static void
update_notebook_complexity (GstTool *tool, GstDialogComplexity complexity)
{
	GtkWidget *hosts;
	GtkNotebook *notebook;
	gint pageno;

	notebook = GTK_NOTEBOOK (gst_dialog_get_widget (tool->main_dialog, "network_admin_notebook"));
	hosts    = gst_dialog_get_widget (tool->main_dialog, "hosts_container");
	pageno   = gtk_notebook_page_num (notebook, hosts);

	switch (complexity) {
	case GST_DIALOG_BASIC:
		g_return_if_fail (pageno != -1);

		if (gtk_notebook_get_current_page (notebook) == pageno)
			gtk_notebook_set_current_page (notebook, pageno - 1);
		gtk_widget_ref (hosts);
		gtk_widget_unparent (hosts);
		gtk_notebook_remove_page (notebook, pageno);
		break;
	case GST_DIALOG_ADVANCED:
		if (pageno != -1)
			return;

		gtk_notebook_append_page (notebook, hosts,
					  gtk_label_new (_("Hosts")));
		gtk_widget_unref (hosts);
		break;
	default:
		g_warning ("update_notebook_complexity: Unsupported complexity.");
	}
}

static void
update_complexity (GstDialog *dialog, gpointer data)
{
	update_notebook_complexity (dialog->tool, dialog->complexity);
	connection_update_complexity (dialog->tool, dialog->complexity);
}

static void
connect_signals (GstDialog *main_dialog, GstDialogSignal *sigs, GstDialogSignal *sigs_after)
{
	GtkWidget *w;

	g_signal_connect (G_OBJECT (main_dialog), "complexity_change",
			  G_CALLBACK (update_complexity), NULL);

	w = gst_dialog_get_widget (tool->main_dialog, "dns_list");
	g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w))),
			  "changed", G_CALLBACK (gst_dialog_modify_cb), tool->main_dialog);

	w = gst_dialog_get_widget (tool->main_dialog, "search_list");
	g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w))),
			  "changed", G_CALLBACK (gst_dialog_modify_cb), tool->main_dialog);

	gst_dialog_connect_signals (main_dialog, sigs);
	gst_dialog_connect_signals_after (main_dialog, sigs_after);
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
	
	struct poptOption options[] =
	{
		{ "new-connection", '\0', 0, &connection, 0, _("Run the connection druid."), NULL },
		{ "ppp-connection", '\0', 0, &ppp_connection, 0, _("Run the PPP connection druid."), NULL },
		{ "eth-connection", '\0', 0, &eth_connection, 0, _("Run the ethernet connection druid."), NULL },
		{ "wlan-connection", '\0', 0, &wlan_connection, 0, _("Run the wlan connection druid."), NULL },
		{ "plip-connection", '\0', 0, &plip_connection, 0, _("Run the parallel port connection druid."), NULL },
		{ "irlan-connection", '\0', 0, &irlan_connection, 0, _("Run the irlan connection druid."), NULL },
		
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

		gst_tool_load_try (tool);

		g_object_set_data (G_OBJECT (druid), "standalone", GINT_TO_POINTER (TRUE));

		connect_signals (tool->main_dialog, signals, signals_after);
		init_editable_filters (tool->main_dialog);
		network_druid_new (druid, tool, type);
		gtk_widget_show_all (druid_window);

		gtk_main ();
	} else {
		connect_signals (tool->main_dialog, signals, signals_after);
		
		gst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_hostname_hook,     NULL);
		/*gst_dialog_add_apply_hook (tool->main_dialog, callbacks_update_connections_hook, NULL);*/
		gst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_dialer_hook,       tool);
		gst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_gateway_hook,      tool);
		gst_tool_add_report_hooks (tool, report_hooks);
		gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
		gst_dialog_enable_complexity (tool->main_dialog);
		gst_dialog_set_widget_policies (tool->main_dialog, policies);

		init_hint_entries ();
		init_editable_filters (tool->main_dialog);

		on_network_admin_show (NULL, tool);
		update_complexity (tool->main_dialog, NULL);
		gst_tool_main (tool, FALSE);
	}
		
	return 0;
}

