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

#include <gnome.h>
#include <glade/glade.h>
#include "gst.h"

#include "transfer.h"
#include "callbacks.h"
#include "table.h"

extern GstTool *tool;

static void
transfer_populate_option_menu (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr runlevels = gst_xml_element_find_first (root, "runlevels");
	xmlNodePtr runlevel;
	GtkWidget *menu_item, *menu_shell, *option_menu;
	gint i, n_item, n_option;

	n_item = 0;
	option_menu = gst_dialog_get_widget (tool->main_dialog, "runlevels_menu");
	gtk_option_menu_remove_menu (GTK_OPTION_MENU (option_menu));
	
	menu_shell = gtk_menu_new ();

	for (runlevel = gst_xml_element_find_first (runlevels, "runlevel");
	     runlevel != NULL;
	     runlevel = gst_xml_element_find_next (runlevel, "runlevel"))
	{
		i = atoi (gst_xml_get_child_content (runlevel, "number"));
		menu_item = gtk_menu_item_new_with_label (gst_xml_get_child_content (runlevel,
										     "description"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu_shell), menu_item);

		g_signal_connect (G_OBJECT (menu_item), "activate",
				  G_CALLBACK (on_runlevel_changed), GINT_TO_POINTER (i));

		g_object_set_data (G_OBJECT (menu_item), "runlevel", GINT_TO_POINTER (i));

		if (gst_xml_get_child_content (runlevel, "default") != NULL) {
			/* It's the default runlevel */
			n_option = n_item;
			g_signal_emit_by_name (G_OBJECT (menu_item), "activate", GINT_TO_POINTER (i));
			g_object_set_data (G_OBJECT (menu_item), "default", GINT_TO_POINTER (TRUE));
		}

		n_item++;
	}

	gtk_widget_show_all (menu_shell);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu_shell);

	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), n_option);
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	root = gst_xml_doc_get_root(tool->config);
	g_return_if_fail (root != NULL);

	transfer_populate_option_menu (tool, root);
}
