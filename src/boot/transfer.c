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
#include <gconf/gconf-client.h>
#include "gst.h"

#include "transfer.h"
#include "callbacks.h"
#include "table.h"

extern GstTool *tool;


static void
transfer_check_default (xmlNodePtr root)
{
	xmlNodePtr node, def_node;
	gchar *buf1, *buf2;
	gboolean found;

	def_node = gst_xml_element_find_first (root, "default");
	if (def_node)
	{
		found = FALSE;
		buf1 = gst_xml_element_get_content (def_node);

		for (node = gst_xml_element_find_first (root, "entry");
			node;
			node = gst_xml_element_find_next (node, "entry"))

		{
			buf2 = gst_xml_get_child_content (node, "label");

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
			gst_xml_element_destroy (def_node);
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
	GtkWidget *sb;
	gchar     *buf;
	gint       value;

	buf = gst_xml_get_child_content (root, "timeout");
	sb = gst_dialog_get_widget (tool->main_dialog, "boot_timeout");
	
	if (buf) {
		value = atoi (buf);
		g_free (buf);
	} else
		value = 0;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (sb), (gfloat) value / 10);
}

static void
transfer_globals_gui_to_xml (xmlNodePtr root)
{
	xmlNodePtr node;
	gint val;

	node = gst_xml_element_find_first (root, "prompt");

	if (!node)
		gst_xml_element_add (root, "prompt");

	val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON
						(gst_dialog_get_widget (tool->main_dialog,
									"boot_timeout")));

	node = gst_xml_element_find_first (root, "timeout");
	if (!node)
		node = gst_xml_element_add (root, "timeout");

	/* We need timeout in tenths of seconds, so multiply by 10 */
	gst_xml_element_set_content (node, g_strdup_printf ("%d", val * 10));
}

/* checks that there is a preferred bootloader set in gconf */
static gchar*
transfer_check_preferred_bootloader (xmlNodePtr bootloaders)
{
	GConfClient *client = gconf_client_get_default ();
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	xmlNodePtr node;
	gchar *exec = NULL;
	gchar *key, *preferred_bootloader, *name;

	key = g_strjoin ("/", GST_GCONF_ROOT, "boot", "preferred_bootloader", NULL);
	preferred_bootloader = gconf_client_get_string (client, key, NULL);

	/* first of all, check if the gconf key contains an element in the list and use it */
	if (preferred_bootloader) {
		for (node = gst_xml_element_find_first (bootloaders, "bootloader");
		     node;
		     node = gst_xml_element_find_next (node, "bootloader"))
		{
			name = gst_xml_get_child_content (node, "name");

			if (strcasecmp (preferred_bootloader, name) == 0)
				exec = gst_xml_get_child_content (node, "exec");
		}
	}

	g_free (preferred_bootloader);
	g_free (key);

	return exec;
}

/* shows the dialog and let the user choose the bootloader */
static gchar*
transfer_get_selected_bootloader (xmlNodePtr bootloaders)
{
	GConfClient *client = gconf_client_get_default ();
	GtkWidget *alignment = gst_dialog_get_widget (tool->main_dialog, "bootloaders_list");
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "several_bootloaders_dialog");
	GtkWidget *save_bootloader = gst_dialog_get_widget (tool->main_dialog, "save_bootloader");
	GtkWidget *box = gtk_vbox_new (TRUE, 6);
	GtkWidget *checkbox;
	GSList *l, *list = NULL;
	xmlNodePtr node;
	gchar *name, *key = NULL, *exec = NULL;
	
	for (node = gst_xml_element_find_first (bootloaders, "bootloader");
	     node;
	     node = gst_xml_element_find_next (node, "bootloader"))
	{
		name = gst_xml_get_child_content (node, "name");
		exec = gst_xml_get_child_content (node, "exec");

		checkbox = gtk_radio_button_new_with_label (list, name);
		g_object_set_data (G_OBJECT (checkbox), "name", name);
		g_object_set_data (G_OBJECT (checkbox), "exec", exec);
		list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (checkbox));
		
		gtk_box_pack_start (GTK_BOX (box), checkbox, FALSE, FALSE, 0);
	}

	gtk_widget_show_all (box);
	gtk_container_add (GTK_CONTAINER (alignment), box);
	gtk_dialog_run (GTK_DIALOG (dialog));

	/* get the selected tool */
	l = list;
	exec = NULL;
	name = NULL;

	while (l && !exec) {
		checkbox = l->data;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox))) {
			name = g_object_get_data (G_OBJECT (checkbox), "name");
			exec = g_object_get_data (G_OBJECT (checkbox), "exec");
		}

		l = l->next;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_bootloader))) {
		key = g_strjoin ("/", GST_GCONF_ROOT, "boot", "preferred_bootloader", NULL);
		gconf_client_set_string (client, key, name, NULL);
	}

	gtk_widget_hide (dialog);
	gtk_widget_destroy (box);

	g_free (key);
	
	return exec;
}

/*
 * checks the existence of several bootloaders,
 * if so, then display the dialog to choose one
 */
static void
transfer_check_several_bootloaders ()
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	xmlNodePtr bootloaders = gst_xml_element_find_first (root, "bootloaders");
	gchar *exec = NULL;
	
	if (!bootloaders)
		return;

	gst_dialog_freeze (tool->main_dialog);

	exec = transfer_check_preferred_bootloader (bootloaders);

	if (!exec)
		exec = transfer_get_selected_bootloader (bootloaders);

	/* replace the XML with the config from the chosen bootloader */
	xmlFreeDoc (tool->config);
	xmlFreeDoc (tool->original_config);
	tool->config = gst_tool_run_get_directive (tool, NULL, "getfrom", exec, NULL);
	tool->original_config = xmlCopyDoc (tool->config, 1);

	gst_dialog_thaw (tool->main_dialog);
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;

	transfer_check_several_bootloaders ();
	
	root = gst_xml_doc_get_root (tool->config);

	transfer_globals_xml_to_gui (root);
	transfer_check_data (root);

	boot_table_clear ();
	table_populate (root);
}

void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = gst_xml_doc_get_root (tool->config);

	transfer_globals_gui_to_xml (root);
	transfer_check_data (root);
}
