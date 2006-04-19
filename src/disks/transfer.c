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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <sys/types.h>
#include <unistd.h>

#include "gst-disks-tool.h"
#include "disks-storage.h"
#include "disks-factory-storage.h"
#include "disks-partition.h"
#include "disks-storage-disk.h"
#include "disks-storage-cdrom.h"
#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-cdrom-disc-audio.h"
#include "disks-cdrom-disc-mixed.h"
#include "disks-gui.h"
#include "transfer.h"

extern GstTool *tool;

static void
transfer_xml_common_to_config (GstDisksStorage *storage, xmlNodePtr disk_node)
{
	xmlNodePtr node;
	gchar *buf;
	gboolean present = FALSE;

	buf = gst_xml_get_child_content (disk_node, "device");
	if (buf) {
		g_object_set (G_OBJECT (storage), "device",
			      buf, NULL);
		g_free (buf);
	}

	buf = gst_xml_get_child_content (disk_node, "alias");
	if (buf) {
		g_object_set (G_OBJECT (storage), "alias",
			      buf, NULL);
		g_free (buf);
	}

	buf = gst_xml_get_child_content (disk_node, "model");
	if (buf) {
		g_object_set (G_OBJECT (storage), "model",
			      buf, NULL);
		g_free (buf);
	}

	node = gst_xml_element_find_first (disk_node, "present");
	if (node) {
		present = gst_xml_element_get_bool_attr (node, "state");
		g_object_set (G_OBJECT (storage), "present",
			      present, NULL);
	}

	buf = gst_xml_get_child_content (disk_node, "size");
	if (buf) {
		g_object_set (G_OBJECT (storage), "size",
			      (gulong) g_ascii_strtoull (buf, NULL, 10),
			      NULL);
		g_free (buf);
	}

	if (GST_IS_DISKS_STORAGE_DISK (storage)) {
		if (present) {
			g_object_set (G_OBJECT (storage), "icon_name",
				      "gnome-dev-harddisk", NULL);
		} else {
			g_object_set (G_OBJECT (storage), "icon_name",
				      "gnome-dev-removable", NULL);
		}
	}
}

