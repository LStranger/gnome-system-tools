/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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

#include <gtk/gtk.h>
#include "gst.h"
#include "gst-network-tool.h"
#include "ifaces-list.h"
#include "connection.h"
#include "hosts.h"
#include "location.h"

static void gst_network_tool_class_init (GstNetworkToolClass *class);
static void gst_network_tool_init       (GstNetworkTool      *tool);
static void gst_network_tool_finalize   (GObject             *object);

static gpointer parent_class;

GType
gst_network_tool_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo tool_info =
        {
	  sizeof (GstNetworkToolClass),
	  NULL,		/* base_init */
	  NULL,		/* base_finalize */
	  (GClassInitFunc) gst_network_tool_class_init,
	  NULL,		/* class_finalize */
	  NULL,		/* class_data */
	  sizeof (GstNetworkTool),
	  0,		/* n_preallocs */
	  (GInstanceInitFunc) gst_network_tool_init,
	};

      type = g_type_register_static (GST_TYPE_TOOL, "GstNetworkTool",
				     &tool_info, 0);
    }

  return type;
}

static void
gst_network_tool_init (GstNetworkTool *tool)
{
  g_return_if_fail (GST_IS_NETWORK_TOOL (tool));

  tool->dns = NULL;
  tool->search = NULL;
  tool->interfaces_model = NULL;
  tool->gateways_model = NULL;
  tool->interfaces_list = NULL;
  tool->gateways_list = NULL;
  tool->hostname = NULL;
  tool->domain = NULL;
  tool->dialog = NULL;
  tool->host_aliases_list = NULL;
  tool->icon_theme = gtk_icon_theme_get_default ();
}

static void
gst_network_tool_class_init (GstNetworkToolClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->finalize = gst_network_tool_finalize;
}

static void
gst_network_tool_finalize (GObject *object)
{
  GstNetworkTool *tool;

  g_return_if_fail (GST_IS_NETWORK_TOOL (object));

  tool = GST_NETWORK_TOOL (object);

  g_object_unref (tool->dns);
  g_object_unref (tool->search);
  g_object_unref (tool->interfaces_model);
  g_free (tool->dialog);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

GstTool*
gst_network_tool_new (void)
{
  GstNetworkTool *tool = g_object_new (GST_TYPE_NETWORK_TOOL, NULL);

  return GST_TOOL (tool);
}

void
gst_network_tool_construct (GstNetworkTool *tool,
			    const gchar    *name,
			    const gchar    *title)
{
  GtkWidget *widget, *add_button, *delete_button;
  
  gst_tool_construct (GST_TOOL (tool), name, title);

  widget        = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "dns_list");
  add_button    = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "dns_list_add");
  delete_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "dns_list_delete");
  tool->dns = gst_address_list_new (GTK_TREE_VIEW (widget),
				    GTK_BUTTON (add_button),
				    GTK_BUTTON (delete_button),
				    GST_ADDRESS_TYPE_IP);

  widget        = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "search_domain_list");
  add_button    = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "search_domain_add");
  delete_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "search_domain_delete");
  tool->search = gst_address_list_new (GTK_TREE_VIEW (widget),
				       GTK_BUTTON (add_button),
				       GTK_BUTTON (delete_button),
				       GST_ADDRESS_TYPE_DOMAIN);

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "hostname");
  tool->hostname = GTK_ENTRY (widget);

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "domain");
  tool->domain = GTK_ENTRY (widget);

  tool->interfaces_model = ifaces_model_create ();
  tool->gateways_model   = gateways_filter_model_create (tool->interfaces_model);
  tool->interfaces_list  = ifaces_list_create ();
  tool->gateways_list    = gateways_combo_create ();

  tool->host_aliases_list = host_aliases_list_create ();

  tool->location = gst_location_combo_new ();
  gtk_widget_show (GTK_WIDGET (tool->location));
  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "locations_box");
  gtk_box_pack_start_defaults (GTK_BOX (widget), GTK_WIDGET (tool->location));

  tool->dialog = connection_dialog_init ();
}
