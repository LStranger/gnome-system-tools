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
#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-partition.h"
#include "disks-gui.h"
#include "callbacks.h"

extern GstTool *tool;

enum {
	TAB_PROPERTIES,
	TAB_PARTITIONS,
	TAB_CDROM_DISC
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
	gboolean          cd_empty;
	GstCdromDisc     *disc;
	GtkWidget        *properties_notebook;

	notebook = gst_dialog_get_widget (tool->main_dialog, "main_notebook");
	
	if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection), &model, &iter)) {
		gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &storage, -1);

		/* Common properties */
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
				gtk_label_set_markup (
					GTK_LABEL (storage_label), 
					g_strdup_printf ("<b>%s</b>\n%s\n<small><i>%s</i></small>",
							 st_model,
							 gst_storage_get_human_readable_size (size),
							 device));
			} else {
				gtk_label_set_markup (
					GTK_LABEL (storage_label), 
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

				g_object_get (G_OBJECT (storage), "partitions", &partitions, NULL);

				treeview = gst_dialog_get_widget (tool->main_dialog, "partition_list");
				model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
				
				if (partitions) {
					gst_disks_gui_setup_partition_list (treeview, partitions);

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

				gtk_widget_hide (gtk_notebook_get_nth_page (
							 GTK_NOTEBOOK (notebook), TAB_CDROM_DISC));
				gtk_widget_show_all (gtk_notebook_get_nth_page (
							     GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
				
			} else if (GST_IS_DISKS_STORAGE_CDROM (storage)) {
				gtk_widget_show (properties_notebook);
				gtk_notebook_set_current_page (GTK_NOTEBOOK (properties_notebook),
							       TAB_PROP_CDROM);
				
				gst_disks_storage_setup_properties_widget (storage);

				g_object_get (G_OBJECT (storage), "empty", &cd_empty,
					      "disc", &disc, NULL);

				if (!cd_empty) {
					gtk_widget_show (gtk_notebook_get_nth_page (
								 GTK_NOTEBOOK (notebook),
								 TAB_CDROM_DISC));
				} else {
					gtk_widget_hide (gtk_notebook_get_nth_page (
								 GTK_NOTEBOOK (notebook),
								 TAB_CDROM_DISC));
				}
				
				gtk_widget_hide (gtk_notebook_get_nth_page (
							 GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
			} else {
				gtk_widget_hide (gtk_notebook_get_nth_page (
							 GTK_NOTEBOOK (notebook), TAB_PARTITIONS));
				gtk_widget_hide (gtk_notebook_get_nth_page (
							 GTK_NOTEBOOK (notebook), TAB_CDROM_DISC));
				gtk_widget_hide (properties_notebook);
			}
		}
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
			gst_disks_partition_setup_properties_widget (part);
		}
	} else {
		gst_disks_partition_setup_properties_widget (part); /* part == NULL */
	}
}

void
gst_on_point_entry_changed (GtkWidget *entry, gpointer gdata)
{
	GtkWidget            *treeview;
	GtkTreeModel         *model;
	GtkTreeIter           iter;
	GtkTreeSelection     *selection;
	GstDisksPartition    *part;
	GstDisksStorageCdrom *cdrom;
	GstCdromDiscData     *disc;
	const gchar *point;

	treeview = (GtkWidget *) gdata;

	point = gtk_entry_get_text (GTK_ENTRY (entry));

	if (point == NULL || strlen (point) <= 0)
		return;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		if (gtk_tree_model_get_n_columns (model) == PARTITION_LIST_LAST) {
			gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
			if (GST_IS_DISKS_PARTITION (part)) {
				g_object_set (G_OBJECT (part), "point", point, NULL);
			}
		} else if (gtk_tree_model_get_n_columns (model) == STORAGE_LIST_LAST) {
			gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
			if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
				g_object_get (G_OBJECT (cdrom), "disc", &disc, NULL);
				if (GST_IS_CDROM_DISC_DATA (disc))
					g_object_set (G_OBJECT (disc), "mount-point",
						      point, NULL);
			}
		}
	}
}

void
gst_on_mount_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget            *treeview;
	GtkTreeModel         *model;
	GtkTreeIter           iter;
	GtkTreeSelection     *selection;
	GstDisksPartition    *part;
	GstDisksStorageCdrom *cdrom;

	treeview = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		if (gtk_tree_model_get_n_columns (model) == PARTITION_LIST_LAST) {
			gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);

			if (GST_IS_DISKS_PARTITION (part)) {
				gst_disks_partition_mount (part);
				
				gst_disks_partition_setup_properties_widget (part);
			}
		} else if (gtk_tree_model_get_n_columns (model) == STORAGE_LIST_LAST) {
			gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
			if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
				gst_disks_cdrom_mount (cdrom);

				gst_disks_storage_setup_properties_widget (GST_DISKS_STORAGE (cdrom));
			}
			
		}
	}
}

void
gst_on_browse_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget            *treeview;
	GtkTreeModel         *model;
	GtkTreeIter           iter;
	GtkTreeSelection     *selection;
	GstDisksPartition    *part;
	GstDisksStorageCdrom *cdrom;
	GstCdromDiscData     *disc;

	treeview = (GtkWidget *) gdata;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		if (gtk_tree_model_get_n_columns (model) == PARTITION_LIST_LAST) {
			gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
			if (GST_IS_DISKS_PARTITION (part)) {
				gst_disks_partition_browse (part);
			}
		} else if (gtk_tree_model_get_n_columns (model) == STORAGE_LIST_LAST) {
			gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &cdrom, -1);
			if (GST_IS_DISKS_STORAGE_CDROM (cdrom)) {
				g_object_get (G_OBJECT (cdrom), "disc", &disc, NULL);
				if (GST_IS_CDROM_DISC_DATA (disc))
					gst_disks_cdrom_disc_data_browse (disc);
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
gst_change_part_mount_point (GtkWidget *button, gpointer gdata)
{
	GtkWidget *filesel;
	GtkWidget *point_entry;

	filesel = (GtkWidget *) gdata;

	point_entry = gst_dialog_get_widget (tool->main_dialog, "part_point_entry");

	gtk_entry_set_text (GTK_ENTRY (point_entry),
			    gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel)));
}

static void
gst_change_cdrom_disc_mount_point (GtkWidget *button, gpointer gdata)
{
	GtkWidget *filesel;
	GtkWidget *point_entry;

	filesel = (GtkWidget *) gdata;

	point_entry = gst_dialog_get_widget (tool->main_dialog, "cd_point_entry");

	gtk_entry_set_text (GTK_ENTRY (point_entry),
			    gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel)));
}

void
gst_on_change_mp_button_clicked (GtkWidget *button, gpointer gdata)
{
	GtkWidget *filesel;

	filesel = gtk_file_selection_new (_("Select New Mount Point Path"));

	if (g_ascii_strcasecmp (gtk_widget_get_name (button), "cd_change_mp_button") == 0) {
		g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
				  "clicked", G_CALLBACK (gst_change_cdrom_disc_mount_point),
				  (gpointer) filesel);
	} else {
		g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
				  "clicked", G_CALLBACK (gst_change_part_mount_point),
				  (gpointer) filesel);
	}
	
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
				  "clicked", G_CALLBACK (gtk_widget_destroy),
				  (gpointer) filesel);
	
	g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
				 "clicked", G_CALLBACK (gtk_widget_destroy),
				 (gpointer) filesel);

	gtk_widget_show (filesel);
}
