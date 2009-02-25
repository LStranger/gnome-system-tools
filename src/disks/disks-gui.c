/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* disks-gui.c: this file is part of disks-admin, a gnome-system-tool frontend 
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

#include <glib/gi18n.h>

#include "gst.h"

#include "gst-disks-tool.h"
#include "disks-storage.h"
#include "disks-partition.h"
#include "disks-storage-disk.h"
#include "disks-storage-cdrom.h"
#include "disks-cdrom-disc.h"
#include "disks-cdrom-disc-data.h"
#include "disks-cdrom-disc-audio.h"
#include "disks-gui.h"
#include "callbacks.h"
#include "transfer.h"

extern GstTool *tool;

gchar *
gst_storage_get_human_readable_size (const gulong size)
{
	if (size == 0)
		return g_strdup ("");
	if ((size / 1024) <= 1024)
		return g_strdup_printf ("%2.2f MiB", (gfloat) size / 1024);
	if ((size / 1024) > 1024)
		return g_strdup_printf ("%2.2f GiB", (gfloat) size / (1024 * 1024));
	
	return NULL;
}

gfloat 
gst_storage_get_float_size (const gulong size)
{
	if (size == 0)
		return 0.0;
	if ((size / 1024) <= 1024)
		return ((gfloat) size / 1024);
	if ((size / 1024) > 1024)
		return ((gfloat) size / (1024 * 1024));

	return (gfloat) 0.0;
}

GdkPixbuf *
gst_storage_get_icon (const gchar *icon_name)
{
	GdkPixbuf *pb = NULL;
	gchar *file;
	
	pb = gtk_icon_theme_load_icon (GST_DISKS_TOOL (tool)->icon_theme,
				       icon_name, 48, 0,
				       NULL);
	if (!pb) {
		file = g_strdup_printf (PIXMAPS_DIR"/%s.png", icon_name);
		pb = gdk_pixbuf_new_from_file (file, NULL);
		g_free (file);
	}
	
	return pb;
}

static GtkWidget *
gst_disks_gui_storage_list_new ()
{
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	model = GTK_TREE_MODEL (gtk_tree_store_new (STORAGE_LIST_LAST,
						    GDK_TYPE_PIXBUF,
						    G_TYPE_STRING,
						    G_TYPE_POINTER));
	
	treeview = gst_dialog_get_widget (tool->main_dialog, "storage_list");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (model);

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "pixbuf",
							   STORAGE_LIST_ICON,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, STORAGE_LIST_ICON);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "markup",
							   STORAGE_LIST_NAME,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, STORAGE_LIST_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (gst_on_storage_list_selection_change),
			  NULL);

	return (treeview);
}

static void
gst_disks_gui_setup_storage_list (GtkWidget *treeview, GList *storages)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GList *list = NULL;
	GstDisksStorage *dsk;
	gboolean valid;
	gchar *icon, *name, *text_name, *hr_size;
	gulong size;

	g_return_if_fail (treeview != NULL);
	g_return_if_fail (storages != NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	list = g_list_first (storages);
	while (list) {
		dsk = list->data;
		if (GST_IS_DISKS_STORAGE (dsk)) {
			g_object_get (G_OBJECT (dsk), "icon_name", &icon,
				      "size", &size, "name", &name,
				      NULL);

			if (!valid)
				gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);

			hr_size = gst_storage_get_human_readable_size (size);
			text_name = g_strdup_printf ("<b>%s</b>\n%s", name, hr_size);
			if (hr_size) g_free (hr_size);
			
			gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
					    STORAGE_LIST_ICON,
					    gst_storage_get_icon (icon),
					    STORAGE_LIST_NAME, text_name,
					    STORAGE_LIST_POINTER, dsk,
					    -1);
			g_free (text_name);
		}
		list = g_list_next (list);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get_iter_first (model, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
	}
}

void
gst_disks_gui_storage_list_reload (GtkWidget *widget, gpointer gdata)
{
	GtkWidget       *treeview;

	treeview = gst_dialog_get_widget (tool->main_dialog, "storage_list");

	gst_disks_gui_setup_storage_list (treeview, (GList *) gdata);
}

static GtkWidget *
gst_disks_gui_partition_list_new ()
{
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *selection;

	model = GTK_TREE_MODEL (gtk_tree_store_new (PARTITION_LIST_LAST,
						    G_TYPE_STRING,
						    G_TYPE_POINTER));

	treeview = gst_dialog_get_widget (tool->main_dialog, "partition_list");
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (model);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "text",
							   PARTITION_LIST_NAME,
							   NULL);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, PARTITION_LIST_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (gst_on_partition_list_selection_change),
			  NULL);

	return (treeview);
}


