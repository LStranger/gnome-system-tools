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
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>

#include "global.h"

#include "transfer.h"
#include "callbacks.h"
#include "connection.h"
#include "hosts.h"
#include "ppp-druid.h"

XstTool *tool = NULL;

XstDialogSignal signals[] = {
	{ "network_admin",         "switch_page",     on_network_notebook_switch_page },
	{ "hostname",              "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "hostname",              "changed",         xst_dialog_modify_cb },
	{ "samba_use",             "toggled",         on_samba_use_toggled },
	{ "samba_use",             "toggled",         xst_dialog_modify_cb },
	{ "description",           "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "description",           "changed",         xst_dialog_modify_cb },
	{ "workgroup",             "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "workgroup",             "changed",         xst_dialog_modify_cb },
	{ "wins_ip",               "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "wins_ip",               "changed",         xst_dialog_modify_cb },
	{ "wins_use",              "toggled",         on_wins_use_toggled },
	{ "wins_use",              "toggled",         xst_dialog_modify_cb },
	{ "connection_list",       "select_row",      on_connection_list_select_row },
	{ "connection_list",       "unselect_row",    on_connection_list_unselect_row },
	{ "connection_add",        "clicked",         on_connection_add_clicked },
	{ "connection_delete",     "clicked",         on_connection_delete_clicked },
	{ "connection_configure",  "clicked",         on_connection_configure_clicked },
	{ "connection_activate",   "clicked",         on_connection_activate_clicked },
	{ "connection_deactivate", "clicked",         on_connection_deactivate_clicked },
	{ "dns_dhcp",              "toggled",         on_dns_dhcp_toggled },
	{ "dns_dhcp",              "toggled",         xst_dialog_modify_cb },
	{ "dns_list",              "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "dns_list",              "changed",         xst_dialog_modify_cb },
	{ "domain",                "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "domain",                "changed",         xst_dialog_modify_cb },
	{ "search_list",           "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "search_list",           "changed",         xst_dialog_modify_cb },
	{ "statichost_list",       "unselect_row",    on_hosts_list_unselect_row },
	{ "statichost_list",       "select_row",      on_hosts_list_select_row },
	{ "ip",                    "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "ip",                    "changed",         xst_dialog_modify_cb },
	{ "ip",                    "changed",         on_hosts_ip_changed },
	{ "alias",                 "focus_in_event",  GTK_SIGNAL_FUNC (update_hint) },
	{ "alias",                 "changed",         on_hosts_alias_changed },
	{ "statichost_add",        "clicked",         on_hosts_add_clicked },
	{ "statichost_add",        "clicked",         xst_dialog_modify_cb },
	{ "statichost_update",     "clicked",         on_hosts_update_clicked },
	{ "statichost_update",     "clicked",         xst_dialog_modify_cb },
	{ "statichost_delete",     "clicked",         on_hosts_delete_clicked },
	{ "statichost_delete",     "clicked",         xst_dialog_modify_cb },
	{ NULL }
};


int
main (int argc, char *argv[])
{
	int i;
	char *s[] = { 
		"wins_ip",
		"dns_list",
		"search_list",
		"ip",
		"alias",
		NULL
	};
	EditableFilterRules e[] = {
		EF_ALLOW_NONE,
		EF_ALLOW_ENTER,
		EF_ALLOW_ENTER | EF_ALLOW_TEXT| EF_ALLOW_SPACE,
		EF_STATIC_HOST,
		EF_STATIC_HOST | EF_ALLOW_ENTER | EF_ALLOW_TEXT | EF_ALLOW_SPACE
	};

	gboolean internet_druid = FALSE;
	
	struct poptOption options[] =
	{
		{ "internet", '\0', 0, &internet_druid, 0,
		  N_("	Run the internet conection druid."), NULL },
		
		{NULL, '\0', 0, NULL, 0}
	};

	init_hint_entries ();
	
	tool = xst_tool_init ("network", _("Network Settings"), argc, argv, options);

	if (internet_druid) {
		PppDruid *ppp;

		ppp = ppp_druid_new (tool);
		xst_tool_set_xml_funcs (tool, NULL, ppp_druid_gui_to_xml, ppp);
		
		xst_tool_load_try (tool);
		ppp_druid_show (ppp);

		gtk_main ();
	} else {
		xst_dialog_connect_signals (tool->main_dialog, signals);
		xst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
		
		connection_init_icons ();

		for (i=0; s[i]; i++)
			connect_editable_filter (xst_dialog_get_widget (tool->main_dialog, s[i]), e[i]);
		
		on_network_admin_show (NULL, NULL);
		xst_tool_main (tool);
	}
		
	return 0;
}

