/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* transfer.c: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
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
 * Authors: Alvaro Peña Gonzalez <apg@esware.com>
 *          Carlos García Campos <elkalmail@yahoo.es>
 */

/* Functions for transferring information between XML tree and UI */


#include <gnome.h>
#include <glade/glade.h>
#include "gst.h"

#include "transfer.h"
#include "disks-config.h"
#include "disks-storage.h"
#include "disks-factory-storage.h"
#include "disks-storage-partition.h"
#include "disks-storage-cdrom.h"

extern GstTool *tool;

static void
transfer_xml_to_config (xmlNodePtr root, GstDisksConfig *cfg)
{
	xmlNodePtr disk_node, part_node, node;
	GstDisksStorage *storage;
	GstDisksStoragePartition *part;
	gchar *buf;
	gulong p_size, storage_size;

	g_return_if_fail (root != NULL);
	g_return_if_fail (cfg != NULL);
	
	for (disk_node = gst_xml_element_find_first (root, "disk");
	     disk_node;
	     disk_node = gst_xml_element_find_next (disk_node, "disk"))
	{
		storage_size = 0;
		buf = gst_xml_get_child_content (disk_node, "media");
		storage = gst_disks_factory_storage_get (buf);
		g_free (buf);
		
		if (GST_IS_DISKS_STORAGE (storage)) {
			buf = gst_xml_get_child_content (disk_node, "device");
			if (buf) {
				g_object_set (G_OBJECT (storage), "device",
					      buf, NULL);
				g_free (buf);
			}
			
			buf = gst_xml_get_child_content (disk_node, "model");
			if (buf) {
				g_object_set (G_OBJECT (storage), "model",
					      buf, NULL);
				g_free (buf);
			}
			
			buf = gst_xml_get_child_content (disk_node, "size");
			if (buf) {
				g_object_set (G_OBJECT (storage), "size",
					      (gulong) g_ascii_strtoull (buf, NULL, 10),
					      NULL);
				storage_size = (gulong) g_ascii_strtoull (buf, NULL, 10);
				g_free (buf);
			}
		}

		if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
			node = gst_xml_element_find_first (disk_node, "empty");
			if (node) {
				if (gst_xml_element_get_bool_attr (node, "state")) {
					g_object_set (G_OBJECT (storage), "status",
						      CDROM_STATUS_EMPTY, NULL);
				} else {
					buf = gst_xml_get_child_content (disk_node, "type-content");
					if (buf) {
						g_object_set (
							G_OBJECT (storage), "status",
							gst_disks_storage_cdrom_get_status_from_name (buf),
							NULL);
						g_free (buf);
					}
				}
			}

			node = gst_xml_element_find_first (disk_node, "play-audio");
			if (node)
				g_object_set (G_OBJECT (storage), "play_audio",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);
			
			node = gst_xml_element_find_first (disk_node, "write-cdr");
			if (node)
				g_object_set (G_OBJECT (storage), "write_cdr",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);

			node = gst_xml_element_find_first (disk_node, "write-cdrw");
			if (node)
				g_object_set (G_OBJECT (storage), "write_cdrw",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);

			node = gst_xml_element_find_first (disk_node, "read-dvd");
			if (node)
				g_object_set (G_OBJECT (storage), "read_dvd",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);

			node = gst_xml_element_find_first (disk_node, "write-dvdr");
			if (node)
				g_object_set (G_OBJECT (storage), "write_dvdr",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);

			node = gst_xml_element_find_first (disk_node, "write-dvdram");
			if (node)
				g_object_set (G_OBJECT (storage), "write_dvdram",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);
		}

		p_size = 0;
		
		for (part_node = gst_xml_element_find_first (disk_node, "partition");
		     part_node;
		     part_node = gst_xml_element_find_next (part_node, "partition"))
		{
			part = GST_DISKS_STORAGE_PARTITION (gst_disks_storage_partition_new ());
			if (GST_IS_DISKS_STORAGE (part))
			{
				buf = gst_xml_get_child_content (part_node, "device");
				if (buf) {
					g_object_set (G_OBJECT (part), "device",
						      buf, NULL);
					g_free (buf);
				}

				buf = gst_xml_get_child_content (part_node, "type");
				if (buf) {
					g_object_set (G_OBJECT (part), "type",
						      gst_disks_storage_partition_get_typefs_from_name (buf),
						      NULL);
					g_free (buf);
				}

				buf = gst_xml_get_child_content (part_node, "point");
				if (buf) {
					g_object_set (G_OBJECT (part), "point",
						      buf, NULL);
					g_free (buf);
				}

				buf = gst_xml_get_child_content (part_node, "size");
				if (buf) {
					g_object_set (G_OBJECT (part), "size",
						      (gulong) g_ascii_strtoull (buf, NULL, 10),
						      NULL);
					p_size += (gulong) g_ascii_strtoull (buf, NULL, 10);
					g_free (buf);
				}

				buf = gst_xml_get_child_content (part_node, "free");
				if (buf) {
					g_object_set (G_OBJECT (part), "free",
						      (gulong) g_ascii_strtoull (buf, NULL, 10),
						      NULL);
					g_free (buf);
				}

				node = gst_xml_element_find_first (part_node, "bootable");
				if (node)
					g_object_set (G_OBJECT (part), "bootable", 
						      gst_xml_element_get_bool_attr (
							      node, "state"),
						      NULL);
				
				node = gst_xml_element_find_first (part_node, "integritycheck");
				if (node)
					g_object_set (G_OBJECT (part), "integritycheck", 
						      gst_xml_element_get_bool_attr (
							      node, "state"),
						      NULL);
				
				node = gst_xml_element_find_first (part_node, "mounted");
				if (node)
					g_object_set (G_OBJECT (part), "mounted", 
						      gst_xml_element_get_bool_attr (
							      node, "state"),
						      NULL);
				
				node = gst_xml_element_find_first (part_node, "listed");
				if (node)
					g_object_set (G_OBJECT (part), "listed", 
						      gst_xml_element_get_bool_attr (
							      node, "state"),
						      NULL);
				
				node = gst_xml_element_find_first (part_node, "detected");
				if (node)
					g_object_set (G_OBJECT (part), "detected", 
						      gst_xml_element_get_bool_attr (
							      node, "state"),
							      NULL);
				
				gst_disks_storage_add_child (storage, GST_DISKS_STORAGE (part));
			}
		}

		/*if ((storage_size - p_size) > 0) {
			part = GST_DISKS_STORAGE_PARTITION (gst_disks_storage_partition_new ());
			g_object_set (G_OBJECT (part), "type", PARTITION_TYPE_FREE,
				      "size", (storage_size - p_size), NULL);
			gst_disks_storage_add_child (storage, GST_DISKS_STORAGE (part));
			}*/
		
		gst_disks_config_add_storage (cfg, storage);
	}
}

static void
transfer_config_to_xml (GstDisksConfig *cfg, xmlNodePtr root)
{
	
}

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	GstDisksConfig *cfg;

	cfg = (GstDisksConfig *) data;
	
	root = gst_xml_doc_get_root (tool->config);

	transfer_xml_to_config (root, cfg);

	gst_storage_gui_setup (cfg);

}

void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	GstDisksConfig *cfg;

	cfg = (GstDisksConfig *) data;
	
	root = gst_xml_doc_get_root (tool->config);

	/*transfer_config_to_xml (dsk, root);*/
	/*transfer_globals_gui_to_xml (root);
	  transfer_check_data (root);*/
}
