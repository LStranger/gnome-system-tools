/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of services-admin, a gnome-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho <garparr@teleline.es>.
 */

#include <gtk/gtk.h>
#include "gst.h"

#include "transfer.h"
#include "callbacks.h"
#include "table.h"

extern GstTool *tool;

static void
transfer_get_default_runlevel (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr runlevels, rl;
	gchar *default_runlevel = NULL;
	gboolean is_default;
	gint n_items;

	n_items = 0;
	runlevels = gst_xml_element_find_first (root, "runlevels");

	for (rl = gst_xml_element_find_first (runlevels, "runlevel");
	     rl; rl = gst_xml_element_find_next (rl, "runlevel")) {
		is_default = gst_xml_element_get_boolean (rl, "default");

		if (is_default || n_items == 0) {
			g_free (default_runlevel);
			default_runlevel = gst_xml_get_child_content (rl, "name");
		}

		n_items++;
	}

	g_object_set_data_full (G_OBJECT (tool), "default_runlevel", default_runlevel, g_free);
}

static void
transfer_populate_main_window (GstTool *tool, xmlNodePtr root)
{
	gchar *default_runlevel;

	default_runlevel = g_object_get_data (G_OBJECT (tool), "default_runlevel");

	if (default_runlevel)
		table_populate (root, default_runlevel);
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	root = gst_xml_doc_get_root(tool->config);

	g_return_if_fail (root != NULL);

	transfer_get_default_runlevel (tool, root);
	transfer_populate_main_window (tool, root);
}
