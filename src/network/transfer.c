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
 * Authors: Hans Petter Jansson <hpj@ximian.com> and Arturo Espinosa <arturo@ximian.com>.
 */

/* Functions for transferring information between XML tree and UI */

#include <ctype.h>

#include <gnome.h>

#include "gst.h"

#include "transfer.h"
#include "callbacks.h"
#include "connection.h"
#include "profile.h"

TransStringEntry transfer_string_entry_table[] =
{
	{ "hostname", "hostname", 0, 0 },
	{ "domain", "domain", 0, 0 },
	{ "smbuse", NULL, "samba_use", 0 },
	{ "workgroup", "workgroup", 0, 0 },
	{ "smbdesc", "smbdesc", 0, 0 },
	{ "winsuse", NULL, "wins_use", 0 },
	{ "winsserver", "winsserver", 0, 0 },	
	{ 0, 0, 0, 0 }
};


TransStringList transfer_string_list_table[] =
{
	{ "nameserver", "dns_list" },
	{ "searchdomain", "search_list" },
	{ 0, 0 }
};


TransStringCList2 transfer_string_clist2_table[] = {
	{ "statichost", "ip", "alias", "statichost_list" },
	{ 0, 0, 0, 0 }
};


static void
transfer_string_entry_xml_to_gui (GstTool *tool, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *s;

	for (i = 0; transfer_string_entry_table [i].xml_path; i++)
	{
		node = gst_xml_element_find_first (root, transfer_string_entry_table [i].xml_path);

		if (node && (s = gst_xml_element_get_content (node)))
		{
			if (transfer_string_entry_table [i].editable)
				gtk_entry_set_text (GTK_ENTRY (gst_dialog_get_widget
							       (tool->main_dialog,
								transfer_string_entry_table [i].editable)), s);
			
			if (transfer_string_entry_table [i].toggle) {
				gboolean res;
				
				res = (*s == '1')? TRUE: FALSE;
				
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							      (gst_dialog_get_widget
							       (tool->main_dialog,
								transfer_string_entry_table [i].toggle)), res);
				gtk_signal_emit_by_name (GTK_OBJECT
							 (gst_dialog_get_widget
							  (tool->main_dialog,
							   transfer_string_entry_table [i].toggle)), "toggled");
			}
			
			g_free (s);
		}
	}
}


static void
transfer_string_entry_gui_to_xml (GstTool *tool, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *content = NULL;

	for (i = 0; transfer_string_entry_table [i].xml_path; i++)
	{
		node = gst_xml_element_find_first (root, transfer_string_entry_table [i].xml_path);
		if (!node)
			node = gst_xml_element_add (root, transfer_string_entry_table [i].xml_path);

		if (transfer_string_entry_table [i].editable)
			content = gtk_editable_get_chars (GTK_EDITABLE
							  (gst_dialog_get_widget
							   (tool->main_dialog,
							    transfer_string_entry_table [i].editable)), 0, -1);
		
		if (transfer_string_entry_table [i].toggle) {
			gboolean res;
			
			res = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							    (gst_dialog_get_widget
							     (tool->main_dialog,
							      transfer_string_entry_table [i].toggle)));
			
			content = g_strdup ((res)? "1": "0");
		}
		
		gst_xml_element_set_content (node, content);
		g_free (content);
	}
}

static void
transfer_string_list_xml_to_gui (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr  node;
	GtkWidget  *w;
	gint        i;
	gchar      *s;

	for (i = 0; transfer_string_list_table[i].xml_path; i++) {
		w = gst_dialog_get_widget (tool->main_dialog, transfer_string_list_table [i].list);

		gst_ui_text_view_clear (GTK_TEXT_VIEW (w));

		for (node = gst_xml_element_find_first (root, transfer_string_list_table [i].xml_path); 
		     node; 
		     node = gst_xml_element_find_next (node, transfer_string_list_table [i].xml_path)) {
			if ((s = gst_xml_element_get_content (node))) {
				gst_ui_text_view_add_text (GTK_TEXT_VIEW (w), s);
				gst_ui_text_view_add_text (GTK_TEXT_VIEW (w), "\n");
			}
		}
	}
}

