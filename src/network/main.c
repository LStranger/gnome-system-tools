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

XstTool *tool = NULL;

static void
watch_it_now_watch_it (GtkEditable *e, gint start_pos, gint end_pos, gpointer data)
{
	xst_dialog_modify (tool->main_dialog);
}

static void
connect_signals (void)
{
	GladeXML *xml = tool->main_dialog->gui;	

	glade_xml_signal_connect_data (xml, "on_dns_dhcp_toggled", on_dns_dhcp_toggled, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_network_admin_show", on_network_admin_show, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_network_notebook_switch_page", on_network_notebook_switch_page, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_samba_use_toggled", on_samba_use_toggled, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_statichost_add_clicked", on_statichost_add_clicked, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_statichost_changed", on_statichost_changed, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_statichost_delete_clicked", on_statichost_delete_clicked, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_statichost_list_select_row", on_statichost_list_select_row, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_statichost_list_unselect_row", on_statichost_list_unselect_row, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_statichost_update_clicked", on_statichost_update_clicked, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_status_button_toggled", on_status_button_toggled, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_wvlan_adhoc_toggled", on_wvlan_adhoc_toggled, tool->main_dialog);

	glade_xml_signal_connect_data (xml, "tool_modified_cb", xst_dialog_modify_cb, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "delete_modified", watch_it_now_watch_it, tool->main_dialog);

	glade_xml_signal_connect_data (xml, "update_hint", update_hint, tool->main_dialog);

	glade_xml_signal_connect_data (xml, "on_connection_add_clicked", on_connection_add_clicked, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_connection_configure_clicked", on_connection_configure_clicked, tool->main_dialog);
	glade_xml_signal_connect_data (xml, "on_connection_delete_clicked", on_connection_delete_clicked, tool->main_dialog);

	gtk_signal_connect (GTK_OBJECT (tool), "fill_gui",
			    GTK_SIGNAL_FUNC (transfer_xml_to_gui),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (tool), "fill_xml",
			    GTK_SIGNAL_FUNC (transfer_gui_to_xml),
			    NULL);

	gtk_signal_connect_object (GTK_OBJECT (tool->main_dialog), "apply",
				   GTK_SIGNAL_FUNC (xst_tool_save),
				   GTK_OBJECT (tool));		
}

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
	
	init_hint_entries ();
	
	tool = xst_tool_init ("network", _("Network Settings - Ximian Setup Tools"), argc, argv);
	xst_dialog_freeze (tool->main_dialog);

	init_icons ();
	connect_signals ();

	for (i=0; s[i]; i++)
		connect_editable_filter (xst_dialog_get_widget (tool->main_dialog, s[i]), e[i]);

	on_network_admin_show (NULL, NULL);

	xst_dialog_thaw (tool->main_dialog);
	xst_tool_main (tool);

	return 0;
}