void 
gst_disks_gui_setup_partition_list (GtkWidget *treeview, GList *partitions)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *list;
	GstDisksPartition *part;
	gchar *device, *name;
	GstPartitionTypeFs type;

	g_return_if_fail (treeview != NULL);
	g_return_if_fail (partitions != NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	gtk_tree_store_clear (GTK_TREE_STORE (model));
	
	list = g_list_first (partitions);
	while (list) {
		part = GST_DISKS_PARTITION (list->data);
		if (GST_IS_DISKS_PARTITION (part)) {
			g_object_get (G_OBJECT (part), "device", &device,
				      "name", &name, "type", &type, NULL);

			gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
			gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
					    PARTITION_LIST_NAME, name,
					    PARTITION_LIST_POINTER, part,
					    -1);
		}
		list = g_list_next (list);
	}
}

static void
gst_disks_gui_setup_mounted (GtkWidget *status_label, GtkWidget *mount_button, gboolean mounted)
{
	GtkWidget *icon, *label;

	if (g_ascii_strcasecmp (gtk_widget_get_name (mount_button), "cd_mount_button") == 0) {
		icon = gst_dialog_get_widget (tool->main_dialog, "cd_mount_button_icon");
		label = gst_dialog_get_widget (tool->main_dialog, "cd_mount_button_label");
	} else {
		icon = gst_dialog_get_widget (tool->main_dialog, "part_mount_button_icon");
		label = gst_dialog_get_widget (tool->main_dialog, "part_mount_button_label");
	}

	if (mounted) {
		gtk_label_set_text (GTK_LABEL (status_label), _("Accessible"));
		gtk_label_set_label (GTK_LABEL (label), _("_Disable"));
		gtk_image_set_from_stock (GTK_IMAGE (icon), GTK_STOCK_UNDO,
					  GTK_ICON_SIZE_BUTTON);
	} else {
		gtk_label_set_text (GTK_LABEL (status_label), _("Inaccessible"));
		gtk_label_set_label (GTK_LABEL (label), _("_Enable"));
		gtk_image_set_from_stock (GTK_IMAGE (icon), GTK_STOCK_REDO,
					  GTK_ICON_SIZE_BUTTON);
	}
}

static void
gst_disks_gui_setup_format_filesys_combo (GtkWidget *combo, GstPartitionTypeFs type)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	gint i, j, current = -1;
	gchar *text_fs;
	gboolean valid, found;
	GstPartitionTypeFsInfo *table;

	table = gst_disks_partition_get_type_fs_info_table ();
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (!model || !gtk_tree_model_get_iter_first (model, &iter)) {
		/* There is no previous model */
		model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT));
		gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);

		j = -1;
		for (i=0; i < PARTITION_TYPE_NUM; i++) {
			if (table[i].fs_format_command &&
			    g_find_program_in_path (table[i].fs_format_command)) {
				gtk_list_store_append (GTK_LIST_STORE (model), &iter);
				gtk_list_store_set (GTK_LIST_STORE (model), &iter,
						    0, table[i].fs_hr_name,
						    1, i,
						    -1);
				j++;

				text_fs = gst_disks_partition_get_typefs (type);
				
				if (g_ascii_strcasecmp (text_fs, table[i].fs_name) == 0)
					current = j;
				
				if (text_fs) g_free (text_fs);
			}
		}

		g_object_unref (model);

		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
						"text", 0, NULL);

		if (current == -1) 
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		else
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), current);

	} else {
		valid = gtk_tree_model_get_iter_first (model, &iter);

		i = 0;
		found = FALSE;
		while (valid && !found) {
			gtk_tree_model_get (model, &iter,
					    1, &current,
					    -1);

			if (current == type) {
				gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
				found = TRUE;
			}

			valid = gtk_tree_model_iter_next (model, &iter);

			i++;
		}

		if (!found)
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	}
}

gboolean
gst_disks_gui_get_mount_point_is_busy (const gchar *point)
{
	GList *storages = NULL;
	GList *partitions = NULL;
	GstDisksStorage *disk;
	GstDisksPartition *part;
	gchar *part_point;

	storages = g_list_first (GST_DISKS_TOOL(tool)->storages);

	while (storages) {
		disk = GST_DISKS_STORAGE (storages->data);

		if (GST_IS_DISKS_STORAGE_DISK (disk)) {
			g_object_get (G_OBJECT (disk), "partitions", &partitions, NULL);
			partitions = g_list_first (partitions);

			while (partitions) {
				part = GST_DISKS_PARTITION (partitions->data);
				
				if (GST_IS_DISKS_PARTITION (part)) {
					g_object_get (G_OBJECT (part), "point",
						      &part_point, NULL);
					if (part_point &&
					    g_ascii_strcasecmp (part_point, point) == 0)
						return TRUE;
				}
				
				partitions = g_list_next (partitions);
			}
		}
		
		storages = g_list_next (storages);
	}

	return FALSE;
}