static gboolean
transfer_string_is_empty (gchar *str)
{
	gchar    *s;

	for (s = str; *s; s++)
		if (isalnum (*s))
			return FALSE;
	return TRUE;
}

static void
transfer_string_list_gui_to_xml (GstTool *tool, xmlNodePtr root)
{
	GtkWidget *widget;
	int i;
	gchar *text, *pos = NULL;
	gchar *end;
	xmlNodePtr node;

	for (i = 0; transfer_string_list_table [i].xml_path; i++)
	{
		/* First remove any old branches in the XML tree */

		gst_xml_element_destroy_children_by_name (root, transfer_string_list_table [i].xml_path);

		/* Add branches corresponding to listed data */
		widget = gst_dialog_get_widget (tool->main_dialog, transfer_string_list_table[i].list);
		text = gst_ui_text_view_get_text (GTK_TEXT_VIEW (widget));

		end = text + strlen (text);
		for (; text < end; text = pos + 1) {
			pos = (gchar *) strchr (text, '\n');
			if (pos)
				*pos = 0;
			
			if (transfer_string_is_empty (text))
				continue;

			node = gst_xml_element_add (root, transfer_string_list_table [i].xml_path);
			gst_xml_element_set_content (node, text);

			if (!pos)
				break;
		}
	}
}

static void
transfer_string_clist2_xml_to_gui (GstTool *tool, xmlNodePtr root)
{
	int i;
	xmlNodePtr node, nodesub;
	char *s, *entry[3];

	hosts_list_clear (tool);
	
	entry[0] = NULL;

	for (i = 0; transfer_string_clist2_table [i].xml_path; i++)
	{
		for (node = gst_xml_element_find_first (
			     root, transfer_string_clist2_table [i].xml_path); 
		     node; 
		     node = gst_xml_element_find_next (
			     node, transfer_string_clist2_table [i].xml_path))
		{
			for (entry[0] = NULL, 
				     nodesub = gst_xml_element_find_first (
					     node, transfer_string_clist2_table [i].xml_path_field_1); 
					     
			     nodesub; 
			     nodesub = gst_xml_element_find_next (
				     nodesub, transfer_string_clist2_table [i].xml_path_field_1))
			{
				if ((s = gst_xml_element_get_content (nodesub)))
				{
					if (!entry[0])
						entry[0] = s;
					else
					{
						entry[0] = g_strjoin (" ", entry[1], s, NULL);
					}
				}
			}

			if (!entry[0])
				continue;

			for (entry[1] = NULL, 
				     nodesub = gst_xml_element_find_first (
					     node, transfer_string_clist2_table [i].xml_path_field_2); 
			     nodesub; 
			     nodesub = gst_xml_element_find_next (
				     nodesub, transfer_string_clist2_table [i].xml_path_field_2))
			{
				if ((s = gst_xml_element_get_content (nodesub)))
				{
					if (!entry[1])
						entry[1] = s;
					else
					{
						gchar *free_me = entry [1];
						entry[1] = g_strjoin (" ", entry[1], s, NULL);
						g_free (free_me);
						g_free (s);
					}
				}
			}

			if (!entry[1])
				continue;

			hosts_list_append (tool, entry);

			g_free (entry[0]);
			g_free (entry[1]);
		}
	}
}

static void
transfer_interfaces_to_xml (GstTool *tool, xmlNodePtr root)
{
	connection_list_save (tool);
}

static void
transfer_interfaces_to_gui (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr node;

	for (node = gst_xml_element_find_first (root, "interface"); 
	     node; 
	     node = gst_xml_element_find_next (node, "interface")) {
		connection_new_from_node (node);
	}

	callbacks_update_connections_hook (tool->main_dialog, NULL);
	connection_update_complexity (tool, tool->main_dialog->complexity);
}

