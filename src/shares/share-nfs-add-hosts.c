/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-nfs-add-hosts.c: this file is part of shares-admin, a gnome-system-tool frontend 
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
 * Authors: Carlos Garnacho Parro <carlosg@gnome.org>.
 */

#include <glib/gi18n.h>

#include "share-nfs-add-hosts.h"
#include "gst.h"

extern GstTool *tool;
extern GtkIconTheme *icon_theme;

static void
share_nfs_create_size_group (void)
{
	GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	GtkWidget    *widget;

	widget = gst_dialog_get_widget (tool->main_dialog, "share_nfs_host_type_label");
	gtk_size_group_add_widget (group, widget);

	widget = gst_dialog_get_widget (tool->main_dialog, "share_nfs_hostname_label");
	gtk_size_group_add_widget (group, widget);
	
	widget = gst_dialog_get_widget (tool->main_dialog, "share_nfs_address_label");
	gtk_size_group_add_widget (group, widget);

	widget = gst_dialog_get_widget (tool->main_dialog, "share_nfs_network_label");
	gtk_size_group_add_widget (group, widget);
}

static void
combo_add_columns (GtkComboBox *combo)
{
	GtkCellRenderer *renderer;

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer,
				       "pixbuf", NFS_HOST_COL_PIXBUF);

	g_object_unref (renderer);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer,
				       "text", NFS_HOST_COL_NAME);
	g_object_unref (renderer);
}

static void
share_nfs_create_combo (void)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_nfs_host_type");
	GtkListStore *store;

	store = gtk_list_store_new (NFS_HOST_COL_LAST,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING,
				    G_TYPE_INT,
				    G_TYPE_STRING,
				    G_TYPE_STRING);

	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));
	g_object_unref (store);

	combo_add_columns (GTK_COMBO_BOX (combo));
}

void
share_nfs_add_hosts_dialog_setup (void)
{
	share_nfs_create_size_group ();
	share_nfs_create_combo ();
}

static void
share_nfs_add_ifaces_combo_elements (GtkListStore *store, xmlNodePtr node)
{
	GtkTreeIter  iter;
	GdkPixbuf   *pixbuf;
	gchar       *dev, *str, *network, *netmask;
	xmlNodePtr   iface;

	pixbuf = gdk_pixbuf_new_from_file (PIXMAPS_DIR "/16_ethernet.xpm", NULL);

	for (iface = gst_xml_element_find_first (node, "interface");
	     iface; iface = gst_xml_element_find_next (iface, "interface")) {
		dev     = gst_xml_get_child_content (iface, "dev");
		network = gst_xml_get_child_content (iface, "network");
		netmask = gst_xml_get_child_content (iface, "mask");

		str = g_strdup_printf (_("Hosts in the %s network"), dev);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    0, pixbuf,
				    1, str,
				    2, NFS_SHARE_IFACE,
				    3, network,
				    4, netmask,
				    -1);
		g_free (dev);
		g_free (str);
		g_free (network);
		g_free (netmask);
	}

	gdk_pixbuf_unref (pixbuf);
}

static void
share_nfs_add_static_combo_elements (GtkListStore *store)
{
	GtkTreeIter  iter;
	GdkPixbuf   *pixbuf;

	pixbuf = gtk_icon_theme_load_icon (icon_theme,
					   "gnome-fs-network",
					   16, 0, NULL);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, pixbuf,
			    1, _("Specify hostname"),
			    2, NFS_SHARE_HOSTNAME,
			    3, NULL,
			    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, pixbuf,
			    1, _("Specify IP address"),
			    2, NFS_SHARE_ADDRESS,
			    3, NULL,
			    -1);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, pixbuf,
			    1, _("Specify network"),
			    2, NFS_SHARE_NETWORK,
			    3, NULL,
			    -1);
	gdk_pixbuf_unref (pixbuf);
}

static void
share_nfs_add_hosts_completion (xmlNodePtr node)
{
	GtkWidget          *entry = gst_dialog_get_widget (tool->main_dialog, "share_nfs_hostname");
	GtkEntryCompletion *completion;
	GtkListStore       *store;
	GtkTreeIter         iter;
	xmlNodePtr          host, alias;
	gchar              *text;

	completion = gtk_entry_completion_new ();
	store = gtk_list_store_new (1, G_TYPE_STRING);

	gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
	g_object_unref (store);

	gtk_entry_set_completion (GTK_ENTRY (entry), completion);
	g_object_unref (completion);

	gtk_entry_completion_set_text_column (completion, 0);

	for (host = gst_xml_element_find_first (node, "statichost");
	     host; host = gst_xml_element_find_next (host, "statichost")) {
		for (alias = gst_xml_element_find_first (host, "alias");
		     alias; alias = gst_xml_element_find_next (alias, "alias")) {
			text = gst_xml_element_get_content (alias);

			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, text, -1);

			g_free (text);
		}
	}
}

static void
share_nfs_add_combo_elements (void)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_nfs_host_type");
	GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	xmlDoc       *doc;
	xmlNodePtr    node;

	doc = gst_tool_run_get_directive (tool, NULL, "get_network_conf", NULL);
	node = gst_xml_doc_get_root (doc);

	gtk_list_store_clear (GTK_LIST_STORE (model));

	share_nfs_add_hosts_completion (node);

	share_nfs_add_ifaces_combo_elements (GTK_LIST_STORE (model), node);
	share_nfs_add_static_combo_elements (GTK_LIST_STORE (model));

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	gst_xml_doc_destroy (doc);
}

GtkWidget*
share_nfs_add_hosts_dialog_prepare (void)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "share_nfs_add_dialog");

	share_nfs_add_combo_elements ();
	
	return dialog;
}

gboolean
share_nfs_add_hosts_dialog_get_data (gchar **str, gboolean *read_only)
{
	GtkWidget    *combo, *toggle, *w1, *w2;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          type;
	gchar        *str1, *str2;

	combo  = gst_dialog_get_widget (tool->main_dialog, "share_nfs_host_type");
	toggle = gst_dialog_get_widget (tool->main_dialog, "share_nfs_readonly");
	
	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
		return FALSE;

	model  = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	gtk_tree_model_get (model, &iter, NFS_HOST_COL_TYPE, &type, -1);

	switch (type) {
	case NFS_SHARE_IFACE:
		gtk_tree_model_get (model, &iter,
				    NFS_HOST_COL_NETWORK, &str1,
				    NFS_HOST_COL_NETMASK, &str2,
				    -1);
		*str = g_strdup_printf ("%s/%s", str1, str2);
		g_free (str1);
		g_free (str2);
		break;
	case NFS_SHARE_HOSTNAME:
		w1 = gst_dialog_get_widget (tool->main_dialog, "share_nfs_hostname");
		*str = g_strdup (gtk_entry_get_text (GTK_ENTRY (w1)));
		break;
	case NFS_SHARE_ADDRESS:
		w1 = gst_dialog_get_widget (tool->main_dialog, "share_nfs_address");
		*str = g_strdup (gtk_entry_get_text (GTK_ENTRY (w1)));
		break;
	case NFS_SHARE_NETWORK:
		w1 = gst_dialog_get_widget (tool->main_dialog, "share_nfs_network");
		str1 = (gchar *) gtk_entry_get_text (GTK_ENTRY (w1));

		w2 = gst_dialog_get_widget (tool->main_dialog, "share_nfs_netmask");
		str2 = (gchar *) gtk_entry_get_text (GTK_ENTRY (w2));

		*str = g_strdup_printf ("%s/%s", str1, str2);
		break;
	}

	*read_only = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));;

	return TRUE;
}