static void
gst_disks_gui_setup_mount_point_combo (GtkWidget *combo, const gchar *point)
{
	GtkTreeModel       *model;
	GtkTreeIter         iter;
	GtkCellRenderer    *renderer;
	GtkEntry           *entry;
	GtkEntryCompletion *completion;
	gint i, j, current = -1;
	static gchar *points[] = {
		"/", "/boot", "/home",
		"/mnt", "/opt", "/root",
		"/usr", "/tmp", "/var",
		NULL
	};
	static gboolean new_combo = TRUE;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (model && GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	} else {
		model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
		gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
		g_object_unref (model);
	}

	j = -1;
	for (i=0; points[i]; i++) {
		if ((point && g_ascii_strcasecmp (points[i], point) == 0) ||
		    (!gst_disks_gui_get_mount_point_is_busy (points[i]))) {
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, points[i],
					    -1);
			j++;

			if (point && g_ascii_strcasecmp (points[i], point) == 0)
				current = j;
		}
	}

	if (new_combo) {
		gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (combo), 0);
		
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_clear (GTK_CELL_LAYOUT (combo));
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
						"text", 0, NULL);

		entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combo)));
		completion = gtk_entry_completion_new ();
		gtk_entry_set_completion (entry, completion);
		g_object_unref (completion);
		gtk_entry_completion_set_model (completion, model);
		gtk_entry_completion_set_text_column (completion, 0);
		g_object_unref (model);
	}

	if (current == -1) {
		if (point && g_ascii_strcasecmp (point, "none") != 0) {
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, point,
					    -1);
			j++;
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), j);
		} else {
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		}
	} else {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), current);
	}
	
	new_combo = FALSE;
}

static void
gst_disks_gui_on_filesys_combo_changed (GtkWidget *combo, gpointer gdata)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GtkWidget       *mp_combo;
	GtkEntry	*entry;
	GstPartitionTypeFs type;

	mp_combo = (GtkWidget *) gdata;
	
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter)) {
		model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
		gtk_tree_model_get (model, &iter,
				    1, &type,
				    -1);

		if (type == PARTITION_TYPE_SWAP) {
			entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (mp_combo)));
			gtk_entry_set_text (entry, _("none"));
			gtk_widget_set_sensitive (mp_combo, FALSE);
		} else {
			gtk_widget_set_sensitive (mp_combo, TRUE);
		}
	}
}

void
gst_disks_gui_setup_format_dialog (GstDisksPartition *part)
{
	GtkWidget *device_label;
	GtkWidget *filesys_combo, *mp_combo;
	GtkWidget *progress;
	gchar     *device, *point;
	GstPartitionTypeFs type;
	static gboolean signals = FALSE;

	g_return_if_fail (GST_IS_DISKS_PARTITION (part));
	
	g_object_get (G_OBJECT (part), "device", &device,
		      "type", &type, "point", &point, NULL);
	
	device_label = gst_dialog_get_widget (tool->main_dialog, "fmt_partition_label");
	filesys_combo = gst_dialog_get_widget (tool->main_dialog, "fmt_filesys_combo");
	mp_combo = gst_dialog_get_widget (tool->main_dialog, "fmt_mount_point_combo");
	progress = gst_dialog_get_widget (tool->main_dialog, "fmt_progress");

	if (!signals) {
		g_signal_connect (G_OBJECT (filesys_combo), "changed",
				  G_CALLBACK (gst_disks_gui_on_filesys_combo_changed),
				  (gpointer) mp_combo);
		signals = TRUE;
	}
	
	gtk_label_set_text (GTK_LABEL (device_label), device);
	gst_disks_gui_setup_format_filesys_combo (filesys_combo, type);
	gst_disks_gui_setup_mount_point_combo (mp_combo, point);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.0);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress), "");
}

void
gst_disks_gui_set_device_speed (GstDisksStorage *storage)
{
	/*gst_tool_queue_directive (tool, gst_disks_storage_get_device_speed_cb,
	  (gpointer) storage, NULL, NULL, "dev_speed");*/
}