static void
transfer_xml_cdrom_to_config (GstDisksStorage *storage, xmlNodePtr disk_node)
{
	xmlNodePtr node;

	node = gst_xml_element_find_first (disk_node, "empty");
	if (node) {
		g_object_set (G_OBJECT (storage), "empty",
			      gst_xml_element_get_bool_attr (
				      node, "state"),
			      NULL);
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

static void
transfer_xml_disk_to_config (GstDisksStorage *storage, xmlNodePtr disk_node)
{
	xmlNodePtr node, part_node;
	GstDisksPartition *part = NULL;
	GList *partitions = NULL;
	gchar *buf;
	gboolean exists;

	g_object_get (G_OBJECT (storage), "partitions", &partitions, NULL);
	
	for (part_node = gst_xml_element_find_first (disk_node, "partition");
	     part_node;
	     part_node = gst_xml_element_find_next (part_node, "partition"))
	{
		exists = FALSE;
		
		buf = gst_xml_get_child_content (part_node, "device");
		if (buf) {
			part = gst_disks_storage_disk_get_partition (
				GST_DISKS_STORAGE_DISK (storage), buf);
			
			if (part == NULL) {
				part = GST_DISKS_PARTITION (gst_disks_partition_new ());
				exists = FALSE;
			} else {
				exists = TRUE;
			}
			
			g_object_set (G_OBJECT (part), "device",
				      buf, NULL);
			
			g_free (buf);
		}
		
		if (part == NULL) {
			part = GST_DISKS_PARTITION (gst_disks_partition_new ());
			exists = FALSE;
		}
		
		buf = gst_xml_get_child_content (part_node, "type");
		if (buf) {
			g_object_set (G_OBJECT (part), "type",
				      gst_disks_partition_get_typefs_from_name (buf),
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

		g_object_set (G_OBJECT (part), "disk", storage, NULL);
		
		if (exists)
			partitions = g_list_remove (partitions, part);
		
		partitions = g_list_append (partitions, part);

		g_object_set (G_OBJECT (storage), "partitions", partitions, NULL);
	}
}
		

static void
transfer_xml_to_config (xmlNodePtr root)
{
	xmlNodePtr disk_node;
	GstDisksStorage *storage;
	gchar *buf;

	g_return_if_fail (root != NULL);
	
	for (disk_node = gst_xml_element_find_first (root, "disk");
	     disk_node;
	     disk_node = gst_xml_element_find_next (disk_node, "disk"))
	{
		buf = gst_xml_get_child_content (disk_node, "media");
		storage = gst_disks_factory_storage_get (buf);
		g_free (buf);
		
		if (GST_IS_DISKS_STORAGE (storage)) {
			transfer_xml_common_to_config (storage, disk_node);
		}

		if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
			transfer_xml_cdrom_to_config (storage, disk_node);
		} else if (GST_IS_DISKS_STORAGE_DISK (storage)) {
			transfer_xml_disk_to_config (storage, disk_node);
		}
		
		if (storage)
			gst_disks_tool_add_storage (GST_DISKS_TOOL (tool), storage);
	}
}

/* Uncomment in the future */
/*static void
transfer_config_to_xml (xmlNodePtr root)
{
	return;
}*/

void
transfer_xml_to_gui (GstTool *tool, gpointer data)
{
	xmlNodePtr root;
	
	root = gst_xml_doc_get_root (tool->config);

	transfer_xml_to_config (root);

	gst_disks_gui_setup ();
}

void
transfer_gui_to_xml (GstTool *tool, gpointer data)
{
	xmlNodePtr root;

	root = gst_xml_doc_get_root (tool->config);

	/*transfer_config_to_xml (dsk, root);*/
	/*transfer_globals_gui_to_xml (root);
	  transfer_check_data (root);*/
}

void 
gst_disks_mount_partition (GstDisksPartition *part)
{
	xmlDoc *xml;
	xmlNodePtr root, part_node, node;
	gchar *device, *typefs, *point;
	gboolean mounted, listed;
	gchar *buf, *text_uid;
	GstPartitionTypeFs type;

	g_return_if_fail (GST_IS_DISKS_PARTITION (part));
	
	g_object_get (G_OBJECT (part), "type", &type, "point", &point,
		      "device", &device, "mounted", &mounted,
		      "listed", &listed, NULL);

	typefs = gst_disks_partition_get_typefs (type);

	text_uid = g_strdup_printf ("%d", (guint) getuid ());
	xml = gst_tool_run_get_directive (tool, NULL, "mount",
					  device, "disk",
					  typefs, point,
					  mounted ? "1" : "0",
					  listed  ? "1" : "0",
					  text_uid,
					  NULL);
	g_free (text_uid);

	if (!xml) {
		return;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		buf = gst_xml_get_child_content (root, "error");
		if (buf) {
			g_warning ("%s", buf);
			g_free (buf);
		}

		part_node = gst_xml_element_find_first (root, "partition");
		if (part_node) {
			buf = gst_xml_get_child_content (part_node, "typefs");
			if (buf) {
				g_object_set (G_OBJECT (part), "type",
					      gst_disks_partition_get_typefs_from_name (buf),
					      NULL);
				g_free (buf);
			}

			buf = gst_xml_get_child_content (part_node, "point");
			if (buf) {
				g_object_set (G_OBJECT (part), "point",
					      buf, NULL);
				g_free (buf);
			}

			buf = gst_xml_get_child_content (part_node, "free");
			if (buf) {
				g_object_set (G_OBJECT (part), "free",
					      (gulong) g_ascii_strtoull (buf, NULL, 10),
					      NULL);
				g_free (buf);
			}

			node = gst_xml_element_find_first (part_node, "mounted");
			if (node)
				g_object_set (G_OBJECT (part), "mounted",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);

			gst_xml_doc_destroy (xml);

			return;
		}
	}
	gst_xml_doc_destroy (xml);

	return;
}

gboolean
gst_disks_format_partition (GstDisksPartition *part, GstPartitionTypeFs fs_type)
{
	xmlDoc *xml;
	xmlNodePtr root;
	gchar *device, *typefs, *command;
	gchar *buf;
	GstPartitionTypeFsInfo *table;

	g_return_val_if_fail (GST_IS_DISKS_PARTITION (part), FALSE);

	g_object_get (G_OBJECT (part), "device", &device, NULL);

	table = gst_disks_partition_get_type_fs_info_table ();
	typefs = g_strdup (table[fs_type].fs_name);
	command = g_strdup (table[fs_type].fs_format_command);
	if (!command) {
		/* File system not possible to format */
		/* TODO: manage error */
		return FALSE;
	}

	xml = gst_tool_run_get_directive (tool, NULL, "format",
					  command, device, typefs,
					  " ", /* TODO: format options */
					  NULL);
	
	if (command) g_free (command);
	if (typefs)  g_free (typefs);

	if (!xml) {
		/* TODO: manage error */
		return FALSE;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		buf = gst_xml_get_child_content (root, "type");
		if (buf) {
			g_object_set (G_OBJECT (part), "type",
				      gst_disks_partition_get_typefs_from_name (buf),
				      NULL);
			g_free (buf);
		}

		buf = gst_xml_get_child_content (root, "error");
		if (buf) {
			g_warning ("%s", buf); /* FIXME: manage error */
			g_free (buf);
			gst_xml_doc_destroy (xml);
			
			return FALSE;
		}
	}
	
	gst_xml_doc_destroy (xml);

	return TRUE;
}

void
gst_disks_mount_cdrom_disc_data (GstCdromDiscData *disc_data)
{
	xmlDoc *xml;
	xmlNodePtr root, cdrom_node, node;
	gchar *device, *alias, *typefs, *point;
	gboolean mounted, listed;
	gulong size;
	gchar *buf, *text_uid;
	GstDisksStorageCdrom *cdrom;

	g_return_if_fail (GST_IS_CDROM_DISC_DATA (disc_data));

	cdrom = GST_DISKS_STORAGE_CDROM (gst_cdrom_disc_get_cdrom (GST_CDROM_DISC (disc_data)));
	g_object_get (G_OBJECT (cdrom), "device", &device,
		      "alias", &alias, "listed", &listed, NULL);

	g_object_get (G_OBJECT (disc_data), "mount-point", &point,
		      "mounted", &mounted, "size", &size, NULL);
		
	typefs = g_strdup ("iso9660");
	text_uid = g_strdup_printf ("%d", (guint) getuid ());

	xml = gst_tool_run_get_directive (tool, NULL, "mount",
					  (listed && alias) ? alias : device,
					  "cdrom", 
					  typefs, point,
					  mounted ? "1" : "0",
					  listed  ? "1" : "0",
					  text_uid,
					  NULL);
	g_free (typefs);
	g_free (text_uid);
	
	if (!xml) {
		return;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		buf = gst_xml_get_child_content (root, "error");
		if (buf) {
			g_warning ("%s", buf);
			g_free (buf);
		}
		
		cdrom_node = gst_xml_element_find_first (root, "cdrom");
		if (cdrom_node) {
			node = gst_xml_element_find_first (cdrom_node, "mounted");
			if (node)
				g_object_set (G_OBJECT (disc_data), "mounted",
					      gst_xml_element_get_bool_attr (
						      node, "state"),
					      NULL);
			
			buf = gst_xml_get_child_content (cdrom_node, "point");
			if (buf) {
				g_object_set (G_OBJECT (disc_data), "mount-point",
					      buf, NULL);
				g_free (buf);
			}
			
			buf = gst_xml_get_child_content (cdrom_node, "size");
			if (buf) {
				g_object_set (G_OBJECT (disc_data), "size",
					      (gulong) g_ascii_strtoull (buf, NULL, 10),
					      NULL);
				g_object_set (G_OBJECT (cdrom), "size",
					      (gulong) g_ascii_strtoull (buf, NULL, 10),
					      NULL);
				g_free (buf);
			}
			gst_xml_doc_destroy (xml);
			
			return;
		}

		gst_xml_doc_destroy (xml);
	}
	
	return;
}

static void
cdrom_parse_data_info (xmlNodePtr disc_info, GstCdromDiscData *disc, GstDisksStorageCdrom *cdrom)
{
	xmlNodePtr node;
	gchar *buf;
	
	node = gst_xml_element_find_first (disc_info, "mounted");
	if (node) {
		g_object_set (G_OBJECT (disc), "mounted",
			      gst_xml_element_get_bool_attr (
				      node, "state"),
			      NULL);
	}

	buf = gst_xml_get_child_content (disc_info, "point");
	if (buf) {
		g_object_set (G_OBJECT (disc), "mount-point",
			      buf, NULL);
		g_free (buf);
	}

	buf = gst_xml_get_child_content (disc_info, "size");
	if (buf) {
		g_object_set (G_OBJECT (disc), "size",
			      (gulong) g_ascii_strtoull (buf, NULL, 10),
			      NULL);
		g_object_set (G_OBJECT (cdrom), "size",
			      (gulong) g_ascii_strtoull (buf, NULL, 10),
			      NULL);
		g_free (buf);
	}
}

static void
cdrom_parse_audio_info (xmlNodePtr disc_info, GstCdromDiscAudio *disc)
{
	gchar *buf;
	
	buf = gst_xml_get_child_content (disc_info, "audio-tracks");
	if (buf) {
		g_object_set (G_OBJECT (disc), "num-tracks",
			      (guint) g_ascii_strtoull (buf, NULL, 10),
			      NULL);
		g_free (buf);
	}

	buf = gst_xml_get_child_content (disc_info, "duration");
	if (buf) {
		g_object_set (G_OBJECT (disc), "duration",
			      buf, NULL);
		g_free (buf);
	}
}

GstCdromDisc *
gst_disks_cdrom_get_disc_from_xml (GstDisksStorageCdrom *cdrom)
{
	xmlDoc *xml;
	xmlNodePtr root, disc_info, node;
	gchar *buf, *device, *alias;
	gboolean empty = TRUE;
	GstCdromDisc *disc;
	GstCdromDiscData *data;
	GstCdromDiscAudio *audio;


	g_object_get (G_OBJECT (cdrom), "device", &device,
		      "alias", &alias, "disc", &disc, NULL);

	xml = gst_tool_run_get_directive (tool, NULL, "cdrom_disc_info",
					  device,
					  alias != NULL ? alias : " ",
					  NULL);
	if (!xml) {
		if (disc)
			g_object_unref (G_OBJECT (disc));
		disc = NULL;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		disc_info = root;
		node = gst_xml_element_find_first (disc_info, "empty");
		if (node) {
			empty = gst_xml_element_get_bool_attr (node, "state");
			g_object_set (G_OBJECT (cdrom), "empty", empty, NULL);
		}
		
		if (empty) {
			if (disc)
				g_object_unref (G_OBJECT (disc));
			disc = NULL;
		} else {
			buf = gst_xml_get_child_content (disc_info, "type-content");
			if (buf) {
				if (g_ascii_strcasecmp (buf, "data") == 0) {
					if (!disc) {
						disc = gst_cdrom_disc_data_new ();
					} else if (!GST_IS_CDROM_DISC_DATA (disc)) {
						g_object_unref (G_OBJECT (disc));
						disc = gst_cdrom_disc_data_new ();
					}

					cdrom_parse_data_info (disc_info, GST_CDROM_DISC_DATA (disc),
							       cdrom);
				} else if (g_ascii_strcasecmp (buf, "audio") == 0) {
					if (!disc) {
						disc = gst_cdrom_disc_audio_new ();
					} else if (!GST_IS_CDROM_DISC_AUDIO (disc)) {
						g_object_unref (G_OBJECT (disc));
						disc = gst_cdrom_disc_audio_new ();
					}

					cdrom_parse_audio_info (disc_info, GST_CDROM_DISC_AUDIO (disc));
				} else if (g_ascii_strcasecmp (buf, "mixed") == 0) {
					if (!disc) {
						disc = gst_cdrom_disc_mixed_new ();
					} else if (!GST_IS_CDROM_DISC_MIXED (disc)) {
						g_object_unref (G_OBJECT (disc));
						disc = gst_cdrom_disc_mixed_new ();
					}
					
					g_object_get (G_OBJECT (disc), "data", &data,
						      "audio", &audio, NULL);

					if (data)
						cdrom_parse_data_info (disc_info, data, cdrom);
					if (audio)
						cdrom_parse_audio_info (disc_info, audio);
				} else if (g_ascii_strcasecmp (buf, "dvd") == 0) {
					if (!disc) {
						disc = gst_cdrom_disc_dvd_new ();
					} else if (!GST_IS_CDROM_DISC_DVD (disc)) {
						g_object_unref (G_OBJECT (disc));
						disc = gst_cdrom_disc_dvd_new ();
					}

					/* cdrom_parse_dvd_info (disc_info, GST_CDROM_DISC_DVD (disc)); */ /* TODO */
				} else if (g_ascii_strcasecmp (buf, "blank") == 0) {
					if (disc)
						g_object_unref (G_OBJECT (disc));
					disc = NULL;
					/* TODO */
				} else {
					if (disc)
						g_object_unref (G_OBJECT (disc));
					disc = NULL;
				}
				g_free (buf);
			}
		}
		gst_xml_doc_destroy (xml);
	}
	
	return GST_CDROM_DISC (disc);
}

void
gst_disks_get_disk_info_from_xml (GstDisksStorageDisk *disk)
{
	xmlDoc *xml;
	xmlNodePtr root;
	gchar *device;
	gboolean present;
	
	g_return_if_fail (GST_IS_DISKS_STORAGE_DISK (disk));

	g_object_get (G_OBJECT (disk), "device", &device,
		      "present", &present, NULL);
	
	xml = gst_tool_run_get_directive (tool, NULL, "disk_info",
					  device,
					  present ? "1" : "0",
					  NULL);
	if (!xml) {
		return;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		transfer_xml_common_to_config (GST_DISKS_STORAGE (disk), root);
		transfer_xml_disk_to_config (GST_DISKS_STORAGE (disk), root);
	}

	gst_xml_doc_destroy (xml);
}
