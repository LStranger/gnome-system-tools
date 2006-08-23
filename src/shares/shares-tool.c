/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#include <glib-object.h>
#include "shares-tool.h"
#include <glib/gi18n.h>

static void gst_shares_tool_class_init (GstSharesToolClass *class);
static void gst_shares_tool_init       (GstSharesTool      *tool);
static void gst_shares_tool_finalize   (GObject            *object);

static void gst_shares_tool_update_gui    (GstTool         *tool);
static void gst_shares_tool_update_config (GstTool         *tool);

static void gst_shares_tool_update_services_availability (GstSharesTool *tool);

G_DEFINE_TYPE (GstSharesTool, gst_shares_tool, GST_TYPE_TOOL);

static void
gst_shares_tool_class_init (GstSharesToolClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GstToolClass *tool_class = GST_TOOL_CLASS (class);

	object_class->finalize = gst_shares_tool_finalize;
	tool_class->update_gui = gst_shares_tool_update_gui;
	tool_class->update_config = gst_shares_tool_update_config;
}

static void
gst_shares_tool_init (GstSharesTool *tool)
{
	GstTool *gst_tool = GST_TOOL (tool);

	tool->nfs_config = oobs_nfs_config_get (gst_tool->session);
	tool->smb_config = oobs_smb_config_get (gst_tool->session);

	tool->services_config = oobs_services_config_get (gst_tool->session);
	tool->hosts_config = oobs_hosts_config_get (gst_tool->session);
	tool->ifaces_config = oobs_ifaces_config_get (gst_tool->session);

	gst_shares_tool_update_services_availability (tool);
}

static void
gst_shares_tool_finalize (GObject *object)
{
	GstSharesTool *tool = GST_SHARES_TOOL (object);

	if (tool->nfs_config)
		g_object_unref (tool->nfs_config);

	(* G_OBJECT_CLASS (gst_shares_tool_parent_class)->finalize) (object);
}

static void
add_shares (OobsList *list)
{
	OobsListIter iter;
	OobsShare *share;
	gboolean valid;

	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		share = OOBS_SHARE (oobs_list_get (list, &iter));

		table_add_share (share, &iter);
		g_object_unref (share);
		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
update_global_smb_config (GstTool       *tool,
			  OobsSMBConfig *config)
{
	GtkWidget *widget;
	gchar *str;
	gboolean is_wins_server;

	str = oobs_smb_config_get_workgroup (config);
	widget = gst_dialog_get_widget (tool->main_dialog, "smb_workgroup");
	gtk_entry_set_text (GTK_ENTRY (widget), (str) ? str : "");

	is_wins_server = oobs_smb_config_get_is_wins_server (config);
	widget = gst_dialog_get_widget (tool->main_dialog, "smb_is_wins");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), is_wins_server);
		
	str = oobs_smb_config_get_wins_server (config);
	widget = gst_dialog_get_widget (tool->main_dialog, "smb_wins_server");
	gtk_entry_set_text (GTK_ENTRY (widget), (str) ? str : "");
}

static void
gst_shares_tool_update_gui (GstTool *tool)
{
	GstSharesTool *shares_tool;
	OobsList *list;

	shares_tool = GST_SHARES_TOOL (tool);

	list  = oobs_nfs_config_get_shares (OOBS_NFS_CONFIG (shares_tool->nfs_config));
	add_shares (list);

	list = oobs_smb_config_get_shares (OOBS_SMB_CONFIG (shares_tool->smb_config));
	add_shares (list);

	update_global_smb_config (tool, shares_tool->smb_config);
}

static void
gst_shares_tool_update_config (GstTool *tool)
{
	GstSharesTool *shares_tool;

	shares_tool = GST_SHARES_TOOL (tool);
	oobs_object_update (shares_tool->nfs_config);
	oobs_object_update (shares_tool->smb_config);
	oobs_object_update (shares_tool->services_config);
	oobs_object_update (shares_tool->hosts_config);
	oobs_object_update (shares_tool->ifaces_config);
	gst_shares_tool_update_services_availability (tool);
}

static void
gst_shares_tool_update_services_availability (GstSharesTool *tool)
{
	OobsList *services;
	OobsListIter iter;
	GObject *service;
	gboolean valid;
	gchar *role;

	services = oobs_services_config_get_services (tool->services_config);
	valid = oobs_list_get_iter_first (services, &iter);

	while (valid) {
		service = oobs_list_get (services, &iter);
		role = oobs_service_get_role (OOBS_SERVICE (service));

		if (strcmp (role, "FILE_SERVER_SMB") == 0)
			tool->smb_available = TRUE;
		else if (strcmp (role, "FILE_SERVER_NFS") == 0)
			tool->nfs_available = TRUE;

		g_object_unref (service);
		valid = oobs_list_iter_next (services, &iter);
	}
}

GstSharesTool*
gst_shares_tool_new (void)
{
	return g_object_new (GST_TYPE_SHARES_TOOL,
			     "name", "shares",
			     "title", _("Shared Folders"),
			     "icon", "gnome-fs-share",
			     NULL);
}