void
gst_disks_gui_setup ()
{
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GList *list;
	/* Partition Widgets */
	GtkWidget *part_format_button;
	GtkWidget *part_change_mp_button;
	GtkWidget *part_mount_button, *part_browse_button;
	GtkWidget *part_point_entry;
	/* Cdrom Disc Data Widgets */
	GtkWidget *cd_change_mp_button;
	GtkWidget *cd_mount_button, *cd_browse_button;
	GtkWidget *cd_point_entry;
	/* Cdrom Disc Audio Widgets */
	GtkWidget *cd_play_button;
	/* DVD Widgets */
	GtkWidget *dvd_play_button;

	list = GST_DISKS_TOOL (tool)->storages;

	g_signal_connect (G_OBJECT (GST_DISKS_TOOL (tool)->icon_theme), "changed",
			  G_CALLBACK (gst_disks_gui_storage_list_reload),
			  (gpointer) list);
	
	treeview = gst_disks_gui_partition_list_new ();

	/** Partition Widgets **/
	/* Format button */
	part_format_button = gst_dialog_get_widget (tool->main_dialog, "part_format_button");
	g_signal_connect (G_OBJECT (part_format_button), "clicked",
			  G_CALLBACK (gst_on_format_button_clicked),
			  (gpointer) treeview);
	
	/* Point entry change, we have to update the partition object */
	part_point_entry = gst_dialog_get_widget (tool->main_dialog, "part_point_entry");
	g_signal_connect (G_OBJECT (part_point_entry), "changed",
			  G_CALLBACK (gst_on_point_entry_changed),
			  (gpointer) treeview);

	/* Mount/Umount button clicked */
	part_mount_button = gst_dialog_get_widget (tool->main_dialog, "part_mount_button");
	g_signal_connect (G_OBJECT (part_mount_button), "clicked",
			  G_CALLBACK (gst_on_mount_button_clicked),
			  (gpointer) treeview);

	/* Browse button clicked */
	part_browse_button = gst_dialog_get_widget (tool->main_dialog, "part_browse_button");
	g_signal_connect (G_OBJECT (part_browse_button), "clicked",
			  G_CALLBACK (gst_on_browse_button_clicked),
			  (gpointer) treeview);

	/* Change Mount Point button callback */
	part_change_mp_button = gst_dialog_get_widget (tool->main_dialog, "part_change_mp_button");
	g_signal_connect (G_OBJECT (part_change_mp_button), "clicked",
			  G_CALLBACK (gst_on_change_mp_button_clicked), NULL);

	treeview = gst_disks_gui_storage_list_new ();

	g_signal_connect (G_OBJECT (treeview), "button_press_event",
			  G_CALLBACK (gst_on_storage_list_button_press),
			  NULL);
	
	gst_disks_gui_setup_storage_list (treeview, list);

	/** Cdrom Disc Data Widgets **/
	/* Point entry change, we have to update the CdromDiscData object */
	cd_point_entry = gst_dialog_get_widget (tool->main_dialog, "cd_point_entry");
	g_signal_connect (G_OBJECT (cd_point_entry), "changed",
			  G_CALLBACK (gst_on_point_entry_changed),
			  (gpointer) treeview);

	/* Mount/Umount button clicked */
	cd_mount_button = gst_dialog_get_widget (tool->main_dialog, "cd_mount_button");
	g_signal_connect (G_OBJECT (cd_mount_button), "clicked",
			  G_CALLBACK (gst_on_mount_button_clicked),
			  (gpointer) treeview);

	/* Browse button clicked */
	cd_browse_button = gst_dialog_get_widget (tool->main_dialog, "cd_browse_button");
	g_signal_connect (G_OBJECT (cd_browse_button), "clicked",
			  G_CALLBACK (gst_on_browse_button_clicked),
			  (gpointer) treeview);

	/** Cdrom Disc Audio Widgets **/
	/* Play button clicked */
	cd_play_button = gst_dialog_get_widget (tool->main_dialog, "cd_play_button");
	g_signal_connect (G_OBJECT (cd_play_button), "clicked",
			  G_CALLBACK (gst_on_play_button_clicked),
			  (gpointer) treeview);

	/* Change Mount Point button callback */
	cd_change_mp_button = gst_dialog_get_widget (tool->main_dialog, "cd_change_mp_button");
	g_signal_connect (G_OBJECT (cd_change_mp_button), "clicked",
			  G_CALLBACK (gst_on_change_mp_button_clicked), NULL);

	/** DVD Widgets **/
	/* Play button clicked */
	dvd_play_button = gst_dialog_get_widget (tool->main_dialog, "dvd_play_button");
	g_signal_connect (G_OBJECT (dvd_play_button), "clicked",
			  G_CALLBACK (gst_on_play_button_clicked),
			  (gpointer) treeview);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_model_get_iter_first (model, &iter);
	gtk_tree_selection_select_iter (selection, &iter);
	
	gtk_widget_show_all (treeview);
}

/* Porperties Widgets */

/* All Storages */
void
gst_disks_gui_setup_storage_properties (GstDisksStorage *storage)
{
	GtkWidget *storage_icon, *storage_label;
	gchar     *icon, *model, *device;
	gchar     *text_label, *hr_size;
	gulong     size;

	g_return_if_fail (GST_IS_DISKS_STORAGE (storage));

	g_object_get (G_OBJECT (storage),
		      "model", &model,
		      "device", &device,
		      "icon_name", &icon,
		      "size", &size,
		      NULL);

	storage_icon = gst_dialog_get_widget (tool->main_dialog, "storage_icon");
	storage_label = gst_dialog_get_widget (tool->main_dialog, "storage_label");

	gtk_image_set_from_pixbuf (GTK_IMAGE (storage_icon),
				   gst_storage_get_icon (icon));

	if (size > 0) {
		hr_size = gst_storage_get_human_readable_size (size);
		text_label = g_strdup_printf ("<b>%s</b>\n%s\n<small><i>%s</i></small>",
					      model, hr_size, device);
		if (hr_size) g_free (hr_size);
	} else {
		text_label = g_strdup_printf ("<b>%s</b>\n<small><i>%s</i></small>",
					      model, device);
	}
	
	gtk_label_set_markup (GTK_LABEL (storage_label), text_label);

	g_free (text_label);
}

