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

#include "xst.h"

#include "transfer.h"
#include "callbacks.h"
#include "connection.h"
#include "hosts.h"
#include "ppp-druid.h"

XstTool *tool;

XstDialogSignal signals[] = {
	{ "network_admin",           "switch_page",     G_CALLBACK (on_network_notebook_switch_page) },
	{ "hostname",                "focus_in_event",  G_CALLBACK (update_hint) },
	{ "hostname",                "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "samba_use",               "toggled",         G_CALLBACK (on_samba_use_toggled) },
	{ "description",             "focus_in_event",  G_CALLBACK (update_hint) },
	{ "description",             "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "workgroup",               "focus_in_event",  G_CALLBACK (update_hint) },
	{ "workgroup",               "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "wins_ip",                 "focus_in_event",  G_CALLBACK (update_hint) },
	{ "wins_ip",                 "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "wins_use",                "toggled",         G_CALLBACK (on_wins_use_toggled) },
	{ "wins_use",                "toggled",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "connection_add",          "clicked",         G_CALLBACK (on_connection_add_clicked) },
	{ "connection_delete",       "clicked",         G_CALLBACK (on_connection_delete_clicked) },
	{ "connection_configure",    "clicked",         G_CALLBACK (on_connection_configure_clicked) },
	{ "connection_activate",     "clicked",         G_CALLBACK (on_connection_activate_clicked) },
	{ "connection_deactivate",   "clicked",         G_CALLBACK (on_connection_deactivate_clicked) },
	{ "connection_def_gw_omenu", "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "dns_list",                "focus_in_event",  G_CALLBACK (update_hint) },
	{ "domain",                  "focus_in_event",  G_CALLBACK (update_hint) },
	{ "domain",                  "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "search_list",             "focus_in_event",  G_CALLBACK (update_hint) },
	{ "ip",                      "focus_in_event",  G_CALLBACK (update_hint) },
	{ "ip",                      "changed",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "ip",                      "changed",         G_CALLBACK (on_hosts_ip_changed) },
	{ "alias",                   "focus_in_event",  G_CALLBACK (update_hint) },
	{ "statichost_add",          "clicked",         G_CALLBACK (on_hosts_add_clicked) },
	{ "statichost_add",          "clicked",         G_CALLBACK (xst_dialog_modify_cb) },
	{ "statichost_delete",       "clicked",         G_CALLBACK (on_hosts_delete_clicked) },
	{ "statichost_delete",       "clicked",         G_CALLBACK (xst_dialog_modify_cb) },
	{ NULL }
};

static const XstWidgetPolicy policies[] = {
	/* Name                      Basic                        Advanced           Require-Root   Default */
	{ "general_hbox",            XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "connections_bbox",        XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "connection_delete",       XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "connection_configure",    XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "connection_activate",     XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "connection_deactivate",   XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "connection_def_gw_hbox",  XST_WIDGET_MODE_HIDDEN,      XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "statichost_table",        XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "statichost_add",          XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "statichost_delete",       XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "samba_use",               XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "samba_frame",             XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "wins_ip",                 XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  FALSE },
	{ "dns_table",               XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "domain",                  XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "domain_label",            XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "dns_list",                XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "dns_list_label",          XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "search_list",             XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ "search_list_label",       XST_WIDGET_MODE_SENSITIVE,   XST_WIDGET_MODE_SENSITIVE, TRUE,  TRUE  },
	{ NULL }
};

static XstReportHookEntry report_hooks[] = {
	{ "file_locate_tool_failed", callbacks_tool_not_found_hook,  XST_REPORT_HOOK_LOAD, TRUE,  NULL },
	{ NULL,                      NULL,                           -1,                   FALSE, NULL }
};

static void
update_notebook_complexity (XstTool *tool, XstDialogComplexity complexity)
{
	GtkWidget *hosts;
	GtkNotebook *notebook;
	gint pageno;

	notebook = GTK_NOTEBOOK (xst_dialog_get_widget (tool->main_dialog, "network_admin"));
	hosts    = xst_dialog_get_widget (tool->main_dialog, "hosts_container");
	pageno   = gtk_notebook_page_num (notebook, hosts);

	switch (complexity) {
	case XST_DIALOG_BASIC:
		g_return_if_fail (pageno != -1);

		if (gtk_notebook_get_current_page (notebook) == pageno)
			gtk_notebook_set_current_page (notebook, pageno - 1);
		gtk_widget_ref (hosts);
		gtk_widget_unparent (hosts);
		gtk_notebook_remove_page (notebook, pageno);
		break;
	case XST_DIALOG_ADVANCED:
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
update_complexity (XstDialog *dialog, gpointer data)
{
	update_notebook_complexity (dialog->tool, dialog->complexity);
	connection_update_complexity (dialog->tool, dialog->complexity);
}

static void
connect_signals (XstDialog *main_dialog, XstDialogSignal *sigs)
{
	GtkWidget *w;

	g_signal_connect (G_OBJECT (main_dialog), "complexity_change",
			  G_CALLBACK (update_complexity), NULL);

	w = xst_dialog_get_widget (tool->main_dialog, "dns_list");
	g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w))),
			  "changed", G_CALLBACK (xst_dialog_modify_cb), tool->main_dialog);

	w = xst_dialog_get_widget (tool->main_dialog, "search_list");
	g_signal_connect (G_OBJECT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w))),
			  "changed", G_CALLBACK (xst_dialog_modify_cb), tool->main_dialog);

	xst_dialog_connect_signals (main_dialog, sigs);
}

int
main (int argc, char *argv[])
{
	gboolean internet_druid = FALSE;
	
	struct poptOption options[] =
	{
		{ "internet", '\0', 0, &internet_druid, 0,
		  N_("	Run the internet conection druid."), NULL },
		
		{NULL, '\0', 0, NULL, 0}
	};

	xst_init ("network-admin", argc, argv, options);
	tool = xst_tool_new ();
	xst_tool_construct (tool , "network", _("Network Settings"));

	if (internet_druid) {
		PppDruid *ppp;

		ppp = ppp_druid_new (tool);
		xst_tool_set_xml_funcs (tool, NULL, ppp_druid_gui_to_xml, ppp);

		xst_tool_load_try (tool);
		ppp_druid_show (ppp);

		gtk_main ();
	} else {
		connect_signals (tool->main_dialog, signals);
		xst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_hostname_hook,     NULL);
		/*xst_dialog_add_apply_hook (tool->main_dialog, callbacks_update_connections_hook, NULL);*/
		xst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_dialer_hook,       tool);
		xst_dialog_add_apply_hook (tool->main_dialog, callbacks_check_gateway_hook,      tool);
		xst_tool_add_report_hooks (tool, report_hooks);
		xst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
		xst_dialog_enable_complexity (tool->main_dialog);
		xst_dialog_set_widget_policies (tool->main_dialog, policies);
		
		init_hint_entries ();
		init_editable_filters (tool->main_dialog);

		on_network_admin_show (NULL, tool);
		update_complexity (tool->main_dialog, NULL);
		xst_tool_main (tool, FALSE);
	}
		
	return 0;
}

