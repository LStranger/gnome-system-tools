/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of disks-admin, a gnome-system-tool frontend 
 * for disks administration.
 * 
 * Copyright (C) 2001 Ximian, Inc.
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
 *          Carlos Garcia Campos <elkalmail@yahoo.es>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "gst.h"

#include "disks-config.h"
#include "disks-storage.h"
#include "disks-storage-disk.h"
#include "disks-storage-cdrom.h"
#include "disks-partition.h"
#include "disks-gui.h"
#include "callbacks.h"

extern GstTool *tool;

enum {
	TAB_PROPERTIES,
	TAB_PARTITIONS
};

enum {
	TAB_PROP_DISK,
	TAB_PROP_CDROM
};

void
gst_on_storage_list_selection_change (GtkWidget *selection, gpointer gdata)
{
	GtkWidget        *notebook;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreeSelection *selec;
	GstDisksStorage  *storage;
	GtkWidget        *storage_icon, *storage_label;
	gchar            *icon, *st_model, *device;
	GtkWidget        *treeview;
	GList            *partitions;
	gulong            size;
	GtkWidget        *properties_notebook;

	notebook = gst_dialog_get_widget (tool->main_dialog, "main_notebook");
	
	if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection), &model, &iter)) {
		gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &storage, -1);
		if (GST_IS_DISKS_STORAGE (storage)) {
			g_object_get (G_OBJECT (storage),
				      "model", &st_model,
				      "device", &device,
				      "icon_name", &icon,
				      "size", &size,
				      NULL);
				      
			storage_icon = gst_dialog_get_widget (tool->main_dialog, "storage_icon");
			storage_label = gst_dialog_get_widget (tool->main_dialog, "storage_label");

			gtk_image_set_from_pixbuf (GTK_IMAGE (storage_icon),
						   gst_storage_get_icon (icon));

			if (size > 0) {
				gtk_label_set_markup (GTK_LABEL (storage_label), 
						      g_strdup_printf ("<b>%s</b>\n%s\n<small><i>%s</i></small>",
								       st_model,
								       gst_storage_get_human_readable_size (size),
								       device));
			} else {
				gtk_label_set_markup (GTK_LABEL (storage_label), 
						      g_strdup_printf ("<b>%s</b>\n<small><i>%s</i></small>",
								       st_model, device));
			}
			
			/* Properties Notebook */
			properties_notebook = gst_dialog_get_widget (tool->main_dialog,
								     "properties_notebook");
			if (GST_IS_DISKS_STORAGE_DISK (storage)) {
				gtk_widget_show (properties_notebook);
				gtk_notebook_set_current_page (GTK_NOTEBOOK (properties_notebook),
							       TAB_PROP_DISK);
				
				gst_disks_storage_setup_properties_widget (storage);
			} else if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
				gtk_widget_show (properties_notebook);
				gtk_notebook_set_current_page (GTK_NOTEBOOK (properties_notebook),
							       TAB_PROP_CDROM);
				
				gst_disks_storage_setup_properties_widget (storage);
			} else {
				gtk_widget_hide (properties_notebook);
			}

			if (GST_IS_DISKS_STORAGE_DISK (storage)) {		
				g_object_get (G_OBJECT (storage), "partitions", &partitions, NULL);
				
				treeview = gst_dialog_get_widget (tool->main_dialog, "partition_list");
				model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
				if (partitions) {
					gst_disks_gui_setup_partition (treeview, partitions);
				
					selec = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
					gtk_tree_model_get_iter_first (model, &iter);
					gtk_tree_selection_select_iter (selec, &iter);
				} else {
					gtk_tree_store_clear (GTK_TREE_STORE (model));
					gst_on_partition_list_selection_change (
						gtk_tree_view_get_selection (
							GTK_TREE_VIEW (treeview)),
						NULL);
				}
				
				gtk_widget_show_all (gtk_notebook_get_nth_page (
							     GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
			}
			else {
				gtk_widget_hide (gtk_notebook_get_nth_page (
							 GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
			}
		}
	}
}

static void
gst_partition_properties_refresh (GstDisksPartition *part)
{
	GtkWidget *device_label;
	GtkWidget *point_entry;
	GtkWidget *size_progress;
	GtkWidget *fs_label;
	GtkWidget *mount_button, *status_label;
	GtkWidget *change_mp_button, *part_browse_button;
	gchar *point, *device;
	GstPartitionTypeFs type;
	gulong size, free;
	gboolean mounted, listed;

	point_entry = gst_dialog_get_widget (tool->main_dialog, "point_entry");
	size_progress = gst_dialog_get_widget (tool->main_dialog, "size_progress");
	fs_label = gst_dialog_get_widget (tool->main_dialog, "fs_label");
	device_label = gst_dialog_get_widget (tool->main_dialog, "device_label");
	mount_button = gst_dialog_get_widget (tool->main_dialog, "mount_button");
	status_label = gst_dialog_get_widget (tool->main_dialog, "status_label");
	part_browse_button = gst_dialog_get_widget (tool->main_dialog, "part_browse_button");
	change_mp_button = gst_dialog_get_widget (tool->main_dialog, "change_mp_button");

	if (part) {
		g_object_get (G_OBJECT (part), "type", &type, "point", &point,
			      "size", &size, "device", &device,
			      "free", &free, "mounted", &mounted,
			      "listed", &listed, NULL);
		
		gtk_widget_set_sensitive (size_progress, TRUE);
		gtk_widget_set_sensitive (device_label, TRUE);
		
		if (point)
			gtk_entry_set_text (GTK_ENTRY (point_entry), point);
		else
			gtk_entry_set_text (GTK_ENTRY (point_entry), "");
		
		if (type == PARTITION_TYPE_SWAP) {
			gtk_widget_set_sensitive (change_mp_button, FALSE);
			gtk_widget_set_sensitive (mount_button, FALSE);
			gtk_widget_set_sensitive (part_browse_button, FALSE);
			gtk_editable_set_editable (GTK_EDITABLE (point_entry), FALSE);
		} else {
			gtk_widget_set_sensitive (change_mp_button, TRUE);
			gtk_widget_set_sensitive (mount_button, TRUE);
			gtk_editable_set_editable (GTK_EDITABLE (point_entry), TRUE);
		}
		
		gtk_label_set_text (GTK_LABEL (device_label), device);
		
		gtk_label_set_text (GTK_LABEL (fs_label),
				    gst_disks_partition_get_human_readable_typefs (type));
		
		gst_disks_gui_setup_mounted (status_label, mount_button, mounted);
		
		if (mounted) {
			if (type != PARTITION_TYPE_SWAP)
				gtk_widget_set_sensitive (part_browse_button, TRUE);
			
			gtk_progress_bar_set_fraction (
				GTK_PROGRESS_BAR (size_progress),
				(1 - ((gfloat)(free) / size)));
			
			if (free) {
				gtk_progress_bar_set_text (
					GTK_PROGRESS_BAR (size_progress),
					g_strdup_printf ("%s (%s Free)",
							 gst_storage_get_human_readable_size (size),
							 gst_storage_get_human_readable_size (free))
					);
			}
			else {
				gtk_progress_bar_set_text (
					GTK_PROGRESS_BAR (size_progress),
					g_strdup_printf ("%s",
							 gst_storage_get_human_readable_size (size))
					);
			}
		}
		else {
			gtk_widget_set_sensitive (part_browse_button, FALSE);
			
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
			gtk_progress_bar_set_text (
				GTK_PROGRESS_BAR (size_progress),
				g_strdup_printf ("%s (Free space not available)",
						 gst_storage_get_human_readable_size (size)));
		}
	} else {
		gtk_entry_set_text (GTK_ENTRY (point_entry), "");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), "");
		gtk_label_set_text (GTK_LABEL (fs_label), "");
		gtk_label_set_text (GTK_LABEL (device_label), "");

		gtk_widget_set_sensitive (size_progress, FALSE);
		gtk_widget_set_sensitive (device_label, FALSE);
		gtk_widget_set_sensitive (change_mp_button, FALSE);
		gtk_widget_set_sensitive (mount_button, FALSE);
		gtk_widget_set_sensitive (part_browse_button, FALSE);
	}
}

