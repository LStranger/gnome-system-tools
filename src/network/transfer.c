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

#include <gnome.h>
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>

#include "global.h"

#include "transfer.h"

/* define to x for debugging output */
#define d(x) x

TransStringEntry transfer_string_entry_table[] =
{
	{ "hostname", "hostname", 0, 0 },
	{ "domain", "domain", 0, 0 },
	{ "workgroup", "workgroup", 0, 0 },
	{ "description", "description", 0, 0 },
	{ "winsserver", "wins_ip" },	
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
transfer_string_entry_xml_to_gui (xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *s;

	for (i = 0; transfer_string_entry_table [i].xml_path; i++)
	{
		node = xml_element_find_first (root, transfer_string_entry_table [i].xml_path);

		if (node && (s = xml_element_get_content (node)))
		{
			gtk_entry_set_text (GTK_ENTRY (tool_widget_get (transfer_string_entry_table [i].editable)), s);

			if (transfer_string_entry_table [i].toggle)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tool_widget_get (transfer_string_entry_table [i].toggle)), TRUE);

			g_free (s);
		}
	}
}


static void
transfer_string_entry_gui_to_xml (xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *content;

	for (i = 0; transfer_string_entry_table [i].xml_path; i++)
	{
		node = xml_element_find_first (root, transfer_string_entry_table [i].xml_path);
		if (!node)
			node = xml_element_add (root, transfer_string_entry_table [i].xml_path);

		content = gtk_editable_get_chars (GTK_EDITABLE (tool_widget_get (transfer_string_entry_table [i].editable)), 0, -1);
		xml_element_set_content (node, content);
		g_free (content);
	}
}

static void
transfer_string_list_xml_to_gui (xmlNodePtr root)
{
	int i;
	xmlNodePtr node;
	gchar *s;
	GtkWidget *w;
	int position;

	for (i = 0; transfer_string_list_table[i].xml_path; i++)
	{
		w = tool_widget_get (transfer_string_list_table [i].list);
		position = 0;

		gtk_text_freeze (GTK_TEXT (w));
		gtk_editable_delete_text (GTK_EDITABLE (w), 0, -1);
		
		for (node = xml_element_find_first (root, transfer_string_list_table [i].xml_path); 
		     node; 
		     node = xml_element_find_next (node, transfer_string_list_table [i].xml_path))
		{
			if ((s = xml_element_get_content (node)))
			{
				gtk_editable_insert_text (GTK_EDITABLE (w), s, strlen (s), &position);
				gtk_editable_insert_text (GTK_EDITABLE (w), "\n", 1, &position);
				g_free (s);
			}
		}

		gtk_text_thaw (GTK_TEXT (w));
	}
}


static void
transfer_string_list_gui_to_xml (xmlNodePtr root)
{
	int i;
	gchar *text;
	xmlNodePtr node;

	for (i = 0; transfer_string_list_table [i].xml_path; i++)
	{
		/* First remove any old branches in the XML tree */

		xml_element_destroy_children_by_name (root, transfer_string_list_table [i].xml_path);

		/* Add branches corresponding to listed data */
		for (text = gtk_editable_get_chars (
			     GTK_EDITABLE (tool_widget_get (transfer_string_list_table[i].list)), 0, -1);
		     text; text = strchr (text, ' ')) {
			if (!*text)
				continue;

			node = xml_element_add (root, transfer_string_list_table [i].xml_path);
			xml_element_set_content (node, text);
		}
	}
}


