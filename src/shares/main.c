/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of shares-admin, a gnome-system-tool frontend 
 * for shared folders administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "gst.h"
#include "table.h"
#include "nfs-acl-table.h"
#include "transfer.h"
#include "share-export-smb.h"
#include "share-settings.h"
#include "share-nfs-add-hosts.h"
#include "callbacks.h"

GstTool *tool;
GtkIconTheme *icon_theme;

static GstDialogSignal signals [] = {
	/* Main dialog */
	{ "add_share",                "clicked",      G_CALLBACK (on_add_share_clicked) },
	{ "edit_share",               "clicked",      G_CALLBACK (on_edit_share_clicked) },
	{ "delete_share",             "clicked",      G_CALLBACK (on_delete_share_clicked) },
	/* Shares dialog */
	{ "share_type",               "changed",      G_CALLBACK (on_share_type_changed) },
	{ "share_nfs_delete",         "clicked",      G_CALLBACK (on_share_nfs_delete_clicked) },
	{ "share_nfs_add",            "clicked",      G_CALLBACK (on_share_nfs_add_clicked) },
	/* NFS add hosts dialog */
	{ "share_nfs_host_type",      "changed",      G_CALLBACK (on_share_nfs_host_type_changed) },
	{ NULL }
};

void
initialize_tables (void)
{
	table_create ();
	nfs_acl_table_create ();
	share_nfs_add_hosts_dialog_setup ();
	share_settings_create_combo ();
}

void
init_standalone_dialog (const gchar *path)
{
	gst_tool_main_with_hidden_dialog (tool, TRUE);
	share_settings_dialog_run (path, TRUE);
}

int
main (int argc, char *argv[])
{
	gchar *path = NULL;
	
	struct poptOption options[] = {
		{ "add-share", '\0', POPT_ARG_STRING, &path, 0, _("Add a shared path, modifies it if it already exists"), _("Path") },
		{ NULL, '\0', 0, NULL, 0 }
	};
	
	gst_init ("shares-admin", argc, argv, options);

	tool = gst_tool_new ();
	gst_tool_construct (tool, "shares", _("Shared folders settings"));
	gst_tool_set_xml_funcs (tool, transfer_xml_to_gui, transfer_gui_to_xml, NULL);
	gst_dialog_connect_signals (tool->main_dialog, signals);

	initialize_tables ();

	if (path)
		init_standalone_dialog (path);
	else
		gst_tool_main (tool, FALSE);

	return 0;
}