void 
gst_on_partition_list_selection_change (GtkTreeSelection *selection, gpointer gdata)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GstDisksPartition *part = NULL;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
		if (GST_IS_DISKS_PARTITION (part)) {
			gst_partition_properties_refresh (part);
		}
	} else {
		gst_partition_properties_refresh (part); /* part == NULL */
	}
}

void
gst_on_point_entry_changed (GtkWidget *entry, gpointer gdata)
{
	GtkWidget        *treeview;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreeSelection *selection;
	GstDisksPartition *part;
	const gchar *point;

	treeview = (GtkWidget *) gdata;

	point = gtk_entry_get_text (GTK_ENTRY (entry));

	if (point == NULL || strlen (point) <= 0)
		return;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
		if (GST_IS_DISKS_PARTITION (part)) {
			g_object_set (G_OBJECT (part), "point", point, NULL);
		}
	}
}

static gboolean
gst_disks_partition_mount (GstDisksPartition *part)
{
	xmlDoc *xml;
	xmlNodePtr root, part_node, node;
	gchar *device, *typefs, *point;
	gboolean mounted, listed;
	gchar *buf;
	gboolean error;
	GstPartitionTypeFs type;

	g_object_get (G_OBJECT (part), "type", &type, "point", &point,
		      "device", &device, "mounted", &mounted,
		      "listed", &listed, NULL);
	typefs = gst_disks_partition_get_typefs (type);

	xml = gst_tool_run_get_directive (tool, NULL, "mount",
					  device, typefs, point,
					  mounted ? "1" : "0",
					  listed  ? "1" : "0",
					  NULL);
	if (!xml) {
		return FALSE;
	}

	error = FALSE;
	root = gst_xml_doc_get_root (xml);
	if (root) {
		buf = gst_xml_get_child_content (root, "error");
		if (buf) {
			error = TRUE;
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
			
			return !error;
		}
	} /*else {
		return FALSE;
		}*/

	return FALSE;
}
			