/* Disk */
void
gst_disks_gui_setup_disk_properties (GstDisksStorageDisk *disk)
{
	gchar *speed, *device, *icon_name;
	gboolean present;

	g_return_if_fail (GST_IS_DISKS_STORAGE_DISK (disk));

	gst_disks_get_disk_info_from_xml (disk);
	
	g_object_get (G_OBJECT (disk), "speed", &speed,
		      "device", &device, "present", &present,
		      "icon_name", &icon_name, NULL);

	if (present) {
		g_object_set (G_OBJECT (disk), "icon_name", "gnome-dev-harddisk", NULL);
	} else {
		g_object_set (G_OBJECT (disk), "icon_name", "gnome-dev-removable", NULL);
	}
	
	gtk_label_set_text (
		GTK_LABEL (gst_dialog_get_widget (tool->main_dialog, "disk_device_label")),
		device);
	
	if (speed == NULL) {
		gst_disks_gui_set_device_speed (GST_DISKS_STORAGE (disk));
	} else {
		gtk_label_set_text (
			GTK_LABEL (gst_dialog_get_widget (tool->main_dialog, "disk_speed_label")),
			speed);
	}

	gst_disks_gui_storage_list_reload (NULL, GST_DISKS_TOOL (tool)->storages);
}

/* Partition */
void 
gst_disks_gui_setup_partition_properties (GstDisksPartition *part)
{
	GtkWidget *device_label, *device_tit_label;
	GtkWidget *point_entry, *point_tit_label;
	GtkWidget *size_progress, *size_tit_label;
	GtkWidget *fs_label, *fs_tit_label, *format_button;
	GtkWidget *mount_button, *status_label, *status_tit_label;
	GtkWidget *change_mp_button, *part_browse_button;
	gchar *point, *device;
	gchar *text_pbar, *text_type_label, *hr_size, *hr_free;
	GstPartitionTypeFs type;
	GstDisksStorageDisk *disk;
	gulong size, free;
	gboolean mounted, listed, disk_present;

	point_entry = gst_dialog_get_widget (tool->main_dialog, "part_point_entry");
	point_tit_label = gst_dialog_get_widget (tool->main_dialog, "part_point_tit_label");
	size_progress = gst_dialog_get_widget (tool->main_dialog, "part_size_progress");
	size_tit_label = gst_dialog_get_widget (tool->main_dialog, "part_size_tit_label");
	fs_label = gst_dialog_get_widget (tool->main_dialog, "part_fs_label");
	fs_tit_label = gst_dialog_get_widget (tool->main_dialog, "part_fs_tit_label");
	format_button = gst_dialog_get_widget (tool->main_dialog, "part_format_button");
	device_label = gst_dialog_get_widget (tool->main_dialog, "part_device_label");
	device_tit_label = gst_dialog_get_widget (tool->main_dialog, "part_device_tit_label");
	mount_button = gst_dialog_get_widget (tool->main_dialog, "part_mount_button");
	status_label = gst_dialog_get_widget (tool->main_dialog, "part_status_label");
	status_tit_label = gst_dialog_get_widget (tool->main_dialog, "part_status_tit_label");
	part_browse_button = gst_dialog_get_widget (tool->main_dialog, "part_browse_button");
	change_mp_button = gst_dialog_get_widget (tool->main_dialog, "part_change_mp_button");

	if (part) {
		g_object_get (G_OBJECT (part), "type", &type, "point", &point,
			      "size", &size, "device", &device,
			      "free", &free, "mounted", &mounted,
			      "listed", &listed, "disk", &disk, NULL);

		g_object_get (G_OBJECT (disk), "present", &disk_present, NULL);
		
		gtk_widget_set_sensitive (size_progress, TRUE);
		gtk_widget_set_sensitive (device_label, TRUE);

		if (point)
			gtk_entry_set_text (GTK_ENTRY (point_entry), point);
		else
			gtk_entry_set_text (GTK_ENTRY (point_entry), _("none"));

		gtk_widget_show (device_tit_label);
		gtk_widget_show (fs_tit_label);
		gtk_widget_show (fs_label);
		gtk_widget_show (format_button);
		gtk_widget_show (point_tit_label);
		gtk_widget_show (point_entry);
		gtk_widget_show (change_mp_button);
		gtk_widget_show (status_tit_label);
		gtk_widget_show (status_label);
		gtk_widget_show (mount_button);
		gtk_widget_show (part_browse_button);
		gtk_widget_show (size_progress);
		gtk_widget_show (size_tit_label);

		gtk_label_set_text (GTK_LABEL (device_label), device);
		
		if (type == PARTITION_TYPE_SWAP) {
			gtk_widget_set_sensitive (change_mp_button, FALSE);
			gtk_widget_set_sensitive (mount_button, FALSE);
			gtk_widget_set_sensitive (part_browse_button, FALSE);
			gtk_editable_set_editable (GTK_EDITABLE (point_entry), FALSE);
			gtk_widget_hide (point_tit_label);
			gtk_widget_hide (point_entry);
			gtk_widget_hide (change_mp_button);
			gtk_widget_hide (status_tit_label);
			gtk_widget_hide (status_label);
			gtk_widget_hide (mount_button);
			gtk_widget_hide (part_browse_button);
		} else if (type == PARTITION_TYPE_FREE) { 
			gtk_widget_hide (device_tit_label);
			gtk_widget_hide (fs_tit_label);
			gtk_widget_hide (fs_label);
			gtk_widget_hide (format_button);
			gtk_widget_hide (point_tit_label);
			gtk_widget_hide (point_entry);
			gtk_widget_hide (change_mp_button);
			gtk_widget_hide (status_tit_label);
			gtk_widget_hide (status_label);
			gtk_widget_hide (mount_button);
			gtk_widget_hide (part_browse_button);
			
			gtk_label_set_text (GTK_LABEL (device_label),
					    _("Free Space, not partitioned"));
		} else {
			gtk_widget_set_sensitive (change_mp_button, TRUE);
			gtk_widget_set_sensitive (mount_button, disk_present);
			gtk_editable_set_editable (GTK_EDITABLE (point_entry), TRUE);
		}

		text_type_label = gst_disks_partition_get_human_readable_typefs (type);
		gtk_label_set_text (GTK_LABEL (fs_label), text_type_label);
		g_free (text_type_label);

		gst_disks_gui_setup_mounted (status_label, mount_button, mounted);

		hr_size = gst_storage_get_human_readable_size (size);
		if (mounted) {
			if (type != PARTITION_TYPE_SWAP)
				gtk_widget_set_sensitive (part_browse_button, TRUE);

			gtk_widget_set_sensitive (format_button, FALSE);

			gtk_progress_bar_set_fraction (
				GTK_PROGRESS_BAR (size_progress),
				(1 - ((gfloat)(free) / size)));

			if (free) {
				hr_size = gst_storage_get_human_readable_size (size);
				hr_free = gst_storage_get_human_readable_size (free);
				text_pbar = g_strdup_printf ("%s (%s Free)", hr_size, hr_free);
				g_free (hr_free);
			} else {
				text_pbar = g_strdup_printf ("%s", hr_size);
			}
			
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), text_pbar);
			g_free (text_pbar);
		} else {
			gtk_widget_set_sensitive (part_browse_button, FALSE);

			if (type == PARTITION_TYPE_SWAP || !disk_present)
				gtk_widget_set_sensitive (format_button, FALSE);
			else
				gtk_widget_set_sensitive (format_button, TRUE);

			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
			
			text_pbar = g_strdup_printf ("%s (Free space not available)", hr_size);
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), text_pbar);
			g_free (text_pbar);
		}
		g_free (hr_size);
	} else {
		gtk_widget_hide (device_tit_label);
		gtk_widget_hide (fs_tit_label);
		gtk_widget_hide (fs_label);
		gtk_widget_hide (format_button);
		gtk_widget_hide (point_tit_label);
		gtk_widget_hide (point_entry);
		gtk_widget_hide (change_mp_button);
		gtk_widget_hide (status_tit_label);
		gtk_widget_hide (status_label);
		gtk_widget_hide (mount_button);
		gtk_widget_hide (part_browse_button);
		gtk_widget_hide (size_progress);
		gtk_widget_hide (size_tit_label);
		gtk_label_set_text (GTK_LABEL (device_label), _("There aren't known partitions in the disk"));
	}
}