static void
transfer_string_clist2_xml_to_gui (xmlNodePtr root)
{
	int i, row;
	xmlNodePtr node, nodesub;
	char *s, *entry[4];
	GtkWidget *clist;

	entry[0] = entry[3] = NULL;

	for (i = 0; transfer_string_clist2_table [i].xml_path; i++)
	{
		for (node = xml_element_find_first (
			     root, transfer_string_clist2_table [i].xml_path); 
		     node; 
		     node = xml_element_find_next (
			     node, transfer_string_clist2_table [i].xml_path))
		{
			for (entry[1] = NULL, 
				     nodesub = xml_element_find_first (
					     node, transfer_string_clist2_table [i].xml_path_field_1); 
					     
			     nodesub; 
			     nodesub = xml_element_find_next (
				     nodesub, transfer_string_clist2_table [i].xml_path_field_1))
			{
				if ((s = xml_element_get_content (nodesub)))
				{
					if (!entry[1])
						entry[1] = s;
					else
					{
						entry[1] = g_strjoin (" ", entry[1], s, NULL);
					}
				}
			}

			d(g_print ("name: (%d) %s\n", xml_element_get_bool_attr (node, "enabled"), node->name));

			if (!entry[1])
				continue;

			for (entry[2] = NULL, 
				     nodesub = xml_element_find_first (
					     node, transfer_string_clist2_table [i].xml_path_field_2); 
			     nodesub; 
			     nodesub = xml_element_find_next (
				     nodesub, transfer_string_clist2_table [i].xml_path_field_2))
			{
				if ((s = xml_element_get_content (nodesub)))
				{
					if (!entry[2])
						entry[2] = s;
					else
					{
						entry[2] = g_strjoin (" ", entry[2], s, NULL);
					}
				}
			}

			if (!entry[2])
				continue;

			clist = tool_widget_get (transfer_string_clist2_table [i].clist);

			row = gtk_clist_append (GTK_CLIST (clist), entry);

			set_clist_checkmark (GTK_CLIST (clist), row, 0, 
					     xml_element_get_bool_attr (node,"enabled"));

			g_free (entry[1]);
			g_free (entry[2]);
		}
	}
}


static void
transfer_string_clist2_gui_to_xml (xmlNodePtr root)
{
	int i, j, row;
	int col0_added;
	gchar *col0, *col1;
	gchar **col1_elem;
	xmlNodePtr node, node2;
	GtkWidget *w;

	for (i = 0; transfer_string_clist2_table [i].xml_path; i++)
	{
		/* First remove any old branches in the XML tree */

		xml_element_destroy_children_by_name (root, transfer_string_clist2_table [i].xml_path);

		/* Add branches corresponding to listed data */

		w = tool_widget_get (transfer_string_clist2_table[i].clist);
		for (row = 0; gtk_clist_get_text (GTK_CLIST (w), row, 1, &col0); row++)
		{
			if (!gtk_clist_get_text (GTK_CLIST (w), row, 2, &col1))
				continue;

			if (!strlen (col1))
				continue;

			/* Enclosing element */
			node = xml_element_add (root, transfer_string_clist2_table [i].xml_path);

			xml_element_set_bool_attr (node, "enabled",
						   get_clist_checkmark (GTK_CLIST (w), row, 0));

			d(g_print ("name: (%d) %s\n", xml_element_get_bool_attr (node, "enabled"), node->name));

			col1_elem = g_strsplit (col1, " ", 0);

			for (j = 0, col0_added = 0; col1_elem[j]; j++)
			{
				if (!strlen (col1_elem[j]))
					continue;
				if (!col0_added)
				{
					node2 = xml_element_add (node, transfer_string_clist2_table [i].xml_path_field_1);
					xml_element_set_content (node2, col0);
				}
				node2 = xml_element_add (node, transfer_string_clist2_table [i].xml_path_field_2);
				xml_element_set_content (node2, col1_elem[j]);
			}

			g_strfreev (col1_elem);
		}
	}
}


void
transfer_xml_to_gui (xmlNodePtr root)
{
	transfer_string_entry_xml_to_gui (root);
	transfer_string_list_xml_to_gui (root);
	transfer_string_clist2_xml_to_gui (root);
}


void
transfer_gui_to_xml (xmlNodePtr root)
{
	transfer_string_entry_gui_to_xml (root);
	transfer_string_list_gui_to_xml (root);
	transfer_string_clist2_gui_to_xml (root);
}
