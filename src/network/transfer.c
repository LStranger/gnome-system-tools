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
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>

#include "xst.h"

#include "transfer.h"
#include "callbacks.h"
#include "connection.h"

TransStringEntry transfer_string_entry_table[] =
{
	{ "hostname", "hostname", 0, 0 },
	{ "domain", "domain", 0, 0 },
	{ "smbuse", NULL, "samba_use", 0 },
	{ "workgroup", "workgroup", 0, 0 },
	{ "description", "description", 0, 0 },
	{ "winsuse", NULL, "wins_use", 0 },	
	{ "winsserver", "wins_ip", 0, 0 },	
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
transfer_string_entry_xml_to_gui (XstTool *tool, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *s;

	for (i = 0; transfer_string_entry_table [i].xml_path; i++)
	{
		node = xst_xml_element_find_first (root, transfer_string_entry_table [i].xml_path);

		if (node && (s = xst_xml_element_get_content (node)))
		{
			if (transfer_string_entry_table [i].editable)
				gtk_entry_set_text (GTK_ENTRY (xst_dialog_get_widget
							       (tool->main_dialog,
								transfer_string_entry_table [i].editable)), s);
			
			if (transfer_string_entry_table [i].toggle) {
				gboolean res;
				
				res = (*s == '1')? TRUE: FALSE;
				
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
							      (xst_dialog_get_widget
							       (tool->main_dialog,
								transfer_string_entry_table [i].toggle)), res);
				gtk_signal_emit_by_name (GTK_OBJECT
							 (xst_dialog_get_widget
							  (tool->main_dialog,
							   transfer_string_entry_table [i].toggle)), "toggled");
			}
			
			g_free (s);
		}
	}
}


static void
transfer_string_entry_gui_to_xml (XstTool *tool, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *content = NULL;

	for (i = 0; transfer_string_entry_table [i].xml_path; i++)
	{
		node = xst_xml_element_find_first (root, transfer_string_entry_table [i].xml_path);
		if (!node)
			node = xst_xml_element_add (root, transfer_string_entry_table [i].xml_path);

		if (transfer_string_entry_table [i].editable)
			content = gtk_editable_get_chars (GTK_EDITABLE
							  (xst_dialog_get_widget
							   (tool->main_dialog,
							    transfer_string_entry_table [i].editable)), 0, -1);
		
		if (transfer_string_entry_table [i].toggle) {
			gboolean res;
			
			res = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
							    (xst_dialog_get_widget
							     (tool->main_dialog,
							      transfer_string_entry_table [i].toggle)));
			
			content = g_strdup ((res)? "1": "0");
		}
		
		xst_xml_element_set_content (node, content);
		g_free (content);
	}
}

