/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of boot-admin, a ximian-setup-tool frontend 
 * for boot administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Tambet Ingo <tambet@ximian.com>.
 */

/* Functions for transferring information between XML tree and UI */


#include <gnome.h>
#include <glade/glade.h>
#include "xst.h"

#include "transfer.h"
#include "callbacks.h"
#include "table.h"

extern XstTool *tool;


static void
transfer_check_default (xmlNodePtr root)
{
	xmlNodePtr node, def_node;
	gchar *buf1, *buf2;
	gboolean found;

	def_node = xst_xml_element_find_first (root, "default");
	if (def_node)
	{
		found = FALSE;
		buf1 = xst_xml_element_get_content (def_node);

		for (node = xst_xml_element_find_first (root, "entry");
			node;
			node = xst_xml_element_find_next (node, "entry"))

		{
			buf2 = xst_xml_get_child_content (node, "label");

			if (!buf2)
				continue;
			
			if (!strcmp (buf1, buf2))
			{
				g_free (buf2);
				found = TRUE;
				break;
			}

			g_free (buf2);
		}

		g_free (buf1);
		
		if (!found)
			xst_xml_element_destroy (def_node);
	}
}

static void
transfer_check_data (xmlNodePtr root)
{
	g_return_if_fail (root != NULL);

	transfer_check_default (root);
}

static void
transfer_globals_xml_to_gui (xmlNodePtr root)
{
	gchar *buf;
	gint value;

	buf = xst_xml_get_child_content (root, "prompt");

	if (buf) {
		if (*buf == '1') {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (xst_dialog_get_widget
						       (tool->main_dialog, "boot_prompt")),
						      TRUE);
			
			gtk_widget_set_sensitive (xst_dialog_get_widget
						  (tool->main_dialog, "boot_timeout"),
						  TRUE);
			
			gtk_widget_set_sensitive (xst_dialog_get_widget
						  (tool->main_dialog,
						   "boot_timeout_label"),
						  TRUE);
		}

		g_free (buf);
	}

	buf = xst_xml_get_child_content (root, "timeout");
	if (buf) {
		value = atoi (buf);
		g_free (buf);
	} else
		value = 50;
	
	/* Set value in seconds. */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (xst_dialog_get_widget (tool->main_dialog,
									   "boot_timeout")),
				   (gfloat) value / 10);
}

static void
transfer_globals_gui_to_xml (xmlNodePtr root)
{
	xmlNodePtr node;
	gint val;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xst_dialog_get_widget
							     (tool->main_dialog, "boot_prompt")))) {
		node = xst_xml_element_find_first (root, "prompt");

		if (!node)
			xst_xml_element_add (root, "prompt");

		val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
							(xst_dialog_get_widget (tool->main_dialog,
										"boot_timeout")));

		node = xst_xml_element_find_first (root, "timeout");
		if (!node)
			node = xst_xml_element_add (root, "timeout");

		/* We need timeout in tenths of seconds, so multiply by 10 */
		xst_xml_element_set_content (node, g_strdup_printf ("%d", val * 10));
	} else {
		node = xst_xml_element_find_first (root, "prompt");

		if (node)
			xst_xml_element_destroy (node);
	}
}

void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = xst_xml_doc_get_root (tool->config);

	transfer_globals_xml_to_gui (root);
	transfer_check_data (root);

	table_populate (root);
}

void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = xst_xml_doc_get_root (tool->config);

	transfer_globals_gui_to_xml (root);
	transfer_check_data (root);
}
