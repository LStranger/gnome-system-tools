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
transfer_populate_option_menu (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr runlevels = gst_xml_element_find_first (root, "runlevels");
	xmlNodePtr runlevel;
	GtkWidget *menu_item, *menu_shell;
	GstWidget *option_menu;
	gchar *first_runlevel, *str;
	gboolean has_default = FALSE;
	gint n_option, n_items = 0;
	
	option_menu = gst_dialog_get_gst_widget (tool->main_dialog, "runlevels_menu");
	gtk_option_menu_remove_menu (GTK_OPTION_MENU (option_menu->widget));
	
	menu_shell = gtk_menu_new ();

	for (runlevel = gst_xml_element_find_first (runlevels, "runlevel");
	     runlevel != NULL;
	     runlevel = gst_xml_element_find_next (runlevel, "runlevel"))
	{
		str = gst_xml_get_child_content (runlevel, "number");
		
		/* we save the first runlevel, just if there is no default runlevel */
		if (n_items == 0)
			first_runlevel = str;
		
		menu_item = gtk_menu_item_new_with_label (gst_xml_get_child_content (runlevel,
										     "description"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu_shell), menu_item);

		g_signal_connect (G_OBJECT (menu_item), "activate",
				  G_CALLBACK (on_runlevel_changed), str);

		g_object_set_data (G_OBJECT (menu_item), "runlevel", str);

		if (gst_xml_get_child_content (runlevel, "default") != NULL) {
			/* It's the default runlevel */
			has_default = TRUE;
			n_option = n_items;
			g_object_set_data (G_OBJECT (option_menu->widget), "default_runlevel", str);
			g_object_set_data (G_OBJECT (menu_item), "default", GINT_TO_POINTER (TRUE));
			change_runlevel (str);
		}

		n_items++;
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu->widget), menu_shell);

	if (n_items == 1) {
		/* it's has only one runlevel, we should hide the runlevels list at all */
		option_menu->advanced = GST_WIDGET_MODE_HIDDEN;
		gst_widget_apply_policy (option_menu);
	} else {
		gtk_widget_show_all (menu_shell);
	}

	if (has_default) {
		gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu->widget), n_option);
		g_object_set_data (G_OBJECT (option_menu->widget), "default_item", GINT_TO_POINTER (n_option));
	} else {
		/* there isn't a default, so we load the first runlevel in the list */
		gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu->widget), 0);
		change_runlevel (first_runlevel);
	}
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	root = gst_xml_doc_get_root(tool->config);
	g_return_if_fail (root != NULL);

	transfer_populate_option_menu (tool, root);
	hide_sequence_ordering_toggle_button (root);
}