void
gst_on_mount_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget        *treeview;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreeSelection *selection;
	GtkWidget        *status_label;
	GstDisksPartition *part;
	gboolean mounted;

	treeview = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
		if (GST_IS_DISKS_PARTITION (part)) {
			if (gst_disks_partition_mount (part)) {
				status_label = gst_dialog_get_widget (tool->main_dialog, "status_label");
				g_object_get (G_OBJECT (part), "mounted", &mounted, NULL);
			}
			gst_partition_properties_refresh (part);
		}
	}
}

void
gst_on_browse_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget        *treeview;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreeSelection *selection;
	GstDisksPartition *part;
	gchar *point, *browser;

	treeview = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
		if (GST_IS_DISKS_PARTITION (part)) {
			g_object_get (G_OBJECT (part), "point", &point, NULL);
			if (point) {
				if ((browser = g_find_program_in_path ("nautilus"))) {
					g_spawn_command_line_async (
						g_strdup_printf ("%s %s", browser, point),
						NULL);
					g_free (browser);
				}
			}
		}
	}
}

gboolean
gst_on_partition_list_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer gdata)
{
	GtkTreePath *path;
	GtkItemFactory *factory;

	factory = (GtkItemFactory *) gdata;

	if (event->button == 3)
	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL))
		{
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			gtk_item_factory_popup (factory, event->x_root, event->y_root,
						event->button, event->time);
		}
		return TRUE;
	}
	return FALSE;
}

void
gst_disks_storage_get_device_speed_cb (GstDirectiveEntry *entry)
{
	GtkWidget *speed_label;
	xmlDoc *xml;
	xmlNodePtr root;
	gchar *device, *media;
	gchar *buf = NULL;
	GstDisksStorage *storage;

	storage = (GstDisksStorage *) entry->data;

	if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
		media = g_strdup ("cdrom");
	} else if (GST_IS_DISKS_STORAGE_DISK (storage)) {
		media = g_strdup ("disk");
	} else
		return;

	speed_label = gst_dialog_get_widget (tool->main_dialog,
					     g_strdup_printf ("%s_speed_label", media));

	g_object_set (G_OBJECT (storage), "speed", _("Getting ..."), NULL);
	gtk_label_set_text (GTK_LABEL (speed_label), _("Getting ..."));

	g_object_get (G_OBJECT (storage), "device", &device, NULL);
	xml = gst_tool_run_get_directive (entry->tool, entry->report_sign,
					  entry->directive, device, NULL);

	if (!xml) {
		g_object_set (G_OBJECT (storage), "speed", _("Not Available"), NULL);
		gtk_label_set_text (GTK_LABEL (speed_label), _("Not Available"));
		return;
	}

	root = gst_xml_doc_get_root (xml);
	if (root) {
		buf = gst_xml_get_child_content (root, "speed");
	}
	gst_xml_doc_destroy (xml);

	if (buf) {
		g_object_set (G_OBJECT (storage), "speed", buf, NULL);
		gtk_label_set_text (GTK_LABEL (speed_label), g_strdup (buf));
		g_free (buf);
	} else {
		g_object_set (G_OBJECT (storage), "speed", _("Not Available"), NULL);
		gtk_label_set_text (GTK_LABEL (speed_label), _("Not Available"));
	}

	g_free (media);
}

static void
gst_change_mount_point (GtkWidget *button, gpointer gdata)
{
	GtkWidget *filesel;
	GtkWidget *point_entry;

	filesel = (GtkWidget *) gdata;

	point_entry = gst_dialog_get_widget (tool->main_dialog, "point_entry");

	gtk_entry_set_text (GTK_ENTRY (point_entry),
			    gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel)));
}

void
gst_on_change_mp_button_clicked (GtkWidget *w, gpointer gdata)
{
	GtkWidget *filesel;

	filesel = gtk_file_selection_new (_("Select New Mount Point Path"));

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			  "clicked", G_CALLBACK (gst_change_mount_point),
			  (gpointer) filesel);
	
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
				  "clicked", G_CALLBACK (gtk_widget_destroy),
				  (gpointer) filesel);
	
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
				 "clicked", G_CALLBACK (gtk_widget_destroy),
				 (gpointer) filesel);

	gtk_widget_show (filesel);
}
