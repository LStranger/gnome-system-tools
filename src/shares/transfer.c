/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of shares-admin, a gnome-system-tool frontend 
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
 * Authors: Carlos Garnacho <carlosg@gnome.org>.
 */

#include "gst.h"
#include "share-export.h"
#include "table.h"

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root, export;

	root   = gst_xml_doc_get_root (tool->config);
	export = gst_xml_element_find_first (root, "exports");

	if (!export)
		return;

	for (export = gst_xml_element_find_first (export, "export");
	     export;
	     export = gst_xml_element_find_next (export, "export")) {
		table_add_share_from_node (export);
	}
}

void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
	xmlNodePtr    root, export;
	GtkWidget    *table;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid;
	GstShare     *share;

	table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	root   = gst_xml_doc_get_root (tool->config);
	export = gst_xml_element_find_first (root, "exports");

	if (!export)
		export = gst_xml_element_add (root, "exports");

	gst_xml_element_destroy_children (export);

	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter,
				    COL_POINTER, &share,
				    -1);
		gst_share_get_xml (share, export);

		g_object_unref (share);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}