/* CDROM */
void
gst_disks_gui_setup_cdrom_properties (GstDisksStorageCdrom *cdrom)
{
	gchar *speed, *device, *icon_name;
	gboolean play_audio, write_cdr, write_cdrw, read_dvd;
	gboolean write_dvdr, write_dvdram;
	GstCdromDisc *disc;

	g_return_if_fail (GST_IS_DISKS_STORAGE_CDROM (cdrom));
	
	g_object_get (G_OBJECT (cdrom), "speed", &speed,
		      "device", &device, "icon_name", &icon_name,
		      NULL);

	gtk_label_set_text (
		GTK_LABEL (gst_dialog_get_widget (tool->main_dialog, "cdrom_device_label")),
		device);

	if (speed == NULL) {
		gst_disks_gui_set_device_speed (GST_DISKS_STORAGE (cdrom));
	} else {
		gtk_label_set_text (
			GTK_LABEL (gst_dialog_get_widget (tool->main_dialog, "cdrom_speed_label")),
			speed);
	}

	disc = gst_disks_cdrom_set_disc (cdrom);
	if (disc)
		gst_cdrom_disc_setup_gui (disc);
	
	gtk_label_set_text (
		GTK_LABEL (gst_dialog_get_widget (tool->main_dialog, "cdrom_status_label")),
		gst_disks_storage_cdrom_get_human_readable_status (cdrom));

	if (GST_IS_CDROM_DISC_AUDIO (disc)) {
		g_object_set (G_OBJECT (cdrom), "icon_name", "gnome-dev-cdrom-audio", NULL);
	} else if (GST_IS_CDROM_DISC_DATA (disc)) {
		g_object_set (G_OBJECT (cdrom), "icon_name", "gnome-dev-cdrom-data", NULL);
	} else if (GST_IS_CDROM_DISC_MIXED (disc)) {
		g_object_set (G_OBJECT (cdrom), "icon_name", "gnome-dev-cdrom-mixed", NULL);
	} else if (GST_IS_CDROM_DISC_DVD (disc)) {
		g_object_set (G_OBJECT (cdrom), "icon_name", "gnome-dev-dvd", NULL);
	} else {
		g_object_set (G_OBJECT (cdrom), "icon_name", "gnome-dev-cdrom", NULL);
	}
	
	gst_disks_gui_storage_list_reload (NULL, GST_DISKS_TOOL (tool)->storages);

	g_object_get (G_OBJECT (cdrom), "play_audio", &play_audio, "write_cdr", &write_cdr,
		      "write_cdrw", &write_cdrw, "read_dvd", &read_dvd, 
		      "write_dvdr", &write_dvdr, "write_dvdram", &write_dvdram, NULL);
	
	gtk_image_set_from_stock (
		GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "play_audio_image")),
		(play_audio) ? GTK_STOCK_YES : GTK_STOCK_NO,
		GTK_ICON_SIZE_MENU);

	gtk_image_set_from_stock (
		GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_cdr_image")),
		(write_cdr) ? GTK_STOCK_YES : GTK_STOCK_NO,
		GTK_ICON_SIZE_MENU);
	
	gtk_image_set_from_stock (
		GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_cdrw_image")),
		(write_cdrw) ? GTK_STOCK_YES : GTK_STOCK_NO,
		GTK_ICON_SIZE_MENU);

	gtk_image_set_from_stock (
		GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "read_dvd_image")),
		(read_dvd) ? GTK_STOCK_YES : GTK_STOCK_NO,
		GTK_ICON_SIZE_MENU);
	
	gtk_image_set_from_stock (
		GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_dvdr_image")),
		(write_dvdr) ? GTK_STOCK_YES : GTK_STOCK_NO,
		GTK_ICON_SIZE_MENU);
	
	gtk_image_set_from_stock (
		GTK_IMAGE (gst_dialog_get_widget (tool->main_dialog, "write_dvdram_image")),
		(write_dvdram) ? GTK_STOCK_YES : GTK_STOCK_NO,
		GTK_ICON_SIZE_MENU);
	
}
	