static void
transfer_string_list_xml_to_gui (XstTool *tool, xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *s;
	GtkWidget *w;
	int position;

	for (i = 0; transfer_string_list_table[i].xml_path; i++)
	{
		w = xst_dialog_get_widget (tool->main_dialog, transfer_string_list_table [i].list);
		position = 0;

		gtk_text_freeze (GTK_TEXT (w));
		gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
		
		for (node = xst_xml_element_find_first (root, transfer_string_list_table [i].xml_path); 
		     node; 
		     node = xst_xml_element_find_next (node, transfer_string_list_table [i].xml_path))
		{
			if ((s = xst_xml_element_get_content (node)))
			{
				gtk_editable_insert_text (GTK_EDITABLE (w), s, strlen (s), &position);
				gtk_editable_insert_text (GTK_EDITABLE (w), "\n", 1, &position);
				g_free (s);
			}
		}

		gtk_text_thaw (GTK_TEXT (w));
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
transfer_string_list_gui_to_xml (XstTool *tool, xmlNodePtr root)
{
	GtkWidget *widget;
	int i;
	gchar *text, *pos = NULL;
	gchar *end;
	xmlNodePtr node;

	for (i = 0; transfer_string_list_table [i].xml_path; i++)
	{
		/* First remove any old branches in the XML tree */

		xst_xml_element_destroy_children_by_name (root, transfer_string_list_table [i].xml_path);

		/* Add branches corresponding to listed data */
		widget = xst_dialog_get_widget (tool->main_dialog, transfer_string_list_table[i].list);
		text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);

		end = text + strlen (text);
		for (; text < end; text = pos + 1) {
			pos = strchr (text, '\n');
			if (pos)
				*pos = 0;
			
			if (transfer_string_is_empty (text))
				continue;

			node = xst_xml_element_add (root, transfer_string_list_table [i].xml_path);
			xst_xml_element_set_content (node, text);

			if (!pos)
				break;
		}
	}
}



static void
transfer_string_clist2_xml_to_gui (XstTool *tool, xmlNodePtr root)
{
	int i, row;
	xmlNodePtr node, nodesub;
	char *s, *entry[3];
	GtkWidget *clist;

	entry[0] = NULL;

	for (i = 0; transfer_string_clist2_table [i].xml_path; i++)
	{
		for (node = xst_xml_element_find_first (
			     root, transfer_string_clist2_table [i].xml_path); 
		     node; 
		     node = xst_xml_element_find_next (
			     node, transfer_string_clist2_table [i].xml_path))
		{
			for (entry[0] = NULL, 
				     nodesub = xst_xml_element_find_first (
					     node, transfer_string_clist2_table [i].xml_path_field_1); 
					     
			     nodesub; 
			     nodesub = xst_xml_element_find_next (
				     nodesub, transfer_string_clist2_table [i].xml_path_field_1))
			{
				if ((s = xst_xml_element_get_content (nodesub)))
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
				     nodesub = xst_xml_element_find_first (
					     node, transfer_string_clist2_table [i].xml_path_field_2); 
			     nodesub; 
			     nodesub = xst_xml_element_find_next (
				     nodesub, transfer_string_clist2_table [i].xml_path_field_2))
			{
				if ((s = xst_xml_element_get_content (nodesub)))
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

			clist = xst_dialog_get_widget (tool->main_dialog, transfer_string_clist2_table [i].clist);

			row = gtk_clist_append (GTK_CLIST (clist), entry);

			g_free (entry[0]);
			g_free (entry[1]);
		}
	}
}


static void
transfer_string_clist2_gui_to_xml_item (XstTool *tool, xmlNodePtr root, TransStringCList2 *specs)
{
	gchar *node_name;
	gchar *field_one;
	gchar *field_rest;
	gchar *widget_name;

	gchar *text_1;
	gchar *text_2;

	GtkWidget *widget;
	gint row;
	gint rows;
	gint j;

	gchar **col1_elem;
	
	xmlNodePtr node, node2;
	gboolean col0_added;

	node_name   = specs->xml_path;
	field_one   = specs->xml_path_field_1;
	field_rest  = specs->xml_path_field_2;
	widget_name = specs->clist;

	/* Get the clist */
	widget = xst_dialog_get_widget (tool->main_dialog, widget_name);
	g_return_if_fail (GTK_IS_CLIST (widget));

	/* First remove any old branches in the XML tree */
	xst_xml_element_destroy_children_by_name (root, node_name);

	rows = GTK_CLIST (widget)->rows;
	
	for (row = 0; row < rows; row++)
	{
		if (!gtk_clist_get_text (GTK_CLIST (widget), row, 0, &text_1))
			break;
		if (!gtk_clist_get_text (GTK_CLIST (widget), row, 1, &text_2))
			continue;
		
		if (!strlen (text_1))
			continue;
		
		/* Enclosing element */
		node = xst_xml_element_add (root, node_name);
		
		col1_elem = g_strsplit (text_2, " ", 0);

		for (j = 0, col0_added = FALSE; col1_elem[j]; j++)
		{
			if (!strlen (col1_elem[j]))
				continue;
			if (!col0_added)
			{
				node2 = xst_xml_element_add (node, field_one);
				xst_xml_element_set_content (node2, text_1);
				col0_added = TRUE;
			}
			node2 = xst_xml_element_add (node, field_rest);
			xst_xml_element_set_content (node2, col1_elem[j]);
		}
		
		g_strfreev (col1_elem);
	}
}

static void
transfer_string_clist2_gui_to_xml (XstTool *tool, xmlNodePtr root)
{
	TransStringCList2 *specs;
	gint i;

	for (i = 0; transfer_string_clist2_table [i].xml_path; i++)
	{
		specs = &transfer_string_clist2_table [i];

		transfer_string_clist2_gui_to_xml_item (tool, root, specs);
	}
}

static void
transfer_interfaces_to_xml (XstTool *tool, xmlNodePtr root)
{
	GtkWidget *clist;
	GList *l;
	int i;

	clist = xst_dialog_get_widget (tool->main_dialog, "connection_list");
	for (i=0; i < GTK_CLIST (clist)->rows; i++)
		connection_save_to_node (gtk_clist_get_row_data (GTK_CLIST (clist), i), root);

	for (l = gtk_object_get_data (GTK_OBJECT (clist), "lo"); l; l = l->next)
		connection_save_to_node (l->data, root);
}

static void
transfer_interfaces_to_gui (XstTool *tool, xmlNodePtr root)
{
	xmlNodePtr node;

	for (node = xst_xml_element_find_first (root, "interface"); 
	     node; 
	     node = xst_xml_element_find_next (node, "interface"))
		connection_new_from_node (node);

	callbacks_update_connections_hook (tool->main_dialog, NULL);
	connection_update_complexity (tool, tool->main_dialog->complexity);
}

static void
transfer_gatewaydev_to_xml (XstTool *tool, xmlNodePtr root)
{
	gchar *dev, *gateway;
	xmlNodePtr node;

	gateway = gtk_object_get_data (GTK_OBJECT (tool), "gateway");
	node = xst_xml_element_find_first (root, "gateway");
	if (!node)
		node = xst_xml_element_add (root, "gateway");
	xst_xml_element_set_content (node, (gateway)? gateway: "");
		
	if (gtk_object_get_data (GTK_OBJECT (tool), "gwdevunsup"))
		return;
	
	dev = gtk_object_get_data (GTK_OBJECT (tool), "gatewaydev");
	node = xst_xml_element_find_first (root, "gatewaydev");
	if (!node)
		node = xst_xml_element_add (root, "gatewaydev");
	xst_xml_element_set_content (node, (dev)? dev: "");
}

static void
transfer_xml_to_gatewaydev (XstTool *tool, xmlNodePtr root)
{
	gchar *dev;
	gboolean unsup;
	
	unsup = xst_xml_element_get_boolean (root, "gwdevunsup");
	if (unsup) {
		gtk_object_set_data (GTK_OBJECT (tool), "gwdevunsup", (gpointer) TRUE);
		xst_dialog_widget_set_user_mode (tool->main_dialog,
						 "connection_def_gw_hbox",
						 XST_WIDGET_MODE_HIDDEN);
		return;
	}
		
	dev = xst_xml_get_child_content (root, "gatewaydev");
	if (dev) {
		connection_default_gw_init (tool, dev);
		g_free (dev);
	}
}

static void
transfer_misc_tool_to_xml (XstTool *tool, xmlNodePtr root)
{
	transfer_gatewaydev_to_xml (tool, root);
}

static void
transfer_misc_xml_to_tool (XstTool *tool, xmlNodePtr root)
{
	gboolean res;

	if (xst_xml_element_find_first (root, "smbinstalled"))
		gtk_object_set_data (GTK_OBJECT (tool), "tool_configured", (gpointer) TRUE);
	
	res = xst_xml_element_get_boolean (root, "smbinstalled");
	gtk_object_set_data (GTK_OBJECT (tool), "smbinstalled", (gpointer) res);

	res = xst_xml_element_get_boolean (root, "dialinstalled");
	gtk_object_set_data (GTK_OBJECT (tool), "dialinstalled", (gpointer) res);

	res = xst_xml_element_get_boolean (root, "smartdhcpcd");
	gtk_object_set_data (GTK_OBJECT (tool), "smartdhcpcd", (gpointer) res);

	res = xst_xml_element_get_boolean (root, "userifacectl");
	gtk_object_set_data (GTK_OBJECT (tool), "userifacectl", (gpointer) res);

	transfer_xml_to_gatewaydev (tool, root);
}
	
void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNode *root = xst_xml_doc_get_root (tool->config);
	
	transfer_string_entry_xml_to_gui (tool, root);
	transfer_string_list_xml_to_gui (tool, root);
	transfer_string_clist2_xml_to_gui (tool, root);
	transfer_interfaces_to_gui (tool, root);
	/* misc has to go last */
	transfer_misc_xml_to_tool (tool, root);
}


void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
	xmlNode *root = xst_xml_doc_get_root (tool->config);

	transfer_string_entry_gui_to_xml (tool, root);
	transfer_string_list_gui_to_xml (tool, root);
	transfer_string_clist2_gui_to_xml (tool, root);
	transfer_interfaces_to_xml (tool, root);
	/* misc has to go last */
	transfer_misc_tool_to_xml (tool, root);
}
