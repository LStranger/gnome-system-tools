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

#ifndef __GST_NETWORK_TOOL_H
#define __GST_NETWORK_TOOL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gtk/gtk.h>
#include "address-list.h"
#include "network-iface.h"
#include "network-iface-ethernet.h"
#include "network-iface-wireless.h"
#include "network-iface-plip.h"
#include "network-iface-irlan.h"
#include "network-iface-modem.h"
#include "connection.h"
#include "location.h"

#define GST_TYPE_NETWORK_TOOL           (gst_network_tool_get_type ())
#define GST_NETWORK_TOOL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_NETWORK_TOOL, GstNetworkTool))
#define GST_NETWORK_TOOL_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj),    GST_TYPE_NETWORK_TOOL, GstNetworkToolClass))
#define GST_IS_NETWORK_TOOL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_NETWORK_TOOL))
#define GST_IS_NETWORK_TOOL_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj),    GST_TYPE_NETWORK_TOOL))
#define GST_NETWORK_TOOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GST_TYPE_NETWORK_TOOL, GstNetworkToolClass))

typedef struct _GstNetworkTool      GstNetworkTool;
typedef struct _GstNetworkToolClass GstNetworkToolClass;

struct _GstNetworkTool
{
  GstTool parent_instance;

  GstAddressList *dns;
  GstAddressList *search;

  GtkTreeModel *interfaces_model;
  GtkTreeModelFilter *gateways_model;
  GtkTreeView  *interfaces_list;
  GtkComboBox  *gateways_list;

  GtkTreeView *host_aliases_list;
  GstLocation *location;

  GtkEntry *hostname;
  GtkEntry *domain;

  GstConnectionDialog *dialog;
  GtkIconTheme *icon_theme;
};

struct _GstNetworkToolClass
{
  GstToolClass parent_class;
};


GType    gst_network_tool_get_type  (void);
GstTool *gst_network_tool_new       (void);
void     gst_network_tool_construct (GstNetworkTool*, const gchar*, const gchar*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GST_NETWORK_TOOL_H */