/* CDROM Disc Data */
void 
gst_disks_gui_setup_cdrom_disc_data (GstCdromDiscData *disc_data)
{
	GtkWidget *point_entry;
	GtkWidget *size_progress;
	GtkWidget *mount_button, *status_label;
	GtkWidget *change_mp_button, *browse_button;
	GtkWidget *tab_cdrom_label;
	GtkWidget *data_cd_info, *audio_cd_info, *dvd_info;
	gulong size;
	gchar *mount_point;
	gchar *text_pbar, *hr_size;
	gboolean mounted;

	g_return_if_fail (GST_IS_CDROM_DISC_DATA (disc_data));

	point_entry = gst_dialog_get_widget (tool->main_dialog, "cd_point_entry");
	size_progress = gst_dialog_get_widget (tool->main_dialog, "cd_size_progress");
	mount_button = gst_dialog_get_widget (tool->main_dialog, "cd_mount_button");
	status_label = gst_dialog_get_widget (tool->main_dialog, "cd_status_label");
	browse_button = gst_dialog_get_widget (tool->main_dialog, "cd_browse_button");
	change_mp_button = gst_dialog_get_widget (tool->main_dialog, "cd_change_mp_button");
	tab_cdrom_label = gst_dialog_get_widget (tool->main_dialog, "tab_cdrom_label");
	data_cd_info = gst_dialog_get_widget (tool->main_dialog, "data_cd_info");
	audio_cd_info = gst_dialog_get_widget (tool->main_dialog, "audio_cd_info");
	dvd_info = gst_dialog_get_widget (tool->main_dialog, "dvd_info");
	
	gtk_label_set_text (GTK_LABEL (tab_cdrom_label), _("Data CD-ROM"));

	gtk_widget_hide (audio_cd_info);
	gtk_widget_hide (dvd_info);
	
	g_object_get (G_OBJECT (disc_data), "size", &size,
		      "mount-point", &mount_point,
		      "mounted", &mounted, NULL);

	if (mount_point)
		gtk_entry_set_text (GTK_ENTRY (point_entry), mount_point);
	else
		gtk_entry_set_text (GTK_ENTRY (point_entry), "");

	gst_disks_gui_setup_mounted (status_label, mount_button, mounted);

	if (mounted) {
		gtk_widget_set_sensitive (browse_button, TRUE);
		
		gtk_progress_bar_set_fraction (
			GTK_PROGRESS_BAR (size_progress), 1.0);
		
		hr_size = gst_storage_get_human_readable_size (size);
		text_pbar = g_strdup_printf ("%s", hr_size);
		if (hr_size) g_free (hr_size);
		
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), text_pbar);
		g_free (text_pbar);
	} else {
		gtk_widget_set_sensitive (browse_button, FALSE);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (size_progress), 0);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (size_progress), _("Not Available"));
	}

	gtk_widget_show_all (data_cd_info);
}

