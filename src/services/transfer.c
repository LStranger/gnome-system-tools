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
transfer_populate_menu (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr    runlevels, runlevel;
	GstWidget    *menu;
	GtkTreeModel *model;
	gchar        *name, *desc, *first_runlevel;
	gboolean      is_default, has_default;
	gint          n_items, n_option;
	
	runlevels = gst_xml_element_find_first (root, "runlevels");
	menu      = gst_dialog_get_gst_widget (tool->main_dialog, "runlevels_menu");
	model     = gtk_combo_box_get_model (GTK_COMBO_BOX (menu->widget));
	n_items   = 0;

	gtk_list_store_clear (GTK_LIST_STORE (model));

	for (runlevel = gst_xml_element_find_first (runlevels, "runlevel");
	     runlevel != NULL;
	     runlevel = gst_xml_element_find_next (runlevel, "runlevel"))
	{
		name = gst_xml_get_child_content (runlevel, "number");
		desc = gst_xml_get_child_content (runlevel, "description");
		is_default = gst_xml_element_get_boolean (runlevel, "default");

		/* we save the first runlevel, just if there is no default runlevel */
		if (n_items == 0)
			first_runlevel = name;

		gtk_combo_box_append_text (GTK_COMBO_BOX (menu->widget), desc);

		if (is_default) {
			has_default = TRUE;
			n_option = n_items;
			g_object_set_data (G_OBJECT (menu->widget), "default_runlevel", name);
		}

		n_items++;
	}

	if (n_items == 1) {
		/* it's has only one runlevel, we should hide the runlevels list at all */
		menu->advanced = GST_WIDGET_MODE_HIDDEN;
		gst_widget_apply_policy (menu);
	}

	if (has_default) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (menu->widget), n_option);
		g_object_set_data (G_OBJECT (menu->widget), "default_item", GINT_TO_POINTER (n_option));
	} else {
		/* there isn't a default, so we load the first runlevel in the list */
		gtk_combo_box_set_active (GTK_COMBO_BOX (menu->widget), 0);
	}
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	root = gst_xml_doc_get_root(tool->config);
	g_return_if_fail (root != NULL);

	transfer_populate_menu (tool, root);
	hide_sequence_ordering_toggle_button (root);
}