static void
transfer_gatewaydev_to_xml (GstTool *tool, xmlNodePtr root)
{
	gchar *dev, *gateway;
	xmlNodePtr node;

	gateway = g_object_get_data (G_OBJECT (tool), "gateway");
	node = gst_xml_element_find_first (root, "gateway");
	if (!node)
		node = gst_xml_element_add (root, "gateway");
	gst_xml_element_set_content (node, (gateway)? gateway: "");
		
	if (g_object_get_data (G_OBJECT (tool), "gwdevunsup"))
		return;
	
	dev = g_object_get_data (G_OBJECT (tool), "gatewaydev");
	node = gst_xml_element_find_first (root, "gatewaydev");
	if (!node)
		node = gst_xml_element_add (root, "gatewaydev");
	gst_xml_element_set_content (node, (dev)? dev: "");
}

static void
transfer_xml_to_gatewaydev (GstTool *tool, xmlNodePtr root)
{
	gchar *dev;
	gboolean unsup;
	
	unsup = gst_xml_element_get_boolean (root, "gwdevunsup");
	if (unsup) {
		g_object_set_data (G_OBJECT (tool), "gwdevunsup", (gpointer) TRUE);
		gst_dialog_widget_set_user_mode (tool->main_dialog,
						 "connection_def_gw_hbox",
						 GST_WIDGET_MODE_HIDDEN);
		return;
	}
		
	dev = gst_xml_get_child_content (root, "gatewaydev");
	if (dev) {
		connection_default_gw_init (tool, dev);
		g_free (dev);
	}
}

static void
transfer_misc_tool_to_xml (GstTool *tool, xmlNodePtr root)
{
	transfer_gatewaydev_to_xml (tool, root);
}

static void
transfer_misc_xml_to_tool (GstTool *tool, xmlNodePtr root)
{
	gboolean res;

	if (gst_xml_element_find_first (root, "smbinstalled"))
		g_object_set_data (G_OBJECT (tool), "tool_configured", (gpointer) TRUE);
	
	res = gst_xml_element_get_boolean (root, "smbinstalled");
	g_object_set_data (G_OBJECT (tool), "smbinstalled", (gpointer) res);

	res = gst_xml_element_get_boolean (root, "dialinstalled");
	g_object_set_data (G_OBJECT (tool), "dialinstalled", (gpointer) res);

	res = gst_xml_element_get_boolean (root, "smartdhcpcd");
	g_object_set_data (G_OBJECT (tool), "smartdhcpcd", (gpointer) res);

	res = gst_xml_element_get_boolean (root, "userifacectl");
	g_object_set_data (G_OBJECT (tool), "userifacectl", (gpointer) res);

	transfer_xml_to_gatewaydev (tool, root);
}

void
transfer_profile_to_gui (GstTool *tool, gpointer data)
{
	xmlNode *root = gst_xml_doc_get_root (tool->config);

	transfer_string_entry_xml_to_gui (tool, root);
	transfer_string_list_xml_to_gui (tool, root);
	transfer_string_clist2_xml_to_gui (tool, root);
	transfer_interfaces_to_gui (tool, root);

	/* misc has to go last */
	transfer_misc_xml_to_tool (tool, root);
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNode *root = gst_xml_doc_get_root (tool->config);

	transfer_string_entry_xml_to_gui (tool, root);
	transfer_string_list_xml_to_gui (tool, root);
	transfer_string_clist2_xml_to_gui (tool, root);

	connection_list_clear (tool);
	transfer_interfaces_to_gui (tool, root);

	profile_populate_option_menu (tool, root);
	profiles_table_populate (tool, root);
	
	/* misc has to go last */
	transfer_misc_xml_to_tool (tool, root);
}

void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
	xmlNode *root = gst_xml_doc_get_root (tool->config);

	transfer_string_entry_gui_to_xml (tool, root);
	transfer_string_list_gui_to_xml (tool, root);
	hosts_list_save (tool, root);
	transfer_interfaces_to_xml (tool, root);
	/* misc has to go last */
	transfer_misc_tool_to_xml (tool, root);
}