/* CDROM Disc Audio */
void
gst_disks_gui_setup_cdrom_disc_audio (GstCdromDiscAudio *disc_audio)
{
	GtkWidget *num_tracks_label, *duration_label;
	GtkWidget *tab_cdrom_label;
	GtkWidget *data_cd_info, *audio_cd_info, *dvd_info;
	guint num_tracks;
	gchar *text_num_tracks;
	gchar *duration;

	g_return_if_fail (GST_IS_CDROM_DISC_AUDIO (disc_audio));

	num_tracks_label = gst_dialog_get_widget (tool->main_dialog, "cd_num_tracks_label");
	duration_label = gst_dialog_get_widget (tool->main_dialog, "cd_duration_label");
	tab_cdrom_label = gst_dialog_get_widget (tool->main_dialog, "tab_cdrom_label");
	data_cd_info = gst_dialog_get_widget (tool->main_dialog, "data_cd_info");
	audio_cd_info = gst_dialog_get_widget (tool->main_dialog, "audio_cd_info");
	dvd_info = gst_dialog_get_widget (tool->main_dialog, "dvd_info");

	gtk_label_set_text (GTK_LABEL (tab_cdrom_label), _("Audio CD-ROM"));

	gtk_widget_hide (data_cd_info);
	gtk_widget_hide (dvd_info);

	g_object_get (G_OBJECT (disc_audio), "num-tracks", &num_tracks,
		      "duration", &duration, NULL);

	text_num_tracks = g_strdup_printf ("%d", num_tracks);
	gtk_label_set_text (GTK_LABEL (num_tracks_label), text_num_tracks);
	g_free (text_num_tracks);

	if (duration)
		gtk_label_set_text (GTK_LABEL (duration_label), duration);
	else
		gtk_label_set_text (GTK_LABEL (duration_label), "");
	
	gtk_widget_show_all (audio_cd_info);
}

/* CDROM Disc Mixed */
void
gst_disks_gui_setup_cdrom_disc_mixed (GstCdromDiscMixed *disc_mixed)
{
	GtkWidget *data_cd_info, *audio_cd_info, *dvd_info;
	GtkWidget *tab_cdrom_label;
	GstCdromDiscData  *data = NULL;
	GstCdromDiscAudio *audio = NULL;

	g_return_if_fail (GST_IS_CDROM_DISC_MIXED (disc_mixed));

	data_cd_info = gst_dialog_get_widget (tool->main_dialog, "data_cd_info");
	audio_cd_info = gst_dialog_get_widget (tool->main_dialog, "audio_cd_info");
	dvd_info = gst_dialog_get_widget (tool->main_dialog, "dvd_info");
	tab_cdrom_label = gst_dialog_get_widget (tool->main_dialog, "tab_cdrom_label");

	g_object_get (G_OBJECT (disc_mixed), "data", &data, "audio", &audio, NULL);

	if (data)
		gst_disks_gui_setup_cdrom_disc_data (data);

	if (audio)
		gst_disks_gui_setup_cdrom_disc_audio (audio);

	gtk_label_set_text (GTK_LABEL (tab_cdrom_label), _("Mixed CD-ROM"));

	gtk_widget_hide (dvd_info);
	
	gtk_widget_show_all (data_cd_info);
	gtk_widget_show_all (audio_cd_info);
}

/* CDROM Disc DVD */
void 
gst_disks_gui_setup_cdrom_disc_dvd (GstCdromDiscDvd *disc_dvd)
{
	GtkWidget *tab_cdrom_label;
	GtkWidget *data_cd_info, *audio_cd_info, *dvd_info;

	g_return_if_fail (GST_IS_CDROM_DISC_DVD (disc_dvd));

	tab_cdrom_label = gst_dialog_get_widget (tool->main_dialog, "tab_cdrom_label");
	data_cd_info = gst_dialog_get_widget (tool->main_dialog, "data_cd_info");
	audio_cd_info = gst_dialog_get_widget (tool->main_dialog, "audio_cd_info");
	dvd_info = gst_dialog_get_widget (tool->main_dialog, "dvd_info");

	gtk_label_set_text (GTK_LABEL (tab_cdrom_label), _("DVD Video"));

	gtk_widget_hide (data_cd_info);
	gtk_widget_hide (audio_cd_info);

	gtk_widget_show_all (dvd_info);
}
