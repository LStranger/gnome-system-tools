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
#include "disks-storage-partition.h"
#include "disks-gui.h"
#include "callbacks.h"

extern GstTool *tool;

enum {
	TAB_PROPERTIES,
	TAB_PARTITIONS
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
	GtkWidget        *properties, *scrolled_properties, *scrolled_viewport, *hbox_properties;
	GtkWidget        *label_properties;
	GList            *box_children;

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
			
			/* Properties widget */
			hbox_properties = gst_dialog_get_widget (tool->main_dialog, "hbox_properties");
			properties = gst_disks_storage_get_properties_widget (storage);

			if (properties) {
				box_children = gtk_container_get_children (
					GTK_CONTAINER (hbox_properties));
				if (box_children) {
					g_object_ref (G_OBJECT (box_children->data));
					gtk_container_remove (GTK_CONTAINER (hbox_properties),
							      GTK_WIDGET (box_children->data));

					g_object_ref (G_OBJECT (properties));
					gtk_widget_unparent (properties);
					gtk_box_pack_start (GTK_BOX (hbox_properties), properties,
							    TRUE, TRUE, 0);
					gst_disks_storage_setup_properties_widget (storage);
				} else {
					g_object_ref (G_OBJECT (properties));
					gtk_widget_unparent (properties);
					gtk_box_pack_start (GTK_BOX (hbox_properties), properties,
							    TRUE, TRUE, 0);
					gst_disks_storage_setup_properties_widget (storage);
				}
			} else {
				box_children = gtk_container_get_children (
					GTK_CONTAINER (hbox_properties));
				if (box_children) {
					g_object_ref (G_OBJECT (box_children->data));
					gtk_container_remove (GTK_CONTAINER (hbox_properties),
							      GTK_WIDGET (box_children->data));
				}
			}
			
			gtk_widget_show_all (hbox_properties);

			if (GST_IS_DISKS_STORAGE_DISK (storage)) {		
				g_object_get (G_OBJECT (storage), "children", &partitions, NULL);
				
				treeview = gst_dialog_get_widget (tool->main_dialog, "partition_list");
				model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
				if (partitions) {
					gst_storage_partition_gui_setup (treeview, partitions);
				
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

void 
gst_on_partition_list_selection_change (GtkTreeSelection *selection, gpointer gdata)
{
	GtkWidget       *notebook;
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GstDisksStoragePartition *part;
	GtkWidget *device_label;
	GtkWidget *point_entry;
	GtkWidget *size_progress;
	GtkWidget *properties_table;
	GtkWidget *fs_option_menu;
	gchar *point, *device;
	GstPartitionType type;
	gulong size, free;
	gboolean mounted, listed;

	point_entry = gst_dialog_get_widget (tool->main_dialog, "point_entry");
	size_progress = gst_dialog_get_widget (tool->main_dialog, "size_progress");
	fs_option_menu = gst_dialog_get_widget (tool->main_dialog, "fs_option_menu");
	device_label = gst_dialog_get_widget (tool->main_dialog, "device_label");

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, PARTITION_LIST_POINTER, &part, -1);
		if (GST_IS_DISKS_STORAGE_PARTITION (part)) {
			g_object_get (G_OBJECT (part), "type", &type, "point", &point,
				      "size", &size, "device", &device,
				      "free", &free, "mounted", &mounted,
				      "listed", &listed, NULL);

			/* gtk_widget_set_sensitive (point_entry, TRUE); */
			gtk_widget_set_sensitive (size_progress, TRUE);
			/* gtk_widget_set_sensitive (fs_option_menu, TRUE); */
			gtk_widget_set_sensitive (device_label, TRUE);

			if (point)
				gtk_entry_set_text (GTK_ENTRY (point_entry), point);
			else
				gtk_entry_set_text (GTK_ENTRY (point_entry), g_strdup (""));

			gtk_label_set_text (GTK_LABEL (device_label), device);
			
			gtk_option_menu_set_history (GTK_OPTION_MENU (fs_option_menu), type);
			
			properties_table = gst_dialog_get_widget (tool->main_dialog,
								  "partition_properties_table");
			if (mounted) {
				gtk_widget_set_sensitive (properties_table, TRUE);
				
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
			else if (listed) {
				gtk_widget_set_sensitive (properties_table, TRUE);

				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
				gtk_progress_bar_set_text (
					GTK_PROGRESS_BAR (size_progress),
					g_strdup_printf ("%s (Free space not available)",
							 gst_storage_get_human_readable_size (size)));
			}
			else {
				gtk_widget_set_sensitive (properties_table, FALSE);

				gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
				gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), "");
			}
		}
	} else { /* There is no partitions */
		gtk_entry_set_text (GTK_ENTRY (point_entry), "");
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), "");
		gtk_option_menu_set_history (GTK_OPTION_MENU (fs_option_menu), PARTITION_TYPE_UNKNOWN);
		gtk_label_set_text (GTK_LABEL (device_label), "");

		/* gtk_widget_set_sensitive (point_entry, FALSE); */
		gtk_widget_set_sensitive (size_progress, FALSE);
		/* gtk_widget_set_sensitive (fs_option_menu, FALSE); */
		gtk_widget_set_sensitive (device_label, FALSE);
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
	xmlDoc *xml;
	xmlNodePtr root, speed;
	gchar *buf, *device, *value, *media;
	GtkWidget *speed_label;
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

	g_object_set (G_OBJECT (storage), "speed", _("(Getting ...)"), NULL);
	gtk_label_set_text (GTK_LABEL (speed_label), _("(Getting ...)"));

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


void 
gst_disks_storage_update_device_speed (GtkWidget *w, gpointer gdata)
{
	GstDisksStorage *storage;
	GtkWidget *treeview;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	treeview = gst_dialog_get_widget (tool->main_dialog, "storage_list");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection), &model, &iter)) {
		gtk_tree_model_get (model, &iter, STORAGE_LIST_POINTER, &storage, -1);
		if (!GST_IS_DISKS_STORAGE (storage)) {
			return;
		}
	}

	gst_disks_gui_set_device_speed (storage);
}
