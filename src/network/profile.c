/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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
 * Authors: Carlos Garnacho Parro  <carlosg@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

#include <gnome.h>

#include "gst.h"
#include "callbacks.h"
#include "profile.h"
#include "transfer.h"

gboolean
profile_delete (xmlNodePtr node)
{
	gint reply;
	gchar *txt, *name;
	GtkWidget *dialog;

	name = gst_xml_get_child_content (node, "name");
	txt = g_strdup_printf (_("Are you sure you want to delete profile %s?"), name);
	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, txt);
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (reply == GTK_RESPONSE_YES) {
		gst_xml_element_destroy (node);
		return TRUE;
	} else {
		return FALSE;
	}
}

static void
profile_save_tag_list (xmlNodePtr source, xmlNodePtr dest, gchar *list[])
{
	gchar **string;
	gchar *value;

	for (string = list; *string != NULL; string++) {
		value = gst_xml_get_child_content (source, *string);
		if (value != NULL)
			gst_xml_element_add_with_content (dest, *string, value);

		g_free (value);
	}
}

static void
profile_save_list (xmlNodePtr source, xmlNodePtr dest, gchar *tag)
{
	xmlNodePtr node, nameserver;

	for (node = gst_xml_element_find_first (source, tag);
	     node != NULL;
	     node = gst_xml_element_find_next (node, tag))
	{
		nameserver = gst_xml_element_add (dest, tag);
		gst_xml_element_set_content (nameserver, gst_xml_element_get_content (node));
	}
}

static void
profile_save_interfaces (xmlNodePtr source, xmlNodePtr dest)
{
	xmlNodePtr node, new_interface;
	gchar *list [] = {
		"address",
		"auto",
		"bootproto",
		"broadcast",
		"debug",
		"dev",
		"enabled",
		"file",
		"name",
		"netmask",
		"network",
		"noauth",
		"persist",
		"serial_hwctl",
		"serial_port",
		"set_default_gw",
		"update_dns",
		"user",
		"wvsection",
		NULL
	};

	for (node = gst_xml_element_find_first (source, "interface");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "interface"))
	{
		new_interface = gst_xml_element_add (dest, "interface");

		profile_save_tag_list (node, new_interface, list);
	}
}

static void
profile_save_dialing (xmlNodePtr source, xmlNodePtr dest)
{
	xmlNodePtr node, new_dialing;
	gchar *list[] = {
		"device",
		"dial_command",
		"init1",
		"init2",
		"init3",
		"init4",
		"init5",
		"init6",
		"init7",
		"init8",
		"init9",
		"login",
		"name",
		"password",
		"phone",
		"speed",
		"stupid",
		"type",
		"volume",
		NULL
	};

	for (node = gst_xml_element_find_first (source, "dialing");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "dialing"))
	{
		new_dialing = gst_xml_element_add (dest, "dialing");

		profile_save_tag_list (node, new_dialing, list);
	}
}

static void
profile_save_statichosts (xmlNodePtr source, xmlNodePtr dest)
{
	xmlNodePtr node, new_statichost, alias, new_alias;

	for (node = gst_xml_element_find_first (source, "statichost");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "statichost"))
	{
		new_statichost = gst_xml_element_add (dest, "statichost");

		gst_xml_element_add_with_content (new_statichost, "ip",
						  gst_xml_get_child_content (node, "ip"));

		for (alias = gst_xml_element_find_first (node, "alias");
		     alias != NULL;
		     alias = gst_xml_element_find_next (alias, "alias"))
		{
			new_alias = gst_xml_element_add (new_statichost, "alias");
			gst_xml_element_set_content (new_alias, gst_xml_element_get_content (alias));
		}
	}
}

static void
profile_save_general_data (xmlNodePtr source, xmlNodePtr dest)
{
	gchar *list[] = {
		"hostname",
		"domain",
		"smbdesc",
		"smbuse",
		"winsserver",
		"winsuse",
		"workgroup",
		"gateway",
		"gatewaydev",
		NULL
	};

	profile_save_tag_list (source, dest, list);
}

void
profile_save_current (const gchar *name, const gchar *description, GstTool *tool)
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);
	xmlNodePtr profiledb = gst_xml_element_find_first (root, "profiledb");
	xmlNodePtr new_profile = gst_xml_element_add (profiledb, "profile");

	/* first of all, we sync the content of the dialogs with the XML */
	transfer_gui_to_xml (tool, NULL);

	/* save name and description */
	gst_xml_element_add_with_content (new_profile, "name", name);
	gst_xml_element_add_with_content (new_profile, "description", description);

	/* store the current configuration */
	profile_save_general_data (root, new_profile);
	profile_save_list (root, new_profile, "nameservers");
	profile_save_list (root, new_profile, "searchdomain");
	profile_save_statichosts (root, new_profile);
	profile_save_interfaces (root, new_profile);
	profile_save_dialing (root, new_profile);
}


/* stuff to active a profile */
static void
profile_set_active_list (xmlNodePtr profile, xmlNodePtr root, gchar *tag)
{
	gst_xml_element_destroy_children_by_name (root, tag);
	profile_save_list (profile, root, tag);
}

static void
profile_set_active_statichosts (xmlNodePtr profile, xmlNodePtr root)
{
	gst_xml_element_destroy_children_by_name (root, "statichost");
	profile_save_statichosts (profile, root);
}

static void
profile_set_active_interfaces (xmlNodePtr profile, xmlNodePtr root)
{
	gst_xml_element_destroy_children_by_name (root, "interface");
	profile_save_interfaces (profile, root);
}

static void
profile_set_active_dialing (xmlNodePtr profile, xmlNodePtr root)
{
	gst_xml_element_destroy_children_by_name (root, "dialing");
	profile_save_dialing (profile, root);
}

void
profile_set_active (xmlNodePtr profile, GstTool *tool)
{
	xmlNodePtr root = gst_xml_doc_get_root (tool->config);

	profile_save_general_data (profile, root);
	
	profile_set_active_list (profile, root, "nameservers");
	profile_set_active_list (profile, root, "searchdomain");
	profile_set_active_statichosts (profile, root);
	profile_set_active_interfaces (profile, root);
	profile_set_active_dialing (profile, root);
}

void
profile_populate_option_menu (GstTool *tool, xmlNodePtr root)
{
	xmlNodePtr profiledb = gst_xml_element_find_first (root, "profiledb");
	xmlNodePtr node;
	GtkWidget *profiles_menu = gst_dialog_get_widget (tool->main_dialog,
							  "network_profiles_menu");
	GtkWidget *menu = gtk_menu_new ();
	GtkWidget *menu_item;
	gchar *profile_name;
	gint counter = 0;
	

	for (node = gst_xml_element_find_first (profiledb, "profile");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "profile"))
	{
		counter++;
		
		profile_name = gst_xml_get_child_content (node, "name");

		/* set the option menu entry */
		menu_item = gtk_menu_item_new_with_label (profile_name);
		g_signal_connect (G_OBJECT (menu_item), "activate",
				  G_CALLBACK (on_network_profile_option_selected), node);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

		g_free (profile_name);
	}

	if (counter == 0) {
		/* set unsensitive the option menu */
		gtk_widget_set_sensitive (profiles_menu, FALSE);
	} else {
		/* show the menu and attach it to the option menu */
		gtk_widget_set_sensitive (profiles_menu, TRUE);
		gtk_widget_show_all (menu);
		gtk_option_menu_set_menu (GTK_OPTION_MENU (profiles_menu), menu);
	}
}
