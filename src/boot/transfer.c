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
#include <gnome-xml/tree.h>
#include <gnome-xml/parser.h>
#include <glade/glade.h>
#include "global.h"

#include "transfer.h"
#include "callbacks.h"
#include "e-table.h"

extern XstTool *tool;


static void
transfer_globals_xml_to_gui (xmlNodePtr root)
{
	GtkWidget *spin;
	xmlNodePtr node;
	gchar *buf;
	gint value;

	node = xml_element_find_first (root, "prompt");

	if (node)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xst_dialog_get_widget
											    (tool->main_dialog, "boot_prompt")),
								TRUE);

		spin = xst_dialog_get_widget (tool->main_dialog, "boot_timeout");
		gtk_widget_set_sensitive (spin, TRUE);

		buf = xml_get_child_content (root, "timeout");
		if (buf)
		{
			value = atoi (buf);
			g_free (buf);
		}
		else
			value = 50;
		
		/* Set value in seconds. */
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), (gfloat) value / 10);
	}
}

void
transfer_xml_to_gui (XstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = xml_doc_get_root (tool->config);
	create_table (root);

	transfer_globals_xml_to_gui (root);
}

void
transfer_gui_to_xml (XstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = xml_doc_get_root (tool->config);
}


