/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho Parro.
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
#include "address-list.h"
#include "gst-network-tool.h"
#include "ifaces-list.h"
#include "hosts.h"
#include "location.h"

extern GstTool *tool;

static void
transfer_address_list_to_gui (xmlNodePtr root, gchar *tag, GstAddressList *al)
{
  xmlNodePtr  node;
  gchar      *address;

  gst_address_list_clear (al);

  for (node = gst_xml_element_find_first (root, tag);
       node; node = gst_xml_element_find_next (node, tag))
    {
      address = gst_xml_element_get_content (node);
      gst_address_list_add_address (al, address);
    }
}

static void
transfer_address_list_to_xml (xmlNodePtr root, gchar *tag, GstAddressList *al)
{
  xmlNodePtr  node;
  GSList     *list, *l;

  list = gst_address_list_get_list (al);
  l = list;

  gst_xml_element_destroy_children_by_name (root, tag);

  while (l)
    {
      node = gst_xml_element_add (root, tag);
      gst_xml_element_set_content (node, l->data);
      l = l->next;
    }

  g_slist_foreach (list, (GFunc) g_free, NULL);
  g_slist_free (list);
}

static void
transfer_string_to_gui (xmlNodePtr root, gchar *tag, GtkEntry *entry)
{
  gchar *text;

  if (text = gst_xml_get_child_content (root, tag))
    {
      gtk_entry_set_text (entry, text);
      g_free (text);
    }
}

static void
transfer_string_to_xml (xmlNodePtr root, gchar *tag, GtkEntry *entry)
{
  gst_xml_set_child_content (root, tag, gtk_entry_get_text (entry));
}

static void
transfer_gateway_to_combo (xmlNodePtr root)
{
  gchar *text;

  if (text = gst_xml_get_child_content (root, "gatewaydev"))
    {
      gateways_combo_select (text);
      g_free (text);
    }
}

static void
transfer_gateway_to_xml (xmlNodePtr root)
{
  gchar      *gatewaydev = gateways_combo_get_selected ();
  xmlNodePtr  node       = gst_xml_element_find_first (root, "gatewaydev");

  if (!gatewaydev)
    gst_xml_element_destroy (node);
  else
    gst_xml_element_set_content (node, gatewaydev);

  g_free (gatewaydev);
}

static void
transfer_interfaces_to_gui (xmlNodePtr root)
{
  xmlNodePtr node;

  ifaces_model_clear ();

  for (node = gst_xml_element_find_first (root, "interface");
       node; node = gst_xml_element_find_next (node, "interface"))
    ifaces_model_add_interface_from_node (node);
}

static void
purge_interfaces (xmlNodePtr root)
{
  xmlNodePtr  node, node_next;
  gchar      *type;

  /* destroy all but lo */
  for (node = gst_xml_element_find_first (root, "interface"); node;)
    {
      node_next = gst_xml_element_find_next (node, "interface");
      type = gst_xml_element_get_attribute (node, "type");

      if (!type || (type && (strcmp (type, "loopback") != 0)))
        gst_xml_element_destroy (node);

      node = node_next;
    }
}

static void
transfer_interfaces_to_xml (xmlNodePtr root)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      valid;
  GstIface     *iface;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  valid = gtk_tree_model_get_iter_first (model, &iter);
  purge_interfaces (root);

  while (valid)
    {
      gtk_tree_model_get (model, &iter, COL_OBJECT, &iface, -1);
      gst_iface_get_xml (iface, root);
      g_object_unref (iface);

      valid = gtk_tree_model_iter_next (model, &iter);
    }
}

static void
transfer_host_aliases_to_gui (xmlNodePtr root)
{
  xmlNodePtr  node;

  host_aliases_clear ();

  for (node = gst_xml_element_find_first (root, "statichost");
       node; node = gst_xml_element_find_next (node, "statichost"))
    host_aliases_add_from_xml (node);
}

static void
transfer_host_aliases_to_xml (xmlNodePtr root)
{
  GtkTreeView  *list;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      valid;

  list  = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);
  valid = gtk_tree_model_get_iter_first (model, &iter);
  gst_xml_element_destroy_children_by_name (root, "statichost");

  while (valid)
    {
      host_aliases_extract_to_xml (&iter, root);
      valid = gtk_tree_model_iter_next (model, &iter);
    }
}

void
transfer_xml_profile_to_gui (GstTool *tool, xmlNodePtr root)
{
  GstNetworkTool *network_tool;

  network_tool = GST_NETWORK_TOOL (tool);

  transfer_address_list_to_gui (root, "nameserver", network_tool->dns);
  transfer_address_list_to_gui (root, "searchdomain", network_tool->search);

  transfer_string_to_gui (root, "hostname", network_tool->hostname);
  transfer_string_to_gui (root, "domain", network_tool->domain);

  transfer_host_aliases_to_gui (root);
  transfer_gateway_to_combo (root);
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
  xmlNodePtr root;
  GstNetworkTool *network_tool;

  network_tool = GST_NETWORK_TOOL (tool);
  root = gst_xml_doc_get_root (tool->config);

  transfer_interfaces_to_gui (root);
  gst_location_combo_setup (network_tool->location, root);

  transfer_xml_profile_to_gui (tool, root);
  gst_dialog_set_modified (tool->main_dialog, FALSE);
}

void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
  GstNetworkTool *network_tool;
  xmlNode *root;

  network_tool = GST_NETWORK_TOOL (tool);
  root = gst_xml_doc_get_root (tool->config);

  transfer_address_list_to_xml (root, "nameserver", network_tool->dns);
  transfer_address_list_to_xml (root, "searchdomain", network_tool->search);

  transfer_string_to_xml (root, "hostname", network_tool->hostname);
  transfer_string_to_xml (root, "domain", network_tool->domain);

  transfer_interfaces_to_xml (root);
  transfer_gateway_to_xml (root);

  transfer_host_aliases_to_xml (root);
}
