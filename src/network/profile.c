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

/* profile saving functions */
static void
profile_save_tag_list (xmlNodePtr source, xmlNodePtr dest, gchar *list[])
{
	gchar **string;
	gchar *value;

	for (string = list; *string != NULL; string++) {
		value = gst_xml_get_child_content (source, *string);
		if ((value != NULL) && (strlen (value) != 0))
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

		profile_save_list (node, new_statichost, "alias");
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

/* functions for comparing a profile with the current configuration */
static gboolean
profile_compare_tag_list (xmlNodePtr current, xmlNodePtr profile, gchar *list[])
{
	gchar **string = list;
	gchar *value1, *value2;
	gboolean value = TRUE;
       
	while ((*string != NULL) && (value == TRUE)) {
		value1 = gst_xml_get_child_content (current, *string);
		value2 = gst_xml_get_child_content (profile, *string);

		if (COMPARE_TAGS (value1, value2))
			value = FALSE;

		g_free (value1);
		g_free (value2);
		string++;
	} 

	return value;
}

static gboolean
profile_compare_general_data (xmlNodePtr current, xmlNodePtr profile)
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

	profile_compare_tag_list (current, profile, list);
}

static gboolean
profile_compare_list (xmlNodePtr current, xmlNodePtr profile, gchar *tag)
{
	xmlNodePtr node1, node2;
	gchar *value1, *value2;
	gboolean value = TRUE;

	node1 = gst_xml_element_find_first (current, tag);
	node2 = gst_xml_element_find_first (profile, tag);

	while ((node1 != NULL) && (node2 != NULL) && (value == TRUE)) {
		value1 = gst_xml_element_get_content (node1);
		value2 = gst_xml_element_get_content (node2);

		if (COMPARE_TAGS (value1, value2))
			value = FALSE;

		g_free (value1);
		g_free (value2);

		node1 = gst_xml_element_find_next (node1, tag);
		node2 = gst_xml_element_find_next (node2, tag);
	} 

	return value;
}

static gboolean
profile_compare_statichosts (xmlNodePtr current, xmlNodePtr profile)
{
	xmlNodePtr node1, node2, alias_node1, alias_node2;
	gchar *ip1, *ip2;
	gboolean value = TRUE;

	node1 = gst_xml_element_find_first (current, "statichost");
	node2 = gst_xml_element_find_first (profile, "statichost");

	while ((node1 != NULL) && (node2 != NULL) && (value == TRUE)) {
		ip1 = gst_xml_get_child_content (node1, "ip");
		ip2 = gst_xml_get_child_content (node2, "ip");

		if (COMPARE_TAGS (ip1, ip2))
			value = FALSE;
		else
			value = profile_compare_list (node1, node2, "alias");

		g_free (ip1);
		g_free (ip2);

		node1 = gst_xml_element_find_next (node1, "statichost");
		node2 = gst_xml_element_find_next (node2, "statichost");
	}

	return value;
}

static gboolean
profile_compare_interfaces (xmlNodePtr current, xmlNodePtr profile)
{
	xmlNodePtr node1, node2;
	gboolean value = TRUE;
	gchar *list [] = {
		"address",
		"auto",
		"bootproto",
		"broadcast",
		"debug",
		"dev",
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

	node1 = gst_xml_element_find_first (current, "interface");
	node2 = gst_xml_element_find_first (profile, "interface");

	while ((node1 != NULL) && (node2 != NULL) && (value == TRUE)) {
		value = profile_compare_tag_list (node1, node2, list);

		node1 = gst_xml_element_find_next (node1, "interface");
		node2 = gst_xml_element_find_next (node2, "interface");
	}

	return value;
}

static gboolean
profile_compare_dialing (xmlNodePtr current, xmlNodePtr profile)
{
	xmlNodePtr node1, node2;
	gboolean value = TRUE;
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

	node1 = gst_xml_element_find_first (current, "dialing");
	node2 = gst_xml_element_find_first (profile, "dialing");

	while ((node1 != NULL) && (node2 != NULL) && (value == TRUE)) {
		value = profile_compare_tag_list (node1, node2, list);

		node1 = gst_xml_element_find_next (node1, "dialing");
		node2 = gst_xml_element_find_next (node2, "dialing");
	} 

	return value;
}

static gboolean
profile_compare_with_current_configuration (xmlNodePtr current, xmlNodePtr profile)
{
	return (profile_compare_general_data (current, profile) &&
		profile_compare_list (current, profile, "nameservers") &&
		profile_compare_list (current, profile, "searchdomain") &&
		profile_compare_statichosts (current, profile) &&
		profile_compare_interfaces (current, profile) &&
		profile_compare_dialing (current, profile));
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
	gint counter = 1;
	gint default_profile = 0;

	menu_item = gtk_menu_item_new_with_label (_("Unknown"));
	gtk_widget_set_sensitive (menu_item, FALSE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	for (node = gst_xml_element_find_first (profiledb, "profile");
	     node != NULL;
	     node = gst_xml_element_find_next (node, "profile"))
	{
		profile_name = gst_xml_get_child_content (node, "name");

		/* set the option menu entry */
		menu_item = gtk_menu_item_new_with_label (profile_name);
		g_signal_connect (G_OBJECT (menu_item), "activate",
				  G_CALLBACK (on_network_profile_option_selected), node);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

		if (profile_compare_with_current_configuration (root, node))
			default_profile = counter;

		g_free (profile_name);
		counter++;
	}

	gtk_widget_set_sensitive (profiles_menu, TRUE);
	gtk_widget_show_all (menu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (profiles_menu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (profiles_menu), default_profile);
}